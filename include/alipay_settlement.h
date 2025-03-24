#pragma once

#include <string>
#include <optional>
#include <cstdint>
#include <mysql/mysql.h>

class AlipaySettlement {
public:
    // 结算状态常量
    static constexpr const char* STATUS_PENDING = "PENDING";        // 待结算
    static constexpr const char* STATUS_PROCESSING = "PROCESSING";  // 结算中
    static constexpr const char* STATUS_SUCCESS = "SUCCESS";        // 结算成功
    static constexpr const char* STATUS_FAILED = "FAILED";         // 结算失败

    AlipaySettlement();
    ~AlipaySettlement();

    // 数据库连接
    bool connectDB(const char* host, const char* user, 
                  const char* password, const char* db);

    // 结算操作
    bool createSettlement(const std::string& outTradeNo,
                         const std::string& merchantId);
    bool querySettlement(const std::string& settlementId);
    bool updateSettlementStatus(const std::string& status);

    // Getters
    std::string getSettlementId() const;
    std::string getMerchantId() const;
    std::string getOutTradeNo() const;
    uint64_t getSettlementAmount() const;
    uint64_t getFeeAmount() const;
    std::string getStatus() const;
    uint64_t getSettleTime() const;

private:
    MYSQL* conn;
    
    std::string settlement_id_;
    std::string merchant_id_;
    std::string out_trade_no_;
    uint64_t settlement_amount_;
    uint64_t fee_amount_;
    std::string status_;
    std::optional<uint64_t> settle_time_;
    uint64_t create_time_;
    uint64_t update_time_;
    std::string bank_account_no_;
    std::string bank_name_;
    std::optional<std::string> remark_;

    bool createSettlementTable();
}; 