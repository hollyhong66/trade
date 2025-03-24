#include "alipay_settlement.h"
#include <sstream>
#include <chrono>
#include <stdexcept>

AlipaySettlement::AlipaySettlement() : conn(nullptr), 
    settlement_amount_(0), fee_amount_(0) {}

AlipaySettlement::~AlipaySettlement() {
    if (conn) {
        mysql_close(conn);
    }
}

bool AlipaySettlement::connectDB(const char* host, const char* user, 
                                const char* password, const char* db) {
    conn = mysql_init(nullptr);
    if (!conn) return false;
    
    if (!mysql_real_connect(conn, host, user, password, db, 0, nullptr, 0)) {
        return false;
    }
    
    mysql_set_character_set(conn, "utf8mb4");
    return createSettlementTable();
}

bool AlipaySettlement::createSettlement(const std::string& outTradeNo,
                                      const std::string& merchantId) {
    if (!conn) return false;
    
    try {
        // 首先查询订单和商户信息
        std::string query = "SELECT o.total_amount, m.fee_rate, "
            "m.bank_account_no, m.bank_name "
            "FROM alipay_orders o "
            "JOIN alipay_merchants m ON o.merchant_id = m.merchant_id "
            "WHERE o.out_trade_no = ? AND m.merchant_id = ?";
            
        MYSQL_STMT* stmt = mysql_stmt_init(conn);
        if (!stmt) throw std::runtime_error("mysql_stmt_init failed");
        
        if (mysql_stmt_prepare(stmt, query.c_str(), query.length())) {
            throw std::runtime_error(mysql_stmt_error(stmt));
        }
        
        MYSQL_BIND bind[2];
        memset(bind, 0, sizeof(bind));
        
        bind[0].buffer_type = MYSQL_TYPE_STRING;
        bind[0].buffer = (void*)outTradeNo.c_str();
        bind[0].buffer_length = outTradeNo.length();
        
        bind[1].buffer_type = MYSQL_TYPE_STRING;
        bind[1].buffer = (void*)merchantId.c_str();
        bind[1].buffer_length = merchantId.length();
        
        if (mysql_stmt_bind_param(stmt, bind)) {
            throw std::runtime_error(mysql_stmt_error(stmt));
        }
        
        if (mysql_stmt_execute(stmt)) {
            throw std::runtime_error(mysql_stmt_error(stmt));
        }
        
        // 绑定结果
        MYSQL_BIND result[4];
        memset(result, 0, sizeof(result));
        
        uint64_t total_amount;
        double fee_rate;
        char bank_account_no[33];
        char bank_name[129];
        unsigned long bank_account_no_length;
        unsigned long bank_name_length;
        
        result[0].buffer_type = MYSQL_TYPE_LONGLONG;
        result[0].buffer = &total_amount;
        result[0].is_unsigned = true;
        
        result[1].buffer_type = MYSQL_TYPE_DOUBLE;
        result[1].buffer = &fee_rate;
        
        result[2].buffer_type = MYSQL_TYPE_STRING;
        result[2].buffer = bank_account_no;
        result[2].buffer_length = sizeof(bank_account_no);
        result[2].length = &bank_account_no_length;
        
        result[3].buffer_type = MYSQL_TYPE_STRING;
        result[3].buffer = bank_name;
        result[3].buffer_length = sizeof(bank_name);
        result[3].length = &bank_name_length;
        
        if (mysql_stmt_bind_result(stmt, result)) {
            throw std::runtime_error(mysql_stmt_error(stmt));
        }
        
        if (mysql_stmt_fetch(stmt)) {
            throw std::runtime_error("Order or merchant not found");
        }
        
        mysql_stmt_close(stmt);
        
        // 计算手续费和结算金额
        fee_amount_ = static_cast<uint64_t>(total_amount * fee_rate);
        settlement_amount_ = total_amount - fee_amount_;
        
        // 生成结算单号
        settlement_id_ = "SETTLE_" + outTradeNo;
        merchant_id_ = merchantId;
        out_trade_no_ = outTradeNo;
        bank_account_no_ = std::string(bank_account_no, bank_account_no_length);
        bank_name_ = std::string(bank_name, bank_name_length);
        status_ = STATUS_PENDING;
        
        // 获取当前时间
        create_time_ = update_time_ = std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now());
        
        // 插入结算记录
        query = "INSERT INTO alipay_settlements ("
            "settlement_id, merchant_id, out_trade_no, settlement_amount, "
            "fee_amount, status, create_time, update_time, "
            "bank_account_no, bank_name"
            ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
            
        stmt = mysql_stmt_init(conn);
        if (!stmt) throw std::runtime_error("mysql_stmt_init failed");
        
        if (mysql_stmt_prepare(stmt, query.c_str(), query.length())) {
            throw std::runtime_error(mysql_stmt_error(stmt));
        }
        
        MYSQL_BIND insert_bind[10];
        memset(insert_bind, 0, sizeof(insert_bind));
        
        // 绑定插入参数
        insert_bind[0].buffer_type = MYSQL_TYPE_STRING;
        insert_bind[0].buffer = (void*)settlement_id_.c_str();
        insert_bind[0].buffer_length = settlement_id_.length();
        
        // ... 绑定其他参数 ...
        
        if (mysql_stmt_bind_param(stmt, insert_bind)) {
            throw std::runtime_error(mysql_stmt_error(stmt));
        }
        
        if (mysql_stmt_execute(stmt)) {
            throw std::runtime_error(mysql_stmt_error(stmt));
        }
        
        mysql_stmt_close(stmt);
        return true;
    }
    catch (const std::exception& e) {
        return false;
    }
}

