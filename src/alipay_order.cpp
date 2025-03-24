#include "alipay_order.h"
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <chrono>

AlipayOrder::AlipayOrder() : conn(nullptr), total_amount_(0) {
    product_code_ = "FAST_INSTANT_TRADE_PAY"; // 默认产品码
}

AlipayOrder::~AlipayOrder() {
    if (conn) {
        mysql_close(conn);
    }
}

// 时间戳转换工具方法实现
std::string AlipayOrder::timestampToString(uint64_t timestamp) {
    time_t time = static_cast<time_t>(timestamp);
    struct tm* timeinfo = localtime(&time);
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    return std::string(buffer);
}

uint64_t AlipayOrder::stringToTimestamp(const std::string& timeStr) {
    struct tm tm = {};
    std::istringstream ss(timeStr);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    if (ss.fail()) {
        throw std::invalid_argument("Invalid time format. Expected: YYYY-MM-DD HH:MM:SS");
    }
    return static_cast<uint64_t>(mktime(&tm));
}

// 金额转换工具方法
std::string AlipayOrder::amountToString(uint64_t amount) {
    std::stringstream ss;
    ss << amount / 100 << "." << std::setw(2) << std::setfill('0') << amount % 100;
    return ss.str();
}

uint64_t AlipayOrder::stringToAmount(const std::string& amountStr) {
    size_t dotPos = amountStr.find('.');
    if (dotPos == std::string::npos) {
        return std::stoull(amountStr) * 100;
    }
    
    std::string intPart = amountStr.substr(0, dotPos);
    std::string decPart = amountStr.substr(dotPos + 1);
    
    if (decPart.length() > 2) {
        decPart = decPart.substr(0, 2);
    } else while (decPart.length() < 2) {
        decPart += "0";
    }
    
    return std::stoull(intPart) * 100 + std::stoull(decPart);
}

bool AlipayOrder::connectDB(const char* host, const char* user, 
                          const char* password, const char* db) {
    conn = mysql_init(nullptr);
    if (!conn) return false;
    
    if (!mysql_real_connect(conn, host, user, password, db, 0, nullptr, 0)) {
        return false;
    }
    
    mysql_set_character_set(conn, "utf8mb4");
    
    // 创建必要的表
    if (!createOrderTable() || !createGoodsTable() || !createExtendParamsTable()) {
        return false;
    }
    
    return true;
}

bool AlipayOrder::createOrder() {
    if (!conn) return false;
    
    mysql_query(conn, "START TRANSACTION");
    
    try {
        // 1. 插入订单基本信息
        std::string query = "INSERT INTO alipay_orders ("
            "out_trade_no, total_amount, subject, product_code, body, "
            "time_expire, timeout_express, store_id, merchant_order_no, create_time"
            ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
            
        MYSQL_STMT* stmt = mysql_stmt_init(conn);
        if (!stmt) throw std::runtime_error("mysql_stmt_init failed");
        
        if (mysql_stmt_prepare(stmt, query.c_str(), query.length())) {
            throw std::runtime_error(mysql_stmt_error(stmt));
        }
        
        // 设置当前时间戳
        create_time_ = std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now());
        
        MYSQL_BIND bind[10];
        memset(bind, 0, sizeof(bind));
        
        // 绑定参数
        bind[0].buffer_type = MYSQL_TYPE_STRING;
        bind[0].buffer = (void*)out_trade_no_.c_str();
        bind[0].buffer_length = out_trade_no_.length();
        
        bind[1].buffer_type = MYSQL_TYPE_LONGLONG;
        bind[1].buffer = &total_amount_;
        bind[1].is_unsigned = true;
        
        bind[2].buffer_type = MYSQL_TYPE_STRING;
        bind[2].buffer = (void*)subject_.c_str();
        bind[2].buffer_length = subject_.length();
        
        bind[3].buffer_type = MYSQL_TYPE_STRING;
        bind[3].buffer = (void*)product_code_.c_str();
        bind[3].buffer_length = product_code_.length();
        
        // 可选参数绑定
        my_bool is_null[6] = {1, 1, 1, 1, 1, 0}; // create_time 不为空
        
        if (body_) {
            bind[4].buffer_type = MYSQL_TYPE_STRING;
            bind[4].buffer = (void*)body_->c_str();
            bind[4].buffer_length = body_->length();
            is_null[0] = 0;
        }
        bind[4].is_null = &is_null[0];
        
        if (time_expire_) {
            bind[5].buffer_type = MYSQL_TYPE_LONGLONG;
            bind[5].buffer = &(*time_expire_);
            bind[5].is_unsigned = true;
            is_null[1] = 0;
        }
        bind[5].is_null = &is_null[1];
        
        // ... 绑定其他可选参数 ...
        
        bind[9].buffer_type = MYSQL_TYPE_LONGLONG;
        bind[9].buffer = &create_time_;
        bind[9].is_unsigned = true;
        bind[9].is_null = &is_null[5];
        
        if (mysql_stmt_bind_param(stmt, bind)) {
            throw std::runtime_error(mysql_stmt_error(stmt));
        }
        
        if (mysql_stmt_execute(stmt)) {
            throw std::runtime_error(mysql_stmt_error(stmt));
        }
        
        mysql_stmt_close(stmt);
        
        // 2. 插入商品明细
        if (!goods_detail_.empty()) {
            query = "INSERT INTO alipay_goods_detail ("
                "out_trade_no, goods_id, goods_name, quantity, price, "
                "alipay_goods_id, show_url, goods_category, categories_tree, body"
                ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
                
            stmt = mysql_stmt_init(conn);
            if (!stmt) throw std::runtime_error("mysql_stmt_init failed");
            
            if (mysql_stmt_prepare(stmt, query.c_str(), query.length())) {
                throw std::runtime_error(mysql_stmt_error(stmt));
            }
            
            MYSQL_BIND goods_bind[10];
            for (const auto& goods : goods_detail_) {
                memset(goods_bind, 0, sizeof(goods_bind));
                // ... 绑定商品参数并执行插入 ...
            }
            
            mysql_stmt_close(stmt);
        }
        
        // 3. 插入扩展参数
        if (extend_params_) {
            query = "INSERT INTO alipay_extend_params ("
                "out_trade_no, sys_service_provider_id, hb_fq_num, "
                "hb_fq_seller_percent, industry_reflux_info, card_type"
                ") VALUES (?, ?, ?, ?, ?, ?)";
                
            // ... 执行扩展参数插入 ...
        }
        
        mysql_query(conn, "COMMIT");
        return true;
    }
    catch (const std::exception& e) {
        mysql_query(conn, "ROLLBACK");
        return false;
    }
}

