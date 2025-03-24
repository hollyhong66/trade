# 数据库设计

## 订单表 (alipay_orders)

| 字段名 | 类型 | 说明 | 约束 |
|--------|------|------|------|
| out_trade_no | VARCHAR(64) | 商户订单号 | PRIMARY KEY |
| total_amount | BIGINT UNSIGNED | 订单总金额(分) | NOT NULL |
| subject | VARCHAR(256) | 订单标题 | NOT NULL |
| product_code | VARCHAR(64) | 产品码 | NOT NULL |
| body | VARCHAR(128) | 订单描述 | NULL |
| time_expire | BIGINT UNSIGNED | 绝对超时时间戳 | NULL |
| timeout_express | BIGINT UNSIGNED | 相对超时时间 | NULL |
| store_id | VARCHAR(32) | 商户门店编号 | NULL |
| merchant_order_no | VARCHAR(32) | 商户原始订单号 | NULL |
| create_time | BIGINT UNSIGNED | 订单创建时间 | NOT NULL |

索引：
- PRIMARY KEY (out_trade_no)
- INDEX idx_create_time (create_time)

## 支付表 (alipay_payments)

| 字段名 | 类型 | 说明 | 约束 |
|--------|------|------|------|
| out_trade_no | VARCHAR(64) | 商户订单号 | PRIMARY KEY |
| trade_no | VARCHAR(64) | 支付宝交易号 | NULL |
| trade_status | VARCHAR(32) | 交易状态 | NOT NULL |
| pay_time | BIGINT UNSIGNED | 支付时间戳 | NULL |
| update_time | BIGINT UNSIGNED | 状态更新时间 | NOT NULL |

索引：
- PRIMARY KEY (out_trade_no)
- FOREIGN KEY (out_trade_no) REFERENCES alipay_orders(out_trade_no)
- INDEX idx_trade_no (trade_no)
- INDEX idx_trade_status (trade_status)
- INDEX idx_update_time (update_time)

## 商品明细表 (alipay_goods_detail)

| 字段名 | 类型 | 说明 | 约束 |
|--------|------|------|------|
| id | BIGINT UNSIGNED | 自增ID | PRIMARY KEY |
| out_trade_no | VARCHAR(64) | 商户订单号 | NOT NULL |
| goods_id | VARCHAR(32) | 商品编号 | NOT NULL |
| goods_name | VARCHAR(256) | 商品名称 | NOT NULL |
| quantity | INT UNSIGNED | 商品数量 | NOT NULL |
| price | BIGINT UNSIGNED | 商品单价(分) | NOT NULL |
| alipay_goods_id | VARCHAR(32) | 支付宝统一商品编号 | NULL |
| show_url | VARCHAR(400) | 商品展示地址 | NULL |
| goods_category | VARCHAR(24) | 商品类目 | NULL |
| categories_tree | VARCHAR(128) | 商品类目树 | NULL |
| body | VARCHAR(1000) | 商品描述 | NULL |

索引：
- PRIMARY KEY (id)
- FOREIGN KEY (out_trade_no) REFERENCES alipay_orders(out_trade_no)
- INDEX idx_out_trade_no (out_trade_no)
- INDEX idx_goods_id (goods_id)

## 扩展参数表 (alipay_extend_params)

| 字段名 | 类型 | 说明 | 约束 |
|--------|------|------|------|
| out_trade_no | VARCHAR(64) | 商户订单号 | PRIMARY KEY |
| sys_service_provider_id | VARCHAR(64) | 系统商编号 | NULL |
| hb_fq_num | VARCHAR(5) | 花呗分期数 | NULL |
| hb_fq_seller_percent | VARCHAR(3) | 卖家承担收费比例 | NULL |
| industry_reflux_info | VARCHAR(2048) | 行业数据回流信息 | NULL |
| card_type | VARCHAR(32) | 卡类型 | NULL |

索引：
- PRIMARY KEY (out_trade_no)
- FOREIGN KEY (out_trade_no) REFERENCES alipay_orders(out_trade_no) 