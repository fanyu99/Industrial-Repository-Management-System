-- 版本: 1
-- 前置条件: 已执行 001_initial.sql
-- 目标: 验证版本 1 的表、引擎、字符集、唯一约束、外键与关键 CHECK 约束
-- 回滚: 001_initial_rollback.sql

USE wms;

-- 1. 验证 12 张基础表是否全部存在。
SELECT
    'tables_exist' AS check_name,
    expected.table_name,
    IF(actual.table_name IS NULL, 'FAIL', 'PASS') AS result
FROM (
    SELECT 'users' AS table_name
    UNION ALL SELECT 'warehouses'
    UNION ALL SELECT 'categories'
    UNION ALL SELECT 'units'
    UNION ALL SELECT 'products'
    UNION ALL SELECT 'inbound_orders'
    UNION ALL SELECT 'inbound_details'
    UNION ALL SELECT 'outbound_orders'
    UNION ALL SELECT 'outbound_details'
    UNION ALL SELECT 'stock_movements'
    UNION ALL SELECT 'stock_balance'
    UNION ALL SELECT 'audit_logs'
) AS expected
LEFT JOIN information_schema.tables AS actual
    ON actual.table_schema = DATABASE()
    AND actual.table_name = expected.table_name
ORDER BY expected.table_name;

-- 2. 验证表数量，防止漏建或多建非预期表。
SELECT
    'table_count' AS check_name,
    12 AS expected_count,
    COUNT(*) AS actual_count,
    IF(COUNT(*) = 12, 'PASS', 'FAIL') AS result
FROM information_schema.tables
WHERE table_schema = DATABASE();

-- 3. 验证所有表使用 InnoDB 和 utf8mb4。
SELECT
    'engine_and_charset' AS check_name,
    table_name,
    engine,
    table_collation,
    IF(engine = 'InnoDB' AND table_collation LIKE 'utf8mb4%', 'PASS', 'FAIL') AS result
FROM information_schema.tables
WHERE table_schema = DATABASE()
ORDER BY table_name;

-- 4. 验证关键唯一约束。注意：UNIQUE 会在 MySQL 中表现为唯一索引。
SELECT
    'unique_indexes' AS check_name,
    expected.table_name,
    expected.column_list AS expected_columns,
    IF(actual.index_name IS NULL, 'FAIL', 'PASS') AS result
FROM (
    SELECT 'users' AS table_name, 'username' AS column_list
    UNION ALL SELECT 'warehouses', 'code'
    UNION ALL SELECT 'categories', 'code'
    UNION ALL SELECT 'units', 'code'
    UNION ALL SELECT 'products', 'code'
    UNION ALL SELECT 'inbound_orders', 'order_no'
    UNION ALL SELECT 'outbound_orders', 'order_no'
    UNION ALL SELECT 'stock_movements', 'movement_no'
    UNION ALL SELECT 'stock_movements', 'source_type,source_id,source_line_id,movement_role'
    UNION ALL SELECT 'stock_balance', 'product_id,warehouse_id'
) AS expected
LEFT JOIN (
    SELECT
        table_name,
        index_name,
        GROUP_CONCAT(column_name ORDER BY seq_in_index SEPARATOR ',') AS column_list
    FROM information_schema.statistics
    WHERE table_schema = DATABASE()
        AND non_unique = 0
    GROUP BY table_name, index_name
) AS actual
    ON actual.table_name = expected.table_name
    AND actual.column_list = expected.column_list
ORDER BY expected.table_name, expected.column_list;

-- 5. 验证关键外键是否存在，并确认引用目标表正确。
SELECT
    'foreign_keys' AS check_name,
    expected.table_name,
    expected.column_name,
    expected.referenced_table_name,
    expected.referenced_column_name,
    IF(actual.constraint_name IS NULL, 'FAIL', 'PASS') AS result