void AlipayOrder::setOutTradeNo(const std::string& value) {
    if (value.length() > 64) {
        throw std::invalid_argument("商户订单号长度不能超过64位");
    }
    out_trade_no_ = value;
}

void AlipayOrder::setTotalAmount(uint64_t amount) {
    if (amount == 0) {
        throw std::invalid_argument("订单金额不能为0");
    }
    if (amount > 10000000000) { // 限制金额不超过100000元
        throw std::invalid_argument("订单金额超出限制");
    }
    total_amount_ = amount;
}

void AlipayOrder::setSubject(const std::string& value) {
    if (value.length() > 256) {
        throw std::invalid_argument("订单标题长度不能超过256位");
    }
    subject_ = value;
}

void AlipayOrder::setBody(const std::string& value) {
    if (value.length() > 128) {
        throw std::invalid_argument("订单描述长度不能超过128位");
    }
    body_ = value;
}

void AlipayOrder::setTimeExpire(uint64_t timestamp) {
    // 验证时间戳是否合理（例如：不能早于当前时间）
    uint64_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    if (timestamp < now) {
        throw std::invalid_argument("超时时间不能早于当前时间");
    }
    time_expire_ = timestamp;
}

void AlipayOrder::setTimeoutExpress(uint64_t seconds) {
    // 验证超时时间范围（1分钟到15天）
    const uint64_t MIN_TIMEOUT = 60;        // 1分钟
    const uint64_t MAX_TIMEOUT = 1296000;   // 15天
    
    if (seconds < MIN_TIMEOUT || seconds > MAX_TIMEOUT) {
        throw std::invalid_argument("超时时间必须在1分钟到15天之间");
    }
    timeout_express_ = seconds;
}

void AlipayOrder::setProductCode(const std::string& value) {
    product_code_ = value;
}

void AlipayOrder::setGoodsDetail(const std::vector<AlipayGoodsDetail>& goods) {
    // 验证商品金额
    uint64_t totalGoodsAmount = 0;
    for (const auto& item : goods) {
        if (item.price == 0) {
            throw std::invalid_argument("商品单价不能为0");
        }
        if (item.quantity == 0) {
            throw std::invalid_argument("商品数量不能为0");
        }
        totalGoodsAmount += item.price * item.quantity;
    }
    
    // 验证商品总金额是否与订单金额匹配
    if (totalGoodsAmount != total_amount_) {
        throw std::invalid_argument("商品总金额与订单金额不匹配");
    }
    
    goods_detail_ = goods;
}

void AlipayOrder::setExtendParams(const AlipayExtendParams& params) {
    extend_params_ = params;
}

