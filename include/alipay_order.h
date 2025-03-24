#pragma once

#include <string>
#include <vector>
#include <optional>
#include <cstdint>
#include <mysql/mysql.h>

// 商品明细信息
struct AlipayGoodsDetail {
    std::string goods_id;        // 必选 - 商品的编号(32)
    std::string goods_name;      // 必选 - 商品名称(256)
    uint32_t quantity;           // 必选 - 商品数量
    uint64_t price;              // 必选 - 商品单价，单位：分
    std::optional<std::string> alipay_goods_id; // 可选 - 支付宝定义的统一商品编号(32)
    std::optional<std::string> show_url;        // 可选 - 商品的展示地址(400)
    std::optional<std::string> goods_category;  // 可选 - 商品类目(24)
    std::optional<std::string> categories_tree; // 可选 - 商品类目树(128)
    std::optional<std::string> body;            // 可选 - 商品描述信息(1000)
};

// 业务扩展参数
struct AlipayExtendParams {
    std::optional<std::string> sys_service_provider_id; // 可选 - 系统商编号(64)
    std::optional<std::string> hb_fq_num;              // 可选 - 花呗分期数(5)
    std::optional<std::string> hb_fq_seller_percent;   // 可选 - 卖家承担收费比例(3)
    std::optional<std::string> industry_reflux_info;   // 可选 - 行业数据回流信息(2048)
    std::optional<std::string> card_type;              // 可选 - 卡类型(32)
};

// 支付宝订单模型
class AlipayOrder {
public:
    AlipayOrder();
    ~AlipayOrder();

    // 数据库连接
    bool connectDB(const char* host, const char* user, 
                  const char* password, const char* db);

    // 订单操作
    bool createOrder();
    bool queryOrder(const std::string& outTradeNo);

    // 必填参数设置
    void setOutTradeNo(const std::string& value);    // 商户订单号(64)
    void setTotalAmount(uint64_t amount);            // 订单总金额(分)
    void setSubject(const std::string& value);       // 订单标题(256)
    void setProductCode(const std::string& value = "FAST_INSTANT_TRADE_PAY"); // 产品码(64)

    // 可选参数设置
    void setBody(const std::string& value);          // 订单描述(128)
    void setTimeExpire(uint64_t timestamp);          // 绝对超时时间戳(秒)
    void setTimeoutExpress(uint64_t seconds);        // 相对超时时间(秒)
    void setGoodsDetail(const std::vector<AlipayGoodsDetail>& goods); // 商品明细
    void setExtendParams(const AlipayExtendParams& params); // 业务扩展参数
    void setStoreId(const std::string& value);       // 商户门店编号(32)
    void setMerchantOrderNo(const std::string& value); // 商户原始订单号(32)

    // Getters
    std::string getOutTradeNo() const;
    uint64_t getTotalAmount() const;
    std::string getSubject() const;
    std::string getProductCode() const;
    std::string getBody() const;
    uint64_t getTimeExpire() const;
    uint64_t getTimeoutExpress() const;
    std::vector<AlipayGoodsDetail> getGoodsDetail() const;
    AlipayExtendParams getExtendParams() const;
    std::string getStoreId() const;
    std::string getMerchantOrderNo() const;
    uint64_t getCreateTime() const;

    // 工具方法
    static std::string amountToString(uint64_t amount);      // 分转元（字符串）
    static uint64_t stringToAmount(const std::string& amountStr); // 元（字符串）转分
    static std::string timestampToString(uint64_t timestamp);     // 时间戳转字符串
    static uint64_t stringToTimestamp(const std::string& timeStr); // 字符串转时间戳

private:
    MYSQL* conn;

    // 订单基本信息
    std::string out_trade_no_;       // 商户订单号
    uint64_t total_amount_;          // 订单总金额(分)
    std::string subject_;            // 订单标题
    std::string product_code_;       // 产品码
    std::optional<std::string> body_;              // 订单描述
    std::optional<uint64_t> time_expire_;          // 绝对超时时间戳
    std::optional<uint64_t> timeout_express_;      // 相对超时时间
    std::optional<std::string> store_id_;          // 商户门店编号
    std::optional<std::string> merchant_order_no_; // 商户原始订单号
    uint64_t create_time_;           // 订单创建时间

    // 商品信息和扩展参数
    std::vector<AlipayGoodsDetail> goods_detail_; // 商品明细
    std::optional<AlipayExtendParams> extend_params_; // 业务扩展参数

    // 数据库操作辅助方法
    bool createOrderTable();    // 创建订单表
    bool createGoodsTable();   // 创建商品明细表
    bool createExtendParamsTable(); // 创建扩展参数表
}; 