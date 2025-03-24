#ifndef ALIPAY_ORDER_H
#define ALIPAY_ORDER_H

#include <string>
#include <mysql/mysql.h>
#include <ctime>

class AlipayOrder {
private:
    std::string order_id;        // 订单号
    std::string trade_no;        // 支付宝交易号
    std::string user_id;         // 用户ID
    double amount;               // 订单金额
    std::string status;          // 订单状态
    time_t create_time;          // 创建时间
    time_t pay_time;            // 支付时间
    MYSQL* conn;                // MySQL连接

public:
    AlipayOrder();
    ~AlipayOrder();
    
    // 数据库连接
    bool connectDB(const char* host, const char* user, 
                  const char* password, const char* db);
    
    // 创建订单
    bool createOrder(const std::string& userId, double amount);
    
    // 更新订单状态
    bool updateOrderStatus(const std::string& orderId, const std::string& status);
    
    // 查询订单
    bool queryOrder(const std::string& orderId);
    
    // getter方法
    std::string getOrderId() const { return order_id; }
    std::string getTradeNo() const { return trade_no; }
    std::string getStatus() const { return status; }
    double getAmount() const { return amount; }
};

#endif 