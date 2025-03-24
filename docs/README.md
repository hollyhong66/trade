# 支付宝支付系统文档

本文档描述了支付宝支付系统的实现细节，包括：

- [数据库设计](database.md)
- [API 文档](api.md)

## 系统架构

系统分为两个主要模块：
1. 订单管理（AlipayOrder）
2. 支付管理（AlipayPayment）

## 依赖

- C++17 或更高版本
- MySQL 8.0 或更高版本
- MySQL Connector/C++ 8.0

## 文档目录

1. [API 文档](api/api.md)
2. [数据库设计](database/database.md)
3. [实现说明](implementation/implementation.md)
4. [安装和设置](setup/setup.md)

## 项目概述

这是一个用 C++ 实现的支付宝订单模型，提供基本的订单管理功能，包括创建、查询、更新订单等操作。

## 快速开始

详细的安装和使用说明请参考 [安装和设置](setup/setup.md) 文档。 

数据库设计文档： 