void AlipayOrder::setStoreId(const std::string& value) {
    if (value.length() > 32) {
        throw std::invalid_argument("商户门店编号长度不能超过32位");
    }
    store_id_ = value;
}

void AlipayOrder::setMerchantOrderNo(const std::string& value) {
    if (value.length() > 32) {
        throw std::invalid_argument("商户原始订单号长度不能超过32位");
    }
    merchant_order_no_ = value;
}

void AlipayOrder::setTradeNo(const std::string& value) {
    trade_no_ = value;
}

void AlipayOrder::setTradeStatus(const std::string& value) {
    trade_status_ = value;
}

void AlipayOrder::setPayTime(uint64_t timestamp) {
    // 验证支付时间
    uint64_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    if (timestamp > now) {
        throw std::invalid_argument("支付时间不能晚于当前时间");
    }
    pay_time_ = timestamp;
}

// Getter implementations
std::string AlipayOrder::getOutTradeNo() const { return out_trade_no_; }
uint64_t AlipayOrder::getTotalAmount() const { return total_amount_; }
std::string AlipayOrder::getSubject() const { return subject_; }
std::string AlipayOrder::getBody() const { return body_.value_or(""); }
uint64_t AlipayOrder::getTimeExpire() const { 
    return time_expire_.value_or(0); 
}
uint64_t AlipayOrder::getTimeoutExpress() const { 
    return timeout_express_.value_or(0); 
}
std::string AlipayOrder::getProductCode() const { return product_code_; }
std::vector<AlipayGoodsDetail> AlipayOrder::getGoodsDetail() const { return goods_detail_; }
AlipayExtendParams AlipayOrder::getExtendParams() const { return extend_params_.value_or(AlipayExtendParams{}); }
std::string AlipayOrder::getStoreId() const { return store_id_.value_or(""); }
std::string AlipayOrder::getMerchantOrderNo() const { return merchant_order_no_.value_or(""); }
std::string AlipayOrder::getTradeNo() const { return trade_no_.value_or(""); }
std::string AlipayOrder::getTradeStatus() const { return trade_status_.value_or(""); }
uint64_t AlipayOrder::getPayTime() const { 
    return pay_time_.value_or(0); 
}

bool AlipayOrder::queryOrder(const std::string& outTradeNo) {
    if (!conn) return false;
    
    try {
        // 1. 查询订单基本信息
        std::string query = "SELECT * FROM alipay_orders WHERE out_trade_no = ?";
            
        MYSQL_STMT* stmt = mysql_stmt_init(conn);
        if (!stmt) throw std::runtime_error("mysql_stmt_init failed");
        
        if (mysql_stmt_prepare(stmt, query.c_str(), query.length())) {
            throw std::runtime_error(mysql_stmt_error(stmt));
        }
        
        MYSQL_BIND bind[1];
        memset(bind, 0, sizeof(bind));
        
        bind[0].buffer_type = MYSQL_TYPE_STRING;
        bind[0].buffer = (void*)outTradeNo.c_str();
        bind[0].buffer_length = outTradeNo.length();
        
        if (mysql_stmt_bind_param(stmt, bind)) {
            throw std::runtime_error(mysql_stmt_error(stmt));
        }
        
        if (mysql_stmt_execute(stmt)) {
            throw std::runtime_error(mysql_stmt_error(stmt));
        }
        
        // 绑定结果集
        MYSQL_BIND result[10];
        memset(result, 0, sizeof(result));
        
        // 准备结果缓冲区
        char out_trade_no_buf[65];
        char subject_buf[257];
        char product_code_buf[65];
        char body_buf[129];
        char store_id_buf[33];
        char merchant_order_no_buf[33];
        my_bool is_null[10];
        unsigned long length[10];
        
        // 绑定结果字段
        result[0].buffer_type = MYSQL_TYPE_STRING;
        result[0].buffer = out_trade_no_buf;
        result[0].buffer_length = sizeof(out_trade_no_buf);
        result[0].is_null = &is_null[0];
        result[0].length = &length[0];
        
        result[1].buffer_type = MYSQL_TYPE_LONGLONG;
        result[1].buffer = &total_amount_;
        result[1].is_unsigned = true;
        result[1].is_null = &is_null[1];
        
        // ... 绑定其他结果字段 ...
        
        if (mysql_stmt_bind_result(stmt, result)) {
            throw std::runtime_error(mysql_stmt_error(stmt));
        }
        
        if (mysql_stmt_fetch(stmt)) {
            throw std::runtime_error(mysql_stmt_error(stmt));
        }
        
        // 设置查询结果到对象属性
        out_trade_no_ = std::string(out_trade_no_buf, length[0]);
        subject_ = std::string(subject_buf, length[2]);
        product_code_ = std::string(product_code_buf, length[3]);
        
        if (!is_null[4]) {
            body_ = std::string(body_buf, length[4]);
        }
        
        // ... 设置其他字段 ...
        
        mysql_stmt_close(stmt);
        
        // 2. 查询商品明细
        goods_detail_.clear();
        query = "SELECT * FROM alipay_goods_detail WHERE out_trade_no = ?";
        stmt = mysql_stmt_init(conn);
        
        // ... 执行商品明细查询 ...
        
        // 3. 查询扩展参数
        query = "SELECT * FROM alipay_extend_params WHERE out_trade_no = ?";
        // ... 执行扩展参数查询 ...
        
        return true;
    }
    catch (const std::exception& e) {
        return false;
    }
}

