#include "alipay_payment.h"
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <chrono>

AlipayPayment::AlipayPayment() : conn(nullptr) {}

AlipayPayment::~AlipayPayment() {
    if (conn) {
        mysql_close(conn);
    }
}

bool AlipayPayment::connectDB(const char* host, const char* user, 
                            const char* password, const char* db) {
    conn = mysql_init(nullptr);
    if (!conn) return false;
    
    if (!mysql_real_connect(conn, host, user, password, db, 0, nullptr, 0)) {
        return false;
    }
    
    mysql_set_character_set(conn, "utf8mb4");
    
    // 创建支付表
    if (!createPaymentTable()) {
        return false;
    }
    
    return true;
}

bool AlipayPayment::createPayment(const std::string& outTradeNo) {
    if (!conn) return false;
    
    try {
        std::string query = "INSERT INTO alipay_payments ("
            "out_trade_no, trade_status, update_time"
            ") VALUES (?, ?, ?)";
            
        MYSQL_STMT* stmt = mysql_stmt_init(conn);
        if (!stmt) throw std::runtime_error("mysql_stmt_init failed");
        
        if (mysql_stmt_prepare(stmt, query.c_str(), query.length())) {
            throw std::runtime_error(mysql_stmt_error(stmt));
        }
        
        // 设置当前时间戳
        update_time_ = std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now());
        
        MYSQL_BIND bind[3];
        memset(bind, 0, sizeof(bind));
        
        // 绑定参数
        out_trade_no_ = outTradeNo;
        std::string initial_status = TRADE_STATUS_WAIT_BUYER_PAY;
        
        bind[0].buffer_type = MYSQL_TYPE_STRING;
        bind[0].buffer = (void*)out_trade_no_.c_str();
        bind[0].buffer_length = out_trade_no_.length();
        
        bind[1].buffer_type = MYSQL_TYPE_STRING;
        bind[1].buffer = (void*)initial_status.c_str();
        bind[1].buffer_length = initial_status.length();
        
        bind[2].buffer_type = MYSQL_TYPE_LONGLONG;
        bind[2].buffer = &update_time_;
        bind[2].is_unsigned = true;
        
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

bool AlipayPayment::queryPayment(const std::string& outTradeNo) {
    if (!conn) return false;
    
    try {
        std::string query = "SELECT * FROM alipay_payments WHERE out_trade_no = ?";
            
        MYSQL_STMT* stmt = mysql_stmt_init(conn);
        if (!stmt) throw std::runtime_error("mysql_stmt_init failed");
        
        if (mysql_stmt_prepare(stmt, query.c_str(), query.length())) {
            throw std::runtime_error(mysql_stmt_error(stmt));
        }
        
        MYSQL_BIND bind[1];
        memset(bind, 0, sizeof(bind));
        
        bind[0].buffer_type = MYSQL_TYPE_STRING;
        bind[0].buffer = (void*)outTradeNo.c_str();
        bind[0].buffer_length = outTradeNo.length();
        
        if (mysql_stmt_bind_param(stmt, bind)) {
            throw std::runtime_error(mysql_stmt_error(stmt));
        }
        
        if (mysql_stmt_execute(stmt)) {
            throw std::runtime_error(mysql_stmt_error(stmt));
        }
        
        // 绑定结果集
        MYSQL_BIND result[5];
        memset(result, 0, sizeof(result));
        
        // 准备结果缓冲区
        char out_trade_no_buf[65];
        char trade_no_buf[65];
        char trade_status_buf[33];
        my_bool is_null[5];
        unsigned long length[5];
        
        // 绑定结果字段
        result[0].buffer_type = MYSQL_TYPE_STRING;
        result[0].buffer = out_trade_no_buf;
        result[0].buffer_length = sizeof(out_trade_no_buf);
        result[0].is_null = &is_null[0];
        result[0].length = &length[0];
        
        result[1].buffer_type = MYSQL_TYPE_STRING;
        result[1].buffer = trade_no_buf;
        result[1].buffer_length = sizeof(trade_no_buf);
        result[1].is_null = &is_null[1];
        result[1].length = &length[1];
        
        result[2].buffer_type = MYSQL_TYPE_STRING;
        result[2].buffer = trade_status_buf;
        result[2].buffer_length = sizeof(trade_status_buf);
        result[2].is_null = &is_null[2];
        result[2].length = &length[2];
        
        uint64_t pay_time_val;
        result[3].buffer_type = MYSQL_TYPE_LONGLONG;
        result[3].buffer = &pay_time_val;
        result[3].is_unsigned = true;
        result[3].is_null = &is_null[3];
        
        result[4].buffer_type = MYSQL_TYPE_LONGLONG;
        result[4].buffer = &update_time_;
        result[4].is_unsigned = true;
        result[4].is_null = &is_null[4];
        
        if (mysql_stmt_bind_result(stmt, result)) {
            throw std::runtime_error(mysql_stmt_error(stmt));
        }
        
        if (mysql_stmt_fetch(stmt)) {
            throw std::runtime_error(mysql_stmt_error(stmt));
        }
        
        // 设置查询结果到对象属性
        out_trade_no_ = std::string(out_trade_no_buf, length[0]);
        
        if (!is_null[1]) {
            trade_no_ = std::string(trade_no_buf, length[1]);
        }
        
        if (!is_null[2]) {
            trade_status_ = std::string(trade_status_buf, length[2]);
        }
        
        if (!is_null[3]) {
            pay_time_ = pay_time_val;
        }
        
        mysql_stmt_close(stmt);
        return true;
    }
    catch (const std::exception& e) {
        return false;
    }
}

bool AlipayPayment::updatePaymentStatus(const std::string& outTradeNo, 
                                      const std::string& tradeNo,
                                      const std::string& status) {
    if (!conn) return false;
    
    try {
        update_time_ = std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now());
            
        std::string query = "UPDATE alipay_payments SET "
            "trade_no = ?, trade_status = ?, "
            "pay_time = IF(? = 'TRADE_SUCCESS', ?, pay_time), "
            "update_time = ? "
            "WHERE out_trade_no = ?";
            
        MYSQL_STMT* stmt = mysql_stmt_init(conn);
        if (!stmt) throw std::runtime_error("mysql_stmt_init failed");
        
        if (mysql_stmt_prepare(stmt, query.c_str(), query.length())) {
            throw std::runtime_error(mysql_stmt_error(stmt));
        }
        
        MYSQL_BIND bind[6];
        memset(bind, 0, sizeof(bind));
        
        bind[0].buffer_type = MYSQL_TYPE_STRING;
        bind[0].buffer = (void*)tradeNo.c_str();
        bind[0].buffer_length = tradeNo.length();
        
        bind[1].buffer_type = MYSQL_TYPE_STRING;
        bind[1].buffer = (void*)status.c_str();
        bind[1].buffer_length = status.length();
        
        bind[2].buffer_type = MYSQL_TYPE_STRING;
        bind[2].buffer = (void*)status.c_str();
        bind[2].buffer_length = status.length();
        
        bind[3].buffer_type = MYSQL_TYPE_LONGLONG;
        bind[3].buffer = &update_time_;
        bind[3].is_unsigned = true;
        
        bind[4].buffer_type = MYSQL_TYPE_LONGLONG;
        bind[4].buffer = &update_time_;
        bind[4].is_unsigned = true;
        
        bind[5].buffer_type = MYSQL_TYPE_STRING;
        bind[5].buffer = (void*)outTradeNo.c_str();
        bind[5].buffer_length = outTradeNo.length();
        
        if (mysql_stmt_bind_param(stmt, bind)) {
            throw std::runtime_error(mysql_stmt_error(stmt));
        }
        
        if (mysql_stmt_execute(stmt)) {
            throw std::runtime_error(mysql_stmt_error(stmt));
        }
        
        mysql_stmt_close(stmt);
        
        // 更新本地状态
        out_trade_no_ = outTradeNo;
        trade_no_ = tradeNo;
        trade_status_ = status;
        if (status == TRADE_STATUS_TRADE_SUCCESS) {
            pay_time_ = update_time_;
        }
        
        return true;
    }
    catch (const std::exception& e) {
        return false;
    }
}

