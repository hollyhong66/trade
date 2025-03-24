#include "alipay_order.h"
#include "alipay_payment.h"
#include "alipay_transaction_manager.h"
#include <iostream>
#include <iomanip>

int main() {
    AlipayOrder order;
    AlipayPayment payment;
    auto& txManager = AlipayTransactionManager::getInstance();
    
    try {
        // 设置订单参数
        order.setOutTradeNo("TEST_ORDER_" + std::to_string(time(nullptr)));
        order.setTotalAmount(9999);  // 99.99元
        order.setSubject("测试商品");
        order.setProductCode("FAST_INSTANT_TRADE_PAY");

        // 设置可选参数
        order.setBody("商品描述信息");
        
        // 设置订单超时时间（30分钟）
        uint64_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        order.setTimeExpire(now + 1800);
        order.setTimeoutExpress(1800);
        
        // 设置商品明细
        std::vector<AlipayGoodsDetail> goods;
        AlipayGoodsDetail item;
        item.goods_id = "GOODS_001";
        item.goods_name = "iPhone 15";
        item.quantity = 1;
        item.price = 9999;  // 99.99元
        item.show_url = "https://example.com/goods/001";
        item.goods_category = "手机数码";
        goods.push_back(item);
        order.setGoodsDetail(goods);
        
        // 设置业务扩展参数
        AlipayExtendParams params;
        params.sys_service_provider_id = "2088511833207846";
        params.hb_fq_num = "3";  // 3期分期
        params.hb_fq_seller_percent = "100";  // 卖家承担100%手续费
        order.setExtendParams(params);
        
        // 开始分布式事务
        std::shared_ptr<AlipayTransaction> transaction;
        if (!txManager.startTransaction(order.getOutTradeNo(), transaction)) {
            throw std::runtime_error("开始事务失败");
        }
        
        // 创建订单
        if (!order.createOrder(*transaction)) {
            transaction->rollbackTransaction();
            throw std::runtime_error("订单创建失败");
        }
        
        // 创建支付记录
        if (!payment.createPayment(order.getOutTradeNo(), *transaction)) {
            transaction->rollbackTransaction();
            throw std::runtime_error("支付记录创建失败");
        }
        
        // 准备事务
        if (!txManager.prepareTransaction(transaction->getXID())) {
            transaction->rollbackTransaction();
            throw std::runtime_error("事务准备失败");
        }
        
        // 提交事务
        if (!txManager.commitTransaction(transaction->getXID())) {
            transaction->rollbackTransaction();
            throw std::runtime_error("事务提交失败");
        }
        
        std::cout << "订单和支付记录创建成功！" << std::endl;
        std::cout << "订单号: " << order.getOutTradeNo() << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "错误：" << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 