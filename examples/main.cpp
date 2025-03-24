#include "alipay_order.h"
#include <iostream>

int main() {
    AlipayOrder order;
    
    // 连接数据库
    if (!order.connectDB("localhost", "username", "password", "dbname")) {
        std::cout << "数据库连接失败！" << std::endl;
        return 1;
    }
    
    // 创建订单
    if (order.createOrder("user123", 99.99)) {
        std::cout << "订单创建成功！订单号: " << order.getOrderId() << std::endl;
    }
    
    // 查询订单
    if (order.queryOrder(order.getOrderId())) {
        std::cout << "订单状态: " << order.getStatus() << std::endl;
        std::cout << "订单金额: " << order.getAmount() << std::endl;
    }
    
    // 更新订单状态
    if (order.updateOrderStatus(order.getOrderId(), "PAID")) {
        std::cout << "订单状态更新成功！" << std::endl;
    }
    
    return 0;
} 