bool AlipayPayment::createPaymentTable() {
    if (!conn) return false;
    
    const char* sql = R"SQL(
        CREATE TABLE IF NOT EXISTS alipay_payments (
            out_trade_no VARCHAR(64) PRIMARY KEY,    -- 商户订单号
            trade_no VARCHAR(64),                    -- 支付宝交易号
            trade_status VARCHAR(32) NOT NULL,       -- 交易状态
            pay_time BIGINT UNSIGNED,                -- 支付时间戳
            update_time BIGINT UNSIGNED NOT NULL,    -- 状态更新时间
            FOREIGN KEY (out_trade_no) REFERENCES alipay_orders(out_trade_no),
            INDEX idx_trade_no (trade_no),
            INDEX idx_trade_status (trade_status),
            INDEX idx_update_time (update_time)
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
    )SQL";
    
    return mysql_query(conn, sql) == 0;
}

// Getter 实现
std::string AlipayPayment::getOutTradeNo() const { return out_trade_no_; }
std::string AlipayPayment::getTradeNo() const { return trade_no_.value_or(""); }
std::string AlipayPayment::getTradeStatus() const { return trade_status_.value_or(""); }
uint64_t AlipayPayment::getPayTime() const { return pay_time_.value_or(0); }
uint64_t AlipayPayment::getUpdateTime() const { return update_time_; }

// 时间工具方法实现
std::string AlipayPayment::timestampToString(uint64_t timestamp) {
    time_t time = static_cast<time_t>(timestamp);
    struct tm* timeinfo = localtime(&time);
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    return std::string(buffer);
}

uint64_t AlipayPayment::stringToTimestamp(const std::string& timeStr) {
    struct tm tm = {};
    std::istringstream ss(timeStr);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    if (ss.fail()) {
        throw std::invalid_argument("Invalid time format. Expected: YYYY-MM-DD HH:MM:SS");
    }
    return static_cast<uint64_t>(mktime(&tm));
} 