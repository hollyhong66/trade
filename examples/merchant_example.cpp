#include "alipay_merchant.h"
#include "alipay_merchant_factory.h"
#include <iostream>
#include <iomanip>

// 辅助函数：打印商户信息
void printMerchantInfo(const AlipayMerchant& merchant) {
    std::cout << "\n商户信息：" << std::endl;
    std::cout << "商户ID: " << merchant.getMerchantId() << std::endl;
    std::cout << "商户名称: " << merchant.getMerchantName() << std::endl;
    std::cout << "商户类型: " << merchant.getMerchantType() << std::endl;
    std::cout << "费率: " << std::fixed << std::setprecision(2) 
              << merchant.getFeeRate() * 100 << "%" << std::endl;
    std::cout << "结算周期: " << merchant.getSettlementCycle() << std::endl;
}

int main() {
    try {
        // 1. 创建普通商户
        AlipayMerchant normalMerchant;
        normalMerchant.connectDB("localhost", "username", "password", "alipay_db");
        
        normalMerchant.setMerchantId("NORMAL_001");
        normalMerchant.setMerchantName("普通商户测试");
        normalMerchant.setMerchantType("NORMAL");
        normalMerchant.setContactInfo(
            "张三",
            "13800138000",
            "zhangsan@example.com"
        );
        normalMerchant.setBankAccount(
            "张三",
            "6222021234567890123",
            "中国工商银行",
            "北京市海淀支行"
        );
        normalMerchant.setSettlementInfo(
            "T+1",      // 结算类型
            "DAILY",    // 结算周期
            0.006       // 费率 0.6%
        );
        
        if (normalMerchant.createMerchant()) {
            printMerchantInfo(normalMerchant);
        }
        
        // 2. 创建服务商
        AlipayMerchant isvMerchant;
        isvMerchant.connectDB("localhost", "username", "password", "alipay_db");
        
        isvMerchant.setMerchantId("ISV_001");
        isvMerchant.setMerchantName("科技服务商");
        isvMerchant.setMerchantType("ISV");
        isvMerchant.setContactInfo(
            "李四",
            "13900139000",
            "lisi@tech.com"
        );
        isvMerchant.setBankAccount(
            "科技有限公司",
            "6222021234567890124",
            "中国建设银行",
            "北京市中关村支行"
        );
        isvMerchant.setSettlementInfo(
            "T+1",
            "WEEKLY",   // 按周结算
            0.004       // 费率 0.4%
        );
        
        // 设置服务商特有信息
        isvMerchant.setISVInfo(
            "技术服务认证等级A",           // 技术认证
            "ISV_CERT_001",              // 认证证书编号
            {"支付接入", "营销工具"},      // 服务类型
            5                            // 服务年限
        );
        
        if (isvMerchant.createMerchant()) {
            printMerchantInfo(isvMerchant);
        }
        
        // 3. 创建子商户
        AlipayMerchant subMerchant;
        subMerchant.connectDB("localhost", "username", "password", "alipay_db");
        
        subMerchant.setMerchantId("SUB_001");
        subMerchant.setMerchantName("子商户测试");
        subMerchant.setMerchantType("SUB");
        subMerchant.setParentId("ISV_001");  // 关联服务商
        subMerchant.setContactInfo(
            "王五",
            "13700137000",
            "wangwu@sub.com"
        );
        subMerchant.setBankAccount(
            "王五",
            "6222021234567890125",
            "中国农业银行",
            "北京市朝阳支行"
        );
        subMerchant.setSettlementInfo(
            "T+1",
            "DAILY",
            0.005       // 费率 0.5%
        );
        
        if (subMerchant.createMerchant()) {
            printMerchantInfo(subMerchant);
        }
        
        // 4. 测试商户关系和费率计算
        std::cout << "\n测试商户关系：" << std::endl;
        
        // 查询子商户的父商户
        auto parentId = subMerchant.getParentId();
        if (!parentId.empty()) {
            std::cout << "子商户(" << subMerchant.getMerchantId() 
                      << ")的父商户: " << parentId << std::endl;
            
            // 计算实际费率分成
            double subFeeRate = subMerchant.getFeeRate();
            double isvFeeRate = isvMerchant.getFeeRate();
            double profitShare = subFeeRate - isvFeeRate;
            
            std::cout << "子商户费率: " << std::fixed << std::setprecision(2) 
                      << subFeeRate * 100 << "%" << std::endl;
            std::cout << "服务商费率: " << isvFeeRate * 100 << "%" << std::endl;
            std::cout << "服务商分润: " << profitShare * 100 << "%" << std::endl;
        }
        
        // 5. 测试商户状态变更
        std::cout << "\n测试商户状态变更：" << std::endl;
        
        // 暂停商户
        if (normalMerchant.updateStatus("SUSPENDED", "违规操作")) {
            std::cout << "商户(" << normalMerchant.getMerchantId() 
                      << ")已暂停，原因: 违规操作" << std::endl;
        }
        
        // 恢复商户
        if (normalMerchant.updateStatus("ACTIVE", "整改完成")) {
            std::cout << "商户(" << normalMerchant.getMerchantId() 
                      << ")已恢复，原因: 整改完成" << std::endl;
        }
        
        // 6. 测试费率调整
        std::cout << "\n测试费率调整：" << std::endl;
        
        // 调整普通商户费率
        if (normalMerchant.updateFeeRate(0.007, "季度调价")) {
            std::cout << "商户(" << normalMerchant.getMerchantId() 
                      << ")费率调整为: " << normalMerchant.getFeeRate() * 100 
                      << "%" << std::endl;
        }
        
        // 7. 测试结算周期变更
        std::cout << "\n测试结算周期变更：" << std::endl;
        
        // 修改服务商结算周期
        if (isvMerchant.updateSettlementCycle("MONTHLY", "商户申请")) {
            std::cout << "商户(" << isvMerchant.getMerchantId() 
                      << ")结算周期修改为: " << isvMerchant.getSettlementCycle() 
                      << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "错误：" << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 