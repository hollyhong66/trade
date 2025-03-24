# 支付宝订单系统数据库设计文档

本文档详细说明支付宝订单系统的数据库设计，包括表结构、索引、关键SQL等内容。

## 数据库环境

- 数据库类型: MySQL 5.7+
- 字符集: UTF-8
- 排序规则: utf8_general_ci

## 数据库配置

### 基本配置


## 目录
1. [数据库概述](#数据库概述)
2. [表结构设计](#表结构设计)
3. [索引设计](#索引设计)
4. [关键SQL语句](#关键sql语句)
5. [数据库维护](#数据库维护)

## 数据库概述

### 系统说明
本数据库设计用于支付宝订单系统，主要处理订单的创建、查询、更新等操作。

### 设计原则
1. 数据完整性：确保订单数据的准确性和一致性
2. 性能优化：针对高频查询操作进行优化
3. 可扩展性：预留字段支持未来功能扩展
4. 安全性：敏感数据加密存储

## 表结构设计

### 1. 订单主表 (alipay_orders)

#### 表结构

| 字段名 | 类型 | 是否为空 | 默认值 | 说明 |
|--------|------|----------|--------|------|
| order_id | VARCHAR(32) | NO | - | 主键，订单号 |
| trade_no | VARCHAR(64) | YES | NULL | 支付宝交易号 |
| user_id | VARCHAR(32) | NO | - | 用户ID |
| amount | DECIMAL(10,2) | NO | 0.00 | 订单金额 |
| status | VARCHAR(20) | NO | 'CREATED' | 订单状态 |
| create_time | TIMESTAMP | NO | CURRENT_TIMESTAMP | 创建时间 |
| pay_time | TIMESTAMP | YES | NULL | 支付时间 |
| update_time | TIMESTAMP | NO | CURRENT_TIMESTAMP | 更新时间 |
| remark | VARCHAR(255) | YES | NULL | 订单备注 |

#### 字段说明

1. **order_id**
   - 格式：ORDER + 时间戳 + 随机数
   - 示例：ORDER202403151234567890

2. **trade_no**
   - 支付宝返回的交易号
   - 支付成功后必填

3. **status**
   - CREATED: 已创建
   - PAID: 已支付
   - REFUNDED: 已退款
   - CLOSED: 已关闭

### 2. 订单历史表 (alipay_order_history)

#### 表结构

| 字段名 | 类型 | 是否为空 | 默认值 | 说明 |
|--------|------|----------|--------|------|
| id | BIGINT | NO | AUTO_INCREMENT | 主键 |
| order_id | VARCHAR(32) | NO | - | 订单号 |
| old_status | VARCHAR(20) | NO | - | 原状态 |
| new_status | VARCHAR(20) | NO | - | 新状态 |
| change_time | TIMESTAMP | NO | CURRENT_TIMESTAMP | 变更时间 |
| operator | VARCHAR(32) | YES | NULL | 操作人 |
| remark | VARCHAR(255) | YES | NULL | 变更备注 |

## 索引设计

### alipay_orders 表索引

1. 主键索引
```sql
CREATE UNIQUE INDEX idx_order_id ON alipay_orders (order_id);
```

2. 普通索引
```sql
CREATE INDEX idx_user_id ON alipay_orders (user_id);
```

3. 组合索引
```sql
CREATE INDEX idx_order_status ON alipay_orders (status);
```

## 关键SQL语句

### 1. 创建订单
```sql
INSERT INTO alipay_orders (order_id, trade_no, user_id, amount, status)
VALUES ('ORDER202403151234567890', '1234567890', 'user123', 99.99, 'CREATED');
```

### 2. 查询订单
```sql
-- 按订单号查询
SELECT * FROM alipay_orders WHERE order_id = 'ORDER202403151234567890';

-- 查询用户最近订单
SELECT * FROM alipay_orders 
WHERE user_id = 'user123' 
ORDER BY create_time DESC 
LIMIT 10;

-- 查询待支付订单
SELECT * FROM alipay_orders 
WHERE status = 'CREATED' 
AND create_time > DATE_SUB(NOW(), INTERVAL 24 HOUR);
```

### 3. 更新订单状态
```sql
-- 更新订单状态
UPDATE alipay_orders 
SET status = 'PAID', 
    pay_time = CURRENT_TIMESTAMP,
    trade_no = '2024031587654321',
    update_time = CURRENT_TIMESTAMP 
WHERE order_id = 'ORDER202403151234567890';

-- 记录状态变更历史
INSERT INTO alipay_order_history 
(order_id, old_status, new_status, operator, remark)
VALUES 
('ORDER202403151234567890', 'CREATED', 'PAID', 'SYSTEM', '支付成功');
```

### 4. 统计查询
```sql
-- 按日统计订单量
SELECT 
    DATE(create_time) as order_date,
    COUNT(*) as total_orders,
    SUM(CASE WHEN status = 'PAID' THEN 1 ELSE 0 END) as paid_orders,
    SUM(CASE WHEN status = 'PAID' THEN amount ELSE 0 END) as total_amount
FROM alipay_orders
GROUP BY DATE(create_time)
ORDER BY order_date DESC;
```

## 数据库维护

### 1. 数据备份策略
- 每日凌晨2点进行全量备份
- 每小时进行增量备份
- 备份文件保留30天

### 2. 性能优化建议
1. **定期更新统计信息**
```sql
ANALYZE TABLE alipay_orders, alipay_order_history;
```

2. **清理历史数据**
- 订单数据保留3年
- 历史记录保留1年
```sql
DELETE FROM alipay_order_history 
WHERE change_time < DATE_SUB(NOW(), INTERVAL 1 YEAR);
```

### 3. 监控指标
1. 关键指标
   - 订单表数据量
   - 查询响应时间
   - 索引使用率
   - 慢查询数量

2. 告警阈值
   - 查询响应时间 > 1秒
   - 表空间使用率 > 80%
   - 慢查询数量 > 100/小时

## 安全措施

### 1. 数据加密
- 敏感信息使用AES-256加密存储
- 传输过程使用TLS 1.2+加密

### 2. 访问控制
- 按角色分配最小权限
- 定期审计数据库访问日志
- 禁止直接访问生产数据库

### 3. 数据备份
- 异地多活备份
- 定期进行恢复演练
- 备份数据加密存储


