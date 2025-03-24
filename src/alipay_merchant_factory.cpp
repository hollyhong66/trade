#include "alipay_merchant_factory.h"

MerchantFactory& MerchantFactory::getInstance() {
    static MerchantFactory instance;
    return instance;
}

std::shared_ptr<MerchantType> MerchantFactory::createMerchantChain() {
    // 创建各类型商户
    auto normal = std::make_shared<NormalMerchant>();
    auto isv = std::make_shared<ISVMerchant>();
    auto sub = std::make_shared<SubMerchant>();
    
    // 构建责任链
    normal->setNext(isv);
    isv->setNext(sub);
    
    // 注册商户类型
    registerMerchantType("NORMAL", normal);
    registerMerchantType("ISV", isv);
    registerMerchantType("SUB", sub);
    
    return normal; // 返回链的头部
}

std::shared_ptr<MerchantType> MerchantFactory::createMerchant(
    const std::string& type) {
    auto it = merchant_types_.find(type);
    if (it != merchant_types_.end()) {
        return it->second;
    }
    return nullptr;
}

void MerchantFactory::registerMerchantType(
    const std::string& type, 
    std::shared_ptr<MerchantType> merchant) {
    merchant_types_[type] = merchant;
} 