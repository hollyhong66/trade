#include "alipay_order.h"
#include "alipay_payment.h"
#include <iostream>
#include <iomanip>

int main() {
    AlipayOrder order;
    AlipayPayment payment;
    
    // 连接数据库
    if (!order.connectDB("localhost", "username", "password", "dbname")) {
        std::cerr << "订单数据库连接失败！" << std::endl;
        return 1;
    }
    
    if (!payment.connectDB("localhost", "username", "password", "dbname")) {
        std::cerr << "支付数据库连接失败！" << std::endl;
        return 1;
    }
    
    try {
        // 1. 创建订单
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
        
        // 创建订单
        if (order.createOrder()) {
            std::cout << "订单创建成功！" << std::endl;
            std::cout << "订单号: " << order.getOutTradeNo() << std::endl;
            std::cout << "金额: " << AlipayOrder::amountToString(order.getTotalAmount()) << "元" << std::endl;
            
            // 2. 创建支付记录
            if (payment.createPayment(order.getOutTradeNo())) {
                std::cout << "支付记录创建成功！" << std::endl;
            } else {
                std::cerr << "支付记录创建失败！" << std::endl;
                return 1;
            }
        } else {
            std::cerr << "订单创建失败！" << std::endl;
            return 1;
        }
        
        // 3. 查询订单和支付状态
        if (order.queryOrder(order.getOutTradeNo()) && 
            payment.queryPayment(order.getOutTradeNo())) {
            std::cout << "\n订单信息：" << std::endl;
            std::cout << "订单号: " << order.getOutTradeNo() << std::endl;
            std::cout << "商品名称: " << order.getSubject() << std::endl;
            std::cout << "订单金额: " << AlipayOrder::amountToString(order.getTotalAmount()) << "元" << std::endl;
            std::cout << "支付状态: " << payment.getTradeStatus() << std::endl;
            
            // 显示商品信息
            const auto& goods_detail = order.getGoodsDetail();
            if (!goods_detail.empty()) {
                std::cout << "\n商品明细：" << std::endl;
                for (const auto& item : goods_detail) {
                    std::cout << "商品名称: " << item.goods_name << std::endl;
                    std::cout << "商品数量: " << item.quantity << std::endl;
                    std::cout << "商品单价: " << AlipayOrder::amountToString(item.price) << "元" << std::endl;
                }
            }
            
            // 显示超时信息
            if (auto expire_time = order.getTimeExpire()) {
                std::cout << "\n超时时间: " << AlipayOrder::timestampToString(expire_time) << std::endl;
            }
        }
        
        // 4. 模拟支付成功
        if (payment.updatePaymentStatus(
            order.getOutTradeNo(),
            "2024032022001476751412341234",  // 支付宝交易号
            AlipayPayment::TRADE_STATUS_TRADE_SUCCESS
        )) {
            std::cout << "\n支付状态更新成功！" << std::endl;
            
            // 再次查询确认状态
            if (payment.queryPayment(order.getOutTradeNo())) {
                std::cout << "当前状态: " << payment.getTradeStatus() << std::endl;
                std::cout << "支付时间: " << AlipayPayment::timestampToString(payment.getPayTime()) << std::endl;
                std::cout << "支付宝交易号: " << payment.getTradeNo() << std::endl;
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "错误：" << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 