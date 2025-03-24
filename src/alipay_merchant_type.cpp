#include "alipay_merchant_type.h"
#include <mysql/mysql.h>

bool NormalMerchant::process(const std::string& merchantId) {
    // 处理普通商户逻辑
    // 1. 验证基本信息
    // 2. 检查费率是否在允许范围内
    // 3. 验证结算周期
    
    if (/* 验证通过 */) {
        return true;
    } else if (next_) {
        return next_->process(merchantId);
    }
    return false;
}

bool ISVMerchant::process(const std::string& merchantId) {
    // 处理服务商逻辑
    // 1. 验证基本信息
    // 2. 检查技术能力认证
    // 3. 验证服务商资质
    // 4. 检查费率是否在允许范围内
    // 5. 验证结算周期
    
    if (/* 验证通过 */) {
        return true;
    } else if (next_) {
        return next_->process(merchantId);
    }
    return false;
}

bool SubMerchant::process(const std::string& merchantId) {
    // 处理子商户逻辑
    // 1. 验证基本信息
    // 2. 验证父商户关系
    // 3. 检查费率是否在允许范围内
    // 4. 验证结算周期
    
    if (/* 验证通过 */) {
        return true;
    } else if (next_) {
        return next_->process(merchantId);
    }
    return false;
} 