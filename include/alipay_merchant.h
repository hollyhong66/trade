#pragma once

#include <string>
#include <optional>
#include <cstdint>
#include <mysql/mysql.h>
#include <memory>

class AlipayMerchant {
public:
    AlipayMerchant();
    ~AlipayMerchant();

    // 数据库连接
    bool connectDB(const char* host, const char* user, 
                  const char* password, const char* db);

    // 商户操作
    bool createMerchant();
    bool queryMerchant(const std::string& merchantId);
    bool updateMerchant();

    // 设置商户信息
    void setMerchantId(const std::string& value);
    void setMerchantName(const std::string& value);
    void setMerchantType(const std::string& value);
    void setContactInfo(const std::string& name, 
                       const std::string& phone, 
                       const std::string& email);
    void setBankAccount(const std::string& accountName,
                       const std::string& accountNo,
                       const std::string& bankName,
                       const std::string& bankBranch);
    void setSettlementInfo(const std::string& type,
                          const std::string& cycle,
                          double feeRate);

    // Getters
    std::string getMerchantId() const;
    std::string getMerchantName() const;
    std::string getMerchantType() const;
    double getFeeRate() const;
    std::string getSettlementCycle() const;
    std::string getBankAccountNo() const;

private:
    MYSQL* conn;
    
    std::string merchant_id_;
    std::string merchant_name_;
    std::string merchant_type_;
    std::string status_;
    uint64_t create_time_;
    uint64_t update_time_;
    
    std::string contact_name_;
    std::string contact_phone_;
    std::optional<std::string> contact_email_;
    
    std::string bank_account_name_;
    std::string bank_account_no_;
    std::string bank_name_;
    std::string bank_branch_;
    
    std::string settlement_type_;
    std::string settlement_cycle_;
    double fee_rate_;

    std::shared_ptr<MerchantType> merchant_type_;

    bool createMerchantTable();
}; 