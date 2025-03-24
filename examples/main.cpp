#include "alipay_order.h"
#include "alipay_payment.h"
#include "alipay_merchant.h"
#include "alipay_settlement.h"
#include "alipay_transaction_manager.h"
#include <iostream>
#include <iomanip>

// 辅助函数：格式化金额显示
std::string formatAmount(uint64_t amount) {
    std::stringstream ss;
    ss << amount / 100 << "." << std::setw(2) << std::setfill('0') << amount % 100;
    return ss.str();
}

int main() {
    try {
        // 1. 初始化各个组件
        AlipayMerchant merchant;
        AlipayOrder order;
        AlipayPayment payment;
        AlipaySettlement settlement;
        auto& txManager = AlipayTransactionManager::getInstance();
        
        // 2. 连接数据库
        const char* host = "localhost";
        const char* user = "username";
        const char* password = "password";
        const char* db = "alipay_db";
        
        if (!merchant.connectDB(host, user, password, db) ||
            !order.connectDB(host, user, password, db) ||
            !payment.connectDB(host, user, password, db) ||
            !settlement.connectDB(host, user, password, db)) {
            throw std::runtime_error("数据库连接失败");
        }
        
        // 3. 创建或查询商户信息
        merchant.setMerchantId("MERCHANT_001");
        merchant.setMerchantName("测试商户");
        merchant.setMerchantType("INDIVIDUAL");
        merchant.setContactInfo(
            "张三",                    // 联系人
            "13800138000",            // 电话
            "zhangsan@example.com"    // 邮箱
        );
        merchant.setBankAccount(
            "张三",                    // 账户名
            "6222021234567890123",    // 账号
            "中国工商银行",            // 银行名称
            "北京市海淀支行"           // 支行
        );
        merchant.setSettlementInfo(
            "T+1",                    // 结算类型
            "DAILY",                  // 结算周期
            0.006                     // 费率 0.6%
        );
        
        if (!merchant.createMerchant()) {
            throw std::runtime_error("商户创建失败");
        }
        
        // 4. 创建订单
        std::string orderNo = "TEST_ORDER_" + std::to_string(time(nullptr));
        order.setOutTradeNo(orderNo);
        order.setMerchantId(merchant.getMerchantId());
        order.setTotalAmount(9999);  // 99.99元
        order.setSubject("测试商品");
        order.setProductCode("FAST_INSTANT_TRADE_PAY");
        order.setBody("商品描述信息");
        
        // 设置订单超时时间（30分钟）
        uint64_t now = std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now());
        order.setTimeExpire(now + 1800);
        
        // 5. 开始分布式事务
        std::shared_ptr<AlipayTransaction> transaction;
        if (!txManager.startTransaction(orderNo, transaction)) {
            throw std::runtime_error("事务启动失败");
        }
        
        // 6. 创建订单和支付记录
        if (!order.createOrder(*transaction)) {
            transaction->rollbackTransaction();
            throw std::runtime_error("订单创建失败");
        }
        
        if (!payment.createPayment(orderNo, *transaction)) {
            transaction->rollbackTransaction();
            throw std::runtime_error("支付记录创建失败");
        }
        
        // 7. 提交事务
        if (!txManager.prepareTransaction(transaction->getXID())) {
            transaction->rollbackTransaction();
            throw std::runtime_error("事务准备失败");
        }
        
        if (!txManager.commitTransaction(transaction->getXID())) {
            transaction->rollbackTransaction();
            throw std::runtime_error("事务提交失败");
        }
        
        std::cout << "订单创建成功！\n" 
                  << "订单号: " << order.getOutTradeNo() << "\n"
                  << "商户号: " << merchant.getMerchantId() << "\n"
                  << "金额: " << formatAmount(order.getTotalAmount()) << "元\n";
        
        // 8. 模拟支付成功
        if (payment.updatePaymentStatus(
            orderNo,
            "2024032022001476751412341234",  // 支付宝交易号
            AlipayPayment::TRADE_STATUS_TRADE_SUCCESS
        )) {
            std::cout << "\n支付成功！\n"
                      << "支付宝交易号: " << payment.getTradeNo() << "\n"
                      << "支付时间: " << AlipayPayment::timestampToString(
                          payment.getPayTime()) << "\n";
            
            // 9. 创建结算记录
            if (settlement.createSettlement(orderNo, merchant.getMerchantId())) {
                std::cout << "\n结算单创建成功！\n"
                          << "结算单号: " << settlement.getSettlementId() << "\n"
                          << "结算金额: " << formatAmount(
                              settlement.getSettlementAmount()) << "元\n"
                          << "手续费: " << formatAmount(
                              settlement.getFeeAmount()) << "元\n"
                          << "结算账号: " << settlement.getBankAccountNo() << "\n"
                          << "开户行: " << settlement.getBankName() << "\n";
                
                // 10. 模拟结算成功
                if (settlement.updateSettlementStatus(
                    AlipaySettlement::STATUS_SUCCESS
                )) {
                    std::cout << "\n结算完成！\n"
                              << "结算时间: " << AlipayPayment::timestampToString(
                                  settlement.getSettleTime()) << "\n";
                }
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "错误：" << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 