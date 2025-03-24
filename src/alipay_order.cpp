#include "alipay_order.h"
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <iomanip>

AlipayOrder::AlipayOrder() : conn(nullptr) {
}

AlipayOrder::~AlipayOrder() {
    if (conn) {
        mysql_close(conn);
    }
}

bool AlipayOrder::connectDB(const char* host, const char* user, 
                          const char* password, const char* db) {
    conn = mysql_init(nullptr);
    if (!conn) {
        return false;
    }
    
    if (!mysql_real_connect(conn, host, user, password, db, 0, nullptr, 0)) {
        return false;
    }
    
    // 创建订单表
    const char* create_table = "CREATE TABLE IF NOT EXISTS alipay_orders ("
        "order_id VARCHAR(32) PRIMARY KEY,"
        "trade_no VARCHAR(64),"
        "user_id VARCHAR(32),"
        "amount DECIMAL(10,2),"
        "status VARCHAR(20),"
        "create_time TIMESTAMP,"
        "pay_time TIMESTAMP NULL"
        ")";
    
    if (mysql_query(conn, create_table)) {
        return false;
    }
    
    return true;
}

bool AlipayOrder::createOrder(const std::string& userId, double amount) {
    // 生成订单号 (简单实现，实际应更复杂)
    std::time_t now = std::time(nullptr);
    std::stringstream ss;
    ss << "ORDER" << now << rand() % 1000;
    order_id = ss.str();
    
    std::string query = "INSERT INTO alipay_orders "
        "(order_id, user_id, amount, status, create_time) VALUES "
        "('" + order_id + "', '" + userId + "', " + 
        std::to_string(amount) + ", 'CREATED', NOW())";
    
    if (mysql_query(conn, query.c_str())) {
        return false;
    }
    
    this->user_id = userId;
    this->amount = amount;
    this->status = "CREATED";
    this->create_time = now;
    
    return true;
}

bool AlipayOrder::updateOrderStatus(const std::string& orderId, 
                                  const std::string& status) {
    std::string query = "UPDATE alipay_orders SET status = '" + status + "'";
    if (status == "PAID") {
        query += ", pay_time = NOW()";
    }
    query += " WHERE order_id = '" + orderId + "'";
    
    if (mysql_query(conn, query.c_str())) {
        return false;
    }
    
    return mysql_affected_rows(conn) > 0;
}

bool AlipayOrder::queryOrder(const std::string& orderId) {
    std::string query = "SELECT * FROM alipay_orders WHERE order_id = '" + 
                       orderId + "'";
    
    if (mysql_query(conn, query.c_str())) {
        return false;
    }
    
    MYSQL_RES* result = mysql_store_result(conn);
    if (!result) {
        return false;
    }
    
    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row) {
        mysql_free_result(result);
        return false;
    }
    
    order_id = row[0];
    trade_no = row[1] ? row[1] : "";
    user_id = row[2];
    amount = std::stod(row[3]);
    status = row[4];
    
    mysql_free_result(result);
    return true;
} 