FROM (
    SELECT 'products' AS table_name, 'category_id' AS column_name, 'categories' AS referenced_table_name, 'id' AS referenced_column_name
    UNION ALL SELECT 'products', 'unit_id', 'units', 'id'
    UNION ALL SELECT 'inbound_orders', 'warehouse_id', 'warehouses', 'id'
    UNION ALL SELECT 'inbound_orders', 'operator_id', 'users', 'id'
    UNION ALL SELECT 'inbound_details', 'order_id', 'inbound_orders', 'id'
    UNION ALL SELECT 'inbound_details', 'product_id', 'products', 'id'
    UNION ALL SELECT 'outbound_orders', 'warehouse_id', 'warehouses', 'id'
    UNION ALL SELECT 'outbound_orders', 'operator_id', 'users', 'id'
    UNION ALL SELECT 'outbound_details', 'order_id', 'outbound_orders', 'id'
    UNION ALL SELECT 'outbound_details', 'product_id', 'products', 'id'
    UNION ALL SELECT 'stock_movements', 'product_id', 'products', 'id'
    UNION ALL SELECT 'stock_movements', 'warehouse_id', 'warehouses', 'id'
    UNION ALL SELECT 'stock_movements', 'operator_id', 'users', 'id'
    UNION ALL SELECT 'stock_balance', 'product_id', 'products', 'id'
    UNION ALL SELECT 'stock_balance', 'warehouse_id', 'warehouses', 'id'
    UNION ALL SELECT 'audit_logs', 'operator_id', 'users', 'id'
) AS expected
LEFT JOIN information_schema.key_column_usage AS actual
    ON actual.table_schema = DATABASE()
    AND actual.table_name = expected.table_name
    AND actual.column_name = expected.column_name
    AND actual.referenced_table_name = expected.referenced_table_name
    AND actual.referenced_column_name = expected.referenced_column_name
ORDER BY expected.table_name, expected.column_name;

-- 6. 验证每张表的关键 CHECK 约束数量。
-- MySQL 会自动生成未命名 CHECK 的约束名，所以这里不依赖具体名称。
SELECT
    'check_constraints' AS check_name,
    expected.table_name,
    expected.min_check_count,
    COUNT(actual.constraint_name) AS actual_check_count,
    IF(COUNT(actual.constraint_name) >= expected.min_check_count, 'PASS', 'FAIL') AS result
FROM (
    SELECT 'users' AS table_name, 2 AS min_check_count
    UNION ALL SELECT 'warehouses', 1
    UNION ALL SELECT 'categories', 1
    UNION ALL SELECT 'units', 1
    UNION ALL SELECT 'products', 2
    UNION ALL SELECT 'inbound_details', 2
    UNION ALL SELECT 'outbound_details', 2
    UNION ALL SELECT 'stock_movements', 1
    UNION ALL SELECT 'stock_balance', 1
) AS expected
LEFT JOIN information_schema.table_constraints AS actual
    ON actual.constraint_schema = DATABASE()
    AND actual.table_name = expected.table_name
    AND actual.constraint_type = 'CHECK'
GROUP BY expected.table_name, expected.min_check_count
ORDER BY expected.table_name;

-- 7. 汇总关键对象数量，便于快速查看版本 1 的整体状态。
SELECT
    'summary' AS check_name,
    (SELECT COUNT(*) FROM information_schema.tables WHERE table_schema = DATABASE()) AS table_count,
    (SELECT COUNT(*) FROM information_schema.table_constraints WHERE constraint_schema = DATABASE() AND constraint_type = 'FOREIGN KEY') AS foreign_key_count,
    (SELECT COUNT(*) FROM information_schema.table_constraints WHERE constraint_schema = DATABASE() AND constraint_type = 'UNIQUE') AS unique_constraint_count,
    (SELECT COUNT(*) FROM information_schema.table_constraints WHERE constraint_schema = DATABASE() AND constraint_type = 'CHECK') AS check_constraint_count;