bool AlipayOrder::updateOrderStatus(const std::string& outTradeNo, 
                                  const std::string& tradeNo,
                                  const std::string& status) {
    if (!conn) return false;
    
    try {
        std::string query = "UPDATE alipay_orders SET trade_no = ?, "
            "trade_status = ?, pay_time = IF(? = 'TRADE_SUCCESS', NOW(), pay_time) "
            "WHERE out_trade_no = ?";
            
        MYSQL_STMT* stmt = mysql_stmt_init(conn);
        if (!stmt) throw std::runtime_error("mysql_stmt_init failed");
        
        if (mysql_stmt_prepare(stmt, query.c_str(), query.length())) {
            throw std::runtime_error(mysql_stmt_error(stmt));
        }
        
        // TODO: 完成参数绑定和执行
        
        mysql_stmt_close(stmt);
        return true;
    }
    catch (const std::exception& e) {
        return false;
    }
}

void AlipayOrder::setTransactionId(const std::string& id) {
    transaction_id = id;
}

std::string AlipayOrder::getTransactionId() const {
    return transaction_id;
}

void AlipayOrder::setBuyerId(const std::string& id) {
    buyer_id = id;
}

std::string AlipayOrder::getBuyerId() const {
    return buyer_id;
}

void AlipayOrder::setExchangeRate(double rate) {
    exchange_rate = rate;
}

double AlipayOrder::getExchangeRate() const {
    return exchange_rate;
}

void AlipayOrder::setPricingCurrency(const std::string& currency) {
    pricing_currency = currency;
}

std::string AlipayOrder::getPricingCurrency() const {
    return pricing_currency;
}

void AlipayOrder::setSettlementCurrency(const std::string& currency) {
    settlement_currency = currency;
}

std::string AlipayOrder::getSettlementCurrency() const {
    return settlement_currency;
}

void AlipayOrder::setPaymentCurrency(const std::string& currency) {
    payment_currency = currency;
}

std::string AlipayOrder::getPaymentCurrency() const {
    return payment_currency;
}

void AlipayOrder::setSettlementExchangeRate(double rate) {
    settlement_exchange_rate = rate;
}

double AlipayOrder::getSettlementExchangeRate() const {
    return settlement_exchange_rate;
}

void AlipayOrder::setPaymentExchangeRate(double rate) {
    payment_exchange_rate = rate;
}

double AlipayOrder::getPaymentExchangeRate() const {
    return payment_exchange_rate;
}

void AlipayOrder::setPricingExchangeRate(double rate) {
    pricing_exchange_rate = rate;
}

double AlipayOrder::getPricingExchangeRate() const {
    return pricing_exchange_rate;
}

// 数据库表创建方法
bool AlipayOrder::createOrderTable() {
    if (!conn) return false;
    
    const char* sql = R"SQL(
        CREATE TABLE IF NOT EXISTS alipay_orders (
            out_trade_no VARCHAR(64) PRIMARY KEY,    -- 商户订单号
            total_amount BIGINT UNSIGNED NOT NULL,   -- 订单总金额(分)
            subject VARCHAR(256) NOT NULL,           -- 订单标题
            product_code VARCHAR(64) NOT NULL,       -- 产品码
            body VARCHAR(128),                       -- 订单描述
            time_expire BIGINT UNSIGNED,             -- 绝对超时时间戳
            timeout_express BIGINT UNSIGNED,         -- 相对超时时间
            store_id VARCHAR(32),                    -- 商户门店编号
            merchant_order_no VARCHAR(32),           -- 商户原始订单号
            create_time BIGINT UNSIGNED NOT NULL,    -- 订单创建时间
            INDEX idx_create_time (create_time)
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
    )SQL";
    
    return mysql_query(conn, sql) == 0;
}

// ... 继续支付类实现 ... 