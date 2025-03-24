#pragma once

#include <string>
#include <optional>
#include <cstdint>
#include <mysql/mysql.h>

class AlipayPayment {
public:
    // 交易状态常量
    static constexpr const char* TRADE_STATUS_WAIT_BUYER_PAY = "WAIT_BUYER_PAY";  // 交易创建，等待买家付款
    static constexpr const char* TRADE_STATUS_TRADE_CLOSED = "TRADE_CLOSED";      // 未付款交易超时关闭，或支付完成后全额退款
    static constexpr const char* TRADE_STATUS_TRADE_SUCCESS = "TRADE_SUCCESS";    // 交易支付成功
    static constexpr const char* TRADE_STATUS_TRADE_FINISHED = "TRADE_FINISHED";  // 交易结束，不可退款

    AlipayPayment();
    ~AlipayPayment();

    // 数据库连接
    bool connectDB(const char* host, const char* user, 
                  const char* password, const char* db);

    // 支付操作
    bool createPayment(const std::string& outTradeNo);
    bool queryPayment(const std::string& outTradeNo);
    bool updatePaymentStatus(const std::string& outTradeNo, 
                           const std::string& tradeNo,
                           const std::string& status);

    // Setters
    void setTradeNo(const std::string& value);       // 支付宝交易号(64)
    void setTradeStatus(const std::string& value);   // 交易状态
    void setPayTime(uint64_t timestamp);             // 支付时间戳(秒)

    // Getters
    std::string getOutTradeNo() const;
    std::string getTradeNo() const;
    std::string getTradeStatus() const;
    uint64_t getPayTime() const;
    uint64_t getUpdateTime() const;

    // 工具方法
    static std::string timestampToString(uint64_t timestamp);
    static uint64_t stringToTimestamp(const std::string& timeStr);

private:
    MYSQL* conn;

    // 支付信息
    std::string out_trade_no_;                   // 商户订单号
    std::optional<std::string> trade_no_;        // 支付宝交易号
    std::optional<std::string> trade_status_;    // 交易状态
    std::optional<uint64_t> pay_time_;           // 支付时间戳
    uint64_t update_time_;                       // 状态更新时间

    // 数据库操作辅助方法
    bool createPaymentTable();    // 创建支付表
}; 