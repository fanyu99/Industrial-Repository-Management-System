-- 版本: 1
-- 前置条件: 数据库已创建(wms)且包含版本1中的表
-- 变更: 用户、分类、单位、物资、入出库单与明细
-- 回滚: 001_initial_rollback.sql
-- 验证: 001_initial_verify.sql
DROP TABLE IF EXISTS audit_logs;

DROP TABLE IF EXISTS stock_balance;

DROP TABLE IF EXISTS stock_movements;

DROP TABLE IF EXISTS outbound_details;

DROP TABLE IF EXISTS outbound_orders;

DROP TABLE IF EXISTS inbound_details;

DROP TABLE IF EXISTS inbound_orders;

DROP TABLE IF EXISTS products;

DROP TABLE IF EXISTS units;

DROP TABLE IF EXISTS categories;

DROP TABLE IF EXISTS warehouses;

DROP TABLE IF EXISTS users;