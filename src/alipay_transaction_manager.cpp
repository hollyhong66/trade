#include "alipay_transaction_manager.h"
#include <chrono>
#include <sstream>

AlipayTransactionManager& AlipayTransactionManager::getInstance() {
    static AlipayTransactionManager instance;
    return instance;
}

AlipayTransactionManager::AlipayTransactionManager() : conn_(nullptr) {
    conn_ = mysql_init(nullptr);
    if (conn_) {
        mysql_real_connect(conn_, "localhost", "username", "password", "dbname", 0, nullptr, 0);
        mysql_set_character_set(conn_, "utf8mb4");
        createTransactionTable();
    }
}

AlipayTransactionManager::~AlipayTransactionManager() {
    if (conn_) {
        mysql_close(conn_);
    }
}

bool AlipayTransactionManager::createTransactionTable() {
    const char* sql = R"SQL(
        CREATE TABLE IF NOT EXISTS alipay_transactions (
            xid VARCHAR(128) PRIMARY KEY,
            status VARCHAR(32) NOT NULL,
            create_time BIGINT UNSIGNED NOT NULL,
            update_time BIGINT UNSIGNED NOT NULL,
            order_no VARCHAR(64) NOT NULL,
            participants TEXT,
            INDEX idx_status (status),
            INDEX idx_create_time (create_time),
            INDEX idx_order_no (order_no)
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
    )SQL";
    
    return mysql_query(conn_, sql) == 0;
}

bool AlipayTransactionManager::startTransaction(
    const std::string& orderNo,
    std::shared_ptr<AlipayTransaction>& transaction) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        // 创建新事务
        auto xid = AlipayTransaction::generateXID("TXN");
        transaction = std::make_shared<AlipayTransaction>();
        
        if (!transaction->connectDB("localhost", "username", "password", "dbname")) {
            return false;
        }
        
        if (!transaction->beginTransaction(xid)) {
            return false;
        }
        
        // 记录事务信息
        TransactionRecord record{
            .xid = xid,
            .status = TransactionStatus::STARTED,
            .create_time = std::chrono::system_clock::to_time_t(
                std::chrono::system_clock::now()),
            .update_time = record.create_time,
            .order_no = orderNo,
            .participants = {"orders", "payments"}
        };
        
        if (!saveTransactionRecord(record)) {
            transaction->rollbackTransaction();
            return false;
        }
        
        active_transactions_[xid] = record;
        return true;
    }
    catch (const std::exception&) {
        return false;
    }
}

bool AlipayTransactionManager::prepareTransaction(const std::string& xid) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = active_transactions_.find(xid);
    if (it == active_transactions_.end()) {
        return false;
    }
    
    try {
        // 更新事务状态
        it->second.status = TransactionStatus::PREPARED;
        it->second.update_time = std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now());
            
        return updateTransactionStatus(xid, TransactionStatus::PREPARED);
    }
    catch (const std::exception&) {
        return false;
    }
}

bool AlipayTransactionManager::commitTransaction(const std::string& xid) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = active_transactions_.find(xid);
    if (it == active_transactions_.end()) {
        return false;
    }
    
    try {
        // 更新事务状态
        it->second.status = TransactionStatus::COMMITTED;
        it->second.update_time = std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now());
            
        if (!updateTransactionStatus(xid, TransactionStatus::COMMITTED)) {
            return false;
        }
        
        // 移除活动事务
        active_transactions_.erase(it);
        return true;
    }
    catch (const std::exception&) {
        return false;
    }
}

void AlipayTransactionManager::recoverTransactions() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        // 查询所有未完成的事务
        std::string query = "SELECT * FROM alipay_transactions "
                           "WHERE status IN ('STARTED', 'PREPARED')";
                           
        if (mysql_query(conn_, query.c_str()) != 0) {
            return;
        }
        
        MYSQL_RES* result = mysql_store_result(conn_);
        if (!result) {
            return;
        }
        
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(result))) {
            TransactionRecord record{
                .xid = row[0],
                .status = TransactionStatus::STARTED,
                .create_time = std::stoull(row[2]),
                .update_time = std::stoull(row[3]),
                .order_no = row[4]
            };
            
            // 解析参与者列表
            std::string participants(row[5]);
            // ... 解析逻辑 ...
            
            active_transactions_[record.xid] = record;
        }
        
        mysql_free_result(result);
    }
    catch (const std::exception&) {
        // 记录错误日志
    }
} 