bool AlipaySettlement::updateSettlementStatus(const std::string& status) {
    if (!conn || settlement_id_.empty()) return false;
    
    try {
        std::string query = "UPDATE alipay_settlements SET "
            "status = ?, update_time = ?, "
            "settle_time = IF(? = 'SUCCESS', ?, settle_time) "
            "WHERE settlement_id = ?";
            
        MYSQL_STMT* stmt = mysql_stmt_init(conn);
        if (!stmt) throw std::runtime_error("mysql_stmt_init failed");
        
        if (mysql_stmt_prepare(stmt, query.c_str(), query.length())) {
            throw std::runtime_error(mysql_stmt_error(stmt));
        }
        
        update_time_ = std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now());
            
        MYSQL_BIND bind[5];
        memset(bind, 0, sizeof(bind));
        
        bind[0].buffer_type = MYSQL_TYPE_STRING;
        bind[0].buffer = (void*)status.c_str();
        bind[0].buffer_length = status.length();
        
        bind[1].buffer_type = MYSQL_TYPE_LONGLONG;
        bind[1].buffer = &update_time_;
        bind[1].is_unsigned = true;
        
        bind[2].buffer_type = MYSQL_TYPE_STRING;
        bind[2].buffer = (void*)status.c_str();
        bind[2].buffer_length = status.length();
        
        bind[3].buffer_type = MYSQL_TYPE_LONGLONG;
        bind[3].buffer = &update_time_;
        bind[3].is_unsigned = true;
        
        bind[4].buffer_type = MYSQL_TYPE_STRING;
        bind[4].buffer = (void*)settlement_id_.c_str();
        bind[4].buffer_length = settlement_id_.length();
        
        if (mysql_stmt_bind_param(stmt, bind)) {
            throw std::runtime_error(mysql_stmt_error(stmt));
        }
        
        if (mysql_stmt_execute(stmt)) {
            throw std::runtime_error(mysql_stmt_error(stmt));
        }
        
        mysql_stmt_close(stmt);
        
        status_ = status;
        if (status == STATUS_SUCCESS) {
            settle_time_ = update_time_;
        }
        
        return true;
    }
    catch (const std::exception& e) {
        return false;
    }
}

bool AlipaySettlement::createSettlementTable() {
    if (!conn) return false;
    
    const char* sql = R"SQL(
        CREATE TABLE IF NOT EXISTS alipay_settlements (
            settlement_id VARCHAR(64) PRIMARY KEY,
            merchant_id VARCHAR(32) NOT NULL,
            out_trade_no VARCHAR(64) NOT NULL,
            settlement_amount BIGINT UNSIGNED NOT NULL,
            fee_amount BIGINT UNSIGNED NOT NULL,
            status VARCHAR(32) NOT NULL,
            settle_time BIGINT UNSIGNED,
            create_time BIGINT UNSIGNED NOT NULL,
            update_time BIGINT UNSIGNED NOT NULL,
            bank_account_no VARCHAR(32) NOT NULL,
            bank_name VARCHAR(128) NOT NULL,
            remark VARCHAR(256),
            INDEX idx_merchant_id (merchant_id),
            INDEX idx_out_trade_no (out_trade_no),
            INDEX idx_create_time (create_time),
            INDEX idx_status (status),
            FOREIGN KEY (merchant_id) REFERENCES alipay_merchants(merchant_id),
            FOREIGN KEY (out_trade_no) REFERENCES alipay_orders(out_trade_no)
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
    )SQL";
    
    return mysql_query(conn, sql) == 0;
}

// Getter 实现
std::string AlipaySettlement::getSettlementId() const { return settlement_id_; }
std::string AlipaySettlement::getMerchantId() const { return merchant_id_; }
std::string AlipaySettlement::getOutTradeNo() const { return out_trade_no_; }
uint64_t AlipaySettlement::getSettlementAmount() const { return settlement_amount_; }
uint64_t AlipaySettlement::getFeeAmount() const { return fee_amount_; }
std::string AlipaySettlement::getStatus() const { return status_; }
uint64_t AlipaySettlement::getSettleTime() const { return settle_time_.value_or(0); } 