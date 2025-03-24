#include "alipay_merchant.h"
#include <sstream>
#include <chrono>
#include <stdexcept>

AlipayMerchant::AlipayMerchant() : conn(nullptr), fee_rate_(0.0) {}

AlipayMerchant::~AlipayMerchant() {
    if (conn) {
        mysql_close(conn);
    }
}

bool AlipayMerchant::connectDB(const char* host, const char* user, 
                              const char* password, const char* db) {
    conn = mysql_init(nullptr);
    if (!conn) return false;
    
    if (!mysql_real_connect(conn, host, user, password, db, 0, nullptr, 0)) {
        return false;
    }
    
    mysql_set_character_set(conn, "utf8mb4");
    return createMerchantTable();
}

bool AlipayMerchant::createMerchant() {
    if (!conn) return false;
    
    try {
        std::string query = "INSERT INTO alipay_merchants ("
            "merchant_id, merchant_name, merchant_type, status, "
            "create_time, update_time, contact_name, contact_phone, contact_email, "
            "bank_account_name, bank_account_no, bank_name, bank_branch, "
            "settlement_type, settlement_cycle, fee_rate"
            ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
            
        MYSQL_STMT* stmt = mysql_stmt_init(conn);
        if (!stmt) throw std::runtime_error("mysql_stmt_init failed");
        
        if (mysql_stmt_prepare(stmt, query.c_str(), query.length())) {
            throw std::runtime_error(mysql_stmt_error(stmt));
        }
        
        // 设置当前时间戳
        create_time_ = update_time_ = std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now());
        
        status_ = "ACTIVE"; // 默认状态为激活
        
        MYSQL_BIND bind[16];
        memset(bind, 0, sizeof(bind));
        
        // 绑定参数
        bind[0].buffer_type = MYSQL_TYPE_STRING;
        bind[0].buffer = (void*)merchant_id_.c_str();
        bind[0].buffer_length = merchant_id_.length();
        
        bind[1].buffer_type = MYSQL_TYPE_STRING;
        bind[1].buffer = (void*)merchant_name_.c_str();
        bind[1].buffer_length = merchant_name_.length();
        
        // ... 绑定其他必填字段 ...
        
        // 绑定可选字段
        my_bool is_null[1] = {1};
        if (contact_email_) {
            bind[8].buffer_type = MYSQL_TYPE_STRING;
            bind[8].buffer = (void*)contact_email_->c_str();
            bind[8].buffer_length = contact_email_->length();
            is_null[0] = 0;
        }
        bind[8].is_null = &is_null[0];
        
        // 绑定费率
        bind[15].buffer_type = MYSQL_TYPE_DOUBLE;
        bind[15].buffer = &fee_rate_;
        
        if (mysql_stmt_bind_param(stmt, bind)) {
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

bool AlipayMerchant::queryMerchant(const std::string& merchantId) {
    if (!conn) return false;
    
    try {
        std::string query = "SELECT * FROM alipay_merchants WHERE merchant_id = ?";
        
        MYSQL_STMT* stmt = mysql_stmt_init(conn);
        if (!stmt) throw std::runtime_error("mysql_stmt_init failed");
        
        if (mysql_stmt_prepare(stmt, query.c_str(), query.length())) {
            throw std::runtime_error(mysql_stmt_error(stmt));
        }
        
        MYSQL_BIND bind[1];
        memset(bind, 0, sizeof(bind));
        
        bind[0].buffer_type = MYSQL_TYPE_STRING;
        bind[0].buffer = (void*)merchantId.c_str();
        bind[0].buffer_length = merchantId.length();
        
        if (mysql_stmt_bind_param(stmt, bind)) {
            throw std::runtime_error(mysql_stmt_error(stmt));
        }
        
        if (mysql_stmt_execute(stmt)) {
            throw std::runtime_error(mysql_stmt_error(stmt));
        }
        
        // 绑定结果集
        MYSQL_BIND result[16];
        memset(result, 0, sizeof(result));
        
        // 准备结果缓冲区
        char merchant_id_buf[33];
        char merchant_name_buf[129];
        // ... 其他字段的缓冲区 ...
        
        // 绑定结果字段
        result[0].buffer_type = MYSQL_TYPE_STRING;
        result[0].buffer = merchant_id_buf;
        result[0].buffer_length = sizeof(merchant_id_buf);
        // ... 绑定其他字段 ...
        
        if (mysql_stmt_bind_result(stmt, result)) {
            throw std::runtime_error(mysql_stmt_error(stmt));
        }
        
        if (mysql_stmt_fetch(stmt)) {
            throw std::runtime_error(mysql_stmt_error(stmt));
        }
        
        // 设置查询结果到对象属性
        merchant_id_ = std::string(merchant_id_buf);
        merchant_name_ = std::string(merchant_name_buf);
        // ... 设置其他字段 ...
        
        mysql_stmt_close(stmt);
        return true;
    }
    catch (const std::exception& e) {
        return false;
    }
}

bool AlipayMerchant::createMerchantTable() {
    if (!conn) return false;
    
    const char* sql = R"SQL(
        CREATE TABLE IF NOT EXISTS alipay_merchants (
            merchant_id VARCHAR(32) PRIMARY KEY,
            merchant_name VARCHAR(128) NOT NULL,
            merchant_type VARCHAR(32) NOT NULL,
            status VARCHAR(16) NOT NULL,
            create_time BIGINT UNSIGNED NOT NULL,
            update_time BIGINT UNSIGNED NOT NULL,
            contact_name VARCHAR(64) NOT NULL,
            contact_phone VARCHAR(32) NOT NULL,
            contact_email VARCHAR(128),
            bank_account_name VARCHAR(128) NOT NULL,
            bank_account_no VARCHAR(32) NOT NULL,
            bank_name VARCHAR(128) NOT NULL,
            bank_branch VARCHAR(256) NOT NULL,
            settlement_type VARCHAR(32) NOT NULL,
            settlement_cycle VARCHAR(32) NOT NULL,
            fee_rate DECIMAL(5,4) NOT NULL,
            INDEX idx_merchant_type (merchant_type),
            INDEX idx_status (status),
            INDEX idx_create_time (create_time)
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
    )SQL";
    
    return mysql_query(conn, sql) == 0;
}

// Setter 实现
void AlipayMerchant::setMerchantId(const std::string& value) {
    merchant_id_ = value;
}

void AlipayMerchant::setMerchantName(const std::string& value) {
    merchant_name_ = value;
}

void AlipayMerchant::setMerchantType(const std::string& value) {
    merchant_type_ = value;
}

void AlipayMerchant::setContactInfo(const std::string& name, 
                                   const std::string& phone, 
                                   const std::string& email) {
    contact_name_ = name;
    contact_phone_ = phone;
    contact_email_ = email;
}

void AlipayMerchant::setBankAccount(const std::string& accountName,
                                   const std::string& accountNo,
                                   const std::string& bankName,
                                   const std::string& bankBranch) {
    bank_account_name_ = accountName;
    bank_account_no_ = accountNo;
    bank_name_ = bankName;
    bank_branch_ = bankBranch;
}

void AlipayMerchant::setSettlementInfo(const std::string& type,
                                      const std::string& cycle,
                                      double feeRate) {
    settlement_type_ = type;
    settlement_cycle_ = cycle;
    fee_rate_ = feeRate;
}

// Getter 实现
std::string AlipayMerchant::getMerchantId() const { return merchant_id_; }
std::string AlipayMerchant::getMerchantName() const { return merchant_name_; }
std::string AlipayMerchant::getMerchantType() const { return merchant_type_; }
double AlipayMerchant::getFeeRate() const { return fee_rate_; }
std::string AlipayMerchant::getSettlementCycle() const { return settlement_cycle_; }
std::string AlipayMerchant::getBankAccountNo() const { return bank_account_no_; } 