#pragma once

#include "alipay_merchant_type.h"
#include <memory>
#include <string>
#include <unordered_map>

class MerchantFactory {
public:
    static MerchantFactory& getInstance();
    
    // 创建商户类型处理链
    std::shared_ptr<MerchantType> createMerchantChain();
    
    // 根据类型创建具体商户
    std::shared_ptr<MerchantType> createMerchant(const std::string& type);
    
    // 注册商户类型
    void registerMerchantType(const std::string& type, 
                            std::shared_ptr<MerchantType> merchant);

private:
    MerchantFactory() = default;
    ~MerchantFactory() = default;
    
    std::unordered_map<std::string, std::shared_ptr<MerchantType>> merchant_types_;
}; 