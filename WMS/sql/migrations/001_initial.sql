-- 版本: 1
-- 前置条件: 空库
-- 变更: 用户、分类、单位、物资、入出库单与明细
-- 回滚: 001_initial_rollback.sql
-- 验证: 001_initial_verify.sql

/*
1.users  用户
2.warehouses  仓库
3.categories  分类
4.units  单位
5.products  物资
6.inbound_orders  入库单
7.inbound_order_details  入库单明细
8.outbound_orders  出库单
9.outbound_order_details  出库单明细
10.stock_movements  库存移动
11.stock_balance  库存余额
12.audit_logs  审计日志
*/




CREATE DATABASE IF NOT EXISTS wms DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;

USE wms;

-- 用户表（PBKDF2 参数化密码存储）
/*
id  username  password_hash  
password_salt  password_algorithm  password_iterations  
real_name  role  is_active  
created_at  updated_at
*/
CREATE TABLE users (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY COMMENT '用户ID',
    username VARCHAR(50) NOT NULL UNIQUE COMMENT '用户名',
    password_hash VARBINARY(255) NOT NULL COMMENT 'PBKDF2 Hash',
    password_salt VARBINARY(64) NOT NULL CHECK (LENGTH(password_salt) >= 16) COMMENT 'CSPRNG 随机盐, >=16 字节',
    password_algorithm VARCHAR(20) NOT NULL DEFAULT 'pbkdf2_hmac_sha256',
    password_iterations INT UNSIGNED NOT NULL COMMENT 'PBKDF2 迭代次数',
    real_name VARCHAR(50) NOT NULL COMMENT '真实姓名',
    role ENUM(
        'admin',
        'manager',
        'operator'
    ) NOT NULL DEFAULT 'operator',
    is_active TINYINT(1) NOT NULL DEFAULT 1 CHECK (is_active IN (0, 1)) COMMENT '状态',
    created_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3) COMMENT '创建时间',
    updated_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3) ON UPDATE CURRENT_TIMESTAMP(3) COMMENT '更新时间',
    INDEX idx_users_active (is_active) -- 状态
) ENGINE = InnoDB DEFAULT CHARSET = utf8mb4 COMMENT '用户表';

-- 仓库表
/*
id  code name is_active
*/
CREATE TABLE warehouses (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY COMMENT '仓库ID',
    code VARCHAR(30) NOT NULL UNIQUE COMMENT '仓库编码',
    name VARCHAR(50) NOT NULL COMMENT '仓库名称',
    is_active TINYINT(1) NOT NULL DEFAULT 1 CHECK (is_active IN (0, 1)) COMMENT '状态',
    created_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3) COMMENT '创建时间',
    updated_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3) ON UPDATE CURRENT_TIMESTAMP(3) COMMENT '更新时间'
) ENGINE = InnoDB DEFAULT CHARSET = utf8mb4 COMMENT '仓库表';

-- 主数据：分类 / 单位 / 物资（停用而非删除）
-- 分类表
/*
id  code  name  is_active  created_at  updated_at
*/
CREATE TABLE categories (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY COMMENT '分类ID',
    code VARCHAR(30) NOT NULL UNIQUE COMMENT '分类编码',
    name VARCHAR(50) NOT NULL COMMENT '分类名称',
    is_active TINYINT(1) NOT NULL DEFAULT 1 CHECK (is_active IN (0, 1)) COMMENT '状态',
    created_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3) COMMENT '创建时间',
    updated_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3) ON UPDATE CURRENT_TIMESTAMP(3) COMMENT '更新时间'
) ENGINE = InnoDB DEFAULT CHARSET = utf8mb4 COMMENT '分类表';

-- 单位表
/*
id  code  name  is_active  created_at  updated_at
*/
CREATE TABLE units (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY COMMENT '单位ID',
    code VARCHAR(20) NOT NULL UNIQUE COMMENT '单位编码',
    name VARCHAR(20) NOT NULL COMMENT '单位名称',
    is_active TINYINT(1) NOT NULL DEFAULT 1 CHECK (is_active IN (0, 1)) COMMENT '状态',
    created_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3) COMMENT '创建时间',
    updated_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3) ON UPDATE CURRENT_TIMESTAMP(3) COMMENT '更新时间'
) ENGINE = InnoDB DEFAULT CHARSET = utf8mb4 COMMENT '单位表';

-- 物资表
/*
id  code  name  category_id  unit_id  specification  
safety_stock  is_active  created_at  updated_at
*/
CREATE TABLE products (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY COMMENT '物资ID',
    code VARCHAR(30) NOT NULL UNIQUE COMMENT '物资编码',
    name VARCHAR(100) NOT NULL COMMENT '物资名称',
    category_id INT UNSIGNED NOT NULL COMMENT '分类ID',
    unit_id INT UNSIGNED NOT NULL COMMENT '单位ID',
    specification VARCHAR(200) COMMENT '规格',
    safety_stock INT NOT NULL DEFAULT 0 CHECK (safety_stock >= 0) COMMENT '安全库存',
    is_active TINYINT(1) NOT NULL DEFAULT 1 CHECK (is_active IN (0, 1)) COMMENT '状态',
    created_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3) COMMENT '创建时间',
    updated_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3) ON UPDATE CURRENT_TIMESTAMP(3) COMMENT '更新时间',
    FOREIGN KEY (category_id) REFERENCES categories (id) ON UPDATE RESTRICT ON DELETE RESTRICT, -- 分类ID
    FOREIGN KEY (unit_id) REFERENCES units (id) ON UPDATE RESTRICT ON DELETE RESTRICT, -- 单位ID
    INDEX idx_products_category (category_id), -- 种类ID
    INDEX idx_products_name (name) -- 物资名称
) ENGINE = InnoDB DEFAULT CHARSET = utf8mb4 COMMENT '物资表';

-- 入库单与明细
--入库单
/*
id  order_no supplier  status  operator_id  
confirmed_at  remark  created_at  updated_at
*/
CREATE TABLE inbound_orders (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY COMMENT '入库单ID',
    order_no VARCHAR(40) NOT NULL UNIQUE COMMENT 'UUID/ULID 或带锁编号表',
    supplier VARCHAR(100) COMMENT '供应商',
    status ENUM(
        'draft',
        'confirmed',
        'cancelled'
    ) NOT NULL DEFAULT 'draft' COMMENT '状态',
    operator_id INT UNSIGNED NOT NULL COMMENT '操作人ID',
    warehouse_id INT UNSIGNED NOT NULL COMMENT '仓库ID',
    confirmed_at DATETIME(3) NULL COMMENT '确认时间',
    remark TEXT COMMENT '备注',
    created_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3) COMMENT '创建时间',
    updated_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3) ON UPDATE CURRENT_TIMESTAMP(3) COMMENT '更新时间',
    FOREIGN KEY (warehouse_id) REFERENCES warehouses (id) ON UPDATE RESTRICT ON DELETE RESTRICT, -- 仓库ID
    FOREIGN KEY (operator_id) REFERENCES users (id) ON UPDATE RESTRICT ON DELETE RESTRICT, -- 操作人ID
    INDEX idx_inbound_status (status), -- 状态
    INDEX idx_inbound_created (created_at) -- 创建时间
) ENGINE = InnoDB DEFAULT CHARSET = utf8mb4 COMMENT '入库单表';

-- 入库单明细
/*
id  order_id product_id quantity unit_price
*/
CREATE TABLE inbound_details (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY COMMENT '入库单明细ID',
    order_id INT UNSIGNED NOT NULL COMMENT '入库单ID',
    product_id INT UNSIGNED NOT NULL COMMENT '物资ID',
    quantity INT NOT NULL CHECK (quantity > 0) COMMENT '数量',
    unit_price DECIMAL(12, 2) NOT NULL DEFAULT 0.00 CHECK (unit_price >= 0) COMMENT '入库单价',
    FOREIGN KEY (order_id) REFERENCES inbound_orders (id) ON UPDATE RESTRICT ON DELETE RESTRICT, -- 入库单ID
    FOREIGN KEY (product_id) REFERENCES products (id) ON UPDATE RESTRICT ON DELETE RESTRICT, -- 物资ID
    INDEX idx_inbound_detail_order (order_id) -- 入库单ID
) ENGINE = InnoDB DEFAULT CHARSET = utf8mb4 COMMENT '入库单明细表';

-- 出库单与明细
-- 出库单
/*
id  order_no recipient  status  operator_id  
confirmed_at  remark  created_at  updated_at
*/
CREATE TABLE outbound_orders (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY COMMENT '出库单ID',
    order_no VARCHAR(40) NOT NULL UNIQUE COMMENT 'UUID/ULID 或带锁编号表',
    recipient VARCHAR(100) COMMENT '接收人',
    status ENUM(
        'draft',
        'confirmed',
        'cancelled'
    ) NOT NULL DEFAULT 'draft' COMMENT '状态', -- 使用enum存储状态
    operator_id INT UNSIGNED NOT NULL COMMENT '操作人ID',
    warehouse_id INT UNSIGNED NOT NULL COMMENT '仓库ID',
    confirmed_at DATETIME(3) NULL COMMENT '确认时间',
    remark TEXT COMMENT '备注',
    created_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3) COMMENT '创建时间',
    updated_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3) ON UPDATE CURRENT_TIMESTAMP(3) COMMENT '更新时间',
    FOREIGN KEY (warehouse_id) REFERENCES warehouses (id) ON UPDATE RESTRICT ON DELETE RESTRICT, -- 仓库ID
    FOREIGN KEY (operator_id) REFERENCES users (id) ON UPDATE RESTRICT ON DELETE RESTRICT, -- 操作人ID
    INDEX idx_outbound_status (status), -- 状态
    INDEX idx_outbound_created (created_at) -- 创建时间
) ENGINE = InnoDB DEFAULT CHARSET = utf8mb4 COMMENT '出库单表';

-- 出库单明细
/*
id  order_id product_id quantity unit_price
*/
CREATE TABLE outbound_details (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY COMMENT '出库单明细ID',
    order_id INT UNSIGNED NOT NULL COMMENT '出库单ID',
    product_id INT UNSIGNED NOT NULL COMMENT '物资ID',
    quantity INT NOT NULL CHECK (quantity > 0) COMMENT '数量',
    unit_price DECIMAL(12, 2) NOT NULL DEFAULT 0.00 CHECK (unit_price >= 0) COMMENT '出库单价',
    FOREIGN KEY (order_id) REFERENCES outbound_orders (id) ON UPDATE RESTRICT ON DELETE RESTRICT, -- 出库单ID
    FOREIGN KEY (product_id) REFERENCES products (id) ON UPDATE RESTRICT ON DELETE RESTRICT, -- 物资ID
    INDEX idx_outbound_detail_order (order_id) -- 出库单ID
) ENGINE = InnoDB DEFAULT CHARSET = utf8mb4 COMMENT '出库单明细表';

-- 库存流水日志表
/*
id  movement_no product_id 
warehouse_id lot_id movement_type 
quantity_delta source_type source_id 
source_line_id movement_role 
operator_id reason created_at
*/
CREATE TABLE stock_movements (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY COMMENT '库存流水ID',
    movement_no VARCHAR(40) NOT NULL UNIQUE COMMENT '库存流水编号',
    product_id INT UNSIGNED NOT NULL COMMENT '物资ID',
    warehouse_id INT UNSIGNED NOT NULL COMMENT '仓库ID',
    lot_id INT UNSIGNED NULL COMMENT '批次,MVP 为 NULL',
    movement_type ENUM(
        'inbound',
        'outbound',
        'adjust',
        'reversal',
        'transfer'
    ) NOT NULL COMMENT '移动类型',
    quantity_delta INT NOT NULL CHECK (quantity_delta <> 0) COMMENT '移动数量,正入负出,不能为0',
    source_type VARCHAR(20) NOT NULL COMMENT '来源类型,inbound/outbound/adjust/reversal/transfer',
    source_id INT UNSIGNED NOT NULL COMMENT '来源ID',
    source_line_id BIGINT UNSIGNED NOT NULL COMMENT '来源行ID',
    movement_role ENUM(
        'normal',
        'transfer_out',
        'transfer_in',
        'reversal'
    ) NOT NULL DEFAULT 'normal' COMMENT '移动角色,normal:普通移动,transfer_out:出库,transfer_in:入库,reversal:反向移动',
    operator_id INT UNSIGNED NOT NULL COMMENT '操作人ID',
    reason VARCHAR(200) COMMENT '移动原因',
    created_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3) COMMENT '创建时间',
    FOREIGN KEY (product_id) REFERENCES products (id) ON UPDATE RESTRICT ON DELETE RESTRICT, -- 物资ID
    FOREIGN KEY (warehouse_id) REFERENCES warehouses (id) ON UPDATE RESTRICT ON DELETE RESTRICT, -- 仓库ID
    FOREIGN KEY (operator_id) REFERENCES users (id) ON UPDATE RESTRICT ON DELETE RESTRICT, -- 操作人ID
    UNIQUE KEY uk_movement_idem (
        source_type,
        source_id,
        source_line_id,
        movement_role
    ), -- 幂等键,防止重复移动
    INDEX idx_movement_product_warehouse (product_id, warehouse_id), -- 物资ID,仓库ID索引
    INDEX idx_movement_created (created_at) -- 创建时间索引
) ENGINE = InnoDB DEFAULT CHARSET = utf8mb4 COMMENT '库存流水日志表';

-- 库存余额
/*
id  product_id warehouse_id
quantity updated_at
*/
CREATE TABLE stock_balance (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY COMMENT '库存余额ID',
    product_id INT UNSIGNED NOT NULL COMMENT '物资ID',
    warehouse_id INT UNSIGNED NOT NULL COMMENT '仓库ID',
    quantity INT NOT NULL DEFAULT 0 CHECK (quantity >= 0) COMMENT '数量',
    updated_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3) ON UPDATE CURRENT_TIMESTAMP(3) COMMENT '更新时间',
    FOREIGN KEY (product_id) REFERENCES products (id) ON UPDATE RESTRICT ON DELETE RESTRICT, -- 物资ID
    FOREIGN KEY (warehouse_id) REFERENCES warehouses (id) ON UPDATE RESTRICT ON DELETE RESTRICT, -- 仓库ID
    -- 目前:  MVP 无批次 : 非空维度组合唯一 ; 
    -- 有批次 : 非空维度组合升级为(product_id,warehouse_id,location_id,lot_id)
    UNIQUE KEY uk_balance (product_id, warehouse_id)
) ENGINE = InnoDB DEFAULT CHARSET = utf8mb4 COMMENT '库存余额表';

-- 审计日志(追加写入,不可修改)
/*
id  operator_id username action 
target_type target_id detail created_at
*/
CREATE TABLE audit_logs (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY COMMENT '审计日志ID',
    operator_id INT UNSIGNED NOT NULL COMMENT '操作人ID',
    username VARCHAR(50) NOT NULL COMMENT '操作人用户名',
    action VARCHAR(20) NOT NULL COMMENT '操作,login/logout/create/update/confirm/cancel/export',
    target_type VARCHAR(20) NOT NULL COMMENT '目标类型',
    target_id VARCHAR(64) COMMENT '目标ID',
    detail JSON COMMENT '操作详情',
    created_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3) COMMENT '创建时间',
    FOREIGN KEY (operator_id) REFERENCES users (id) ON UPDATE RESTRICT ON DELETE RESTRICT, -- 操作人ID
    INDEX idx_audit_username (username), -- 操作人用户名
    INDEX idx_audit_action (action), -- 操作
    INDEX idx_audit_created (created_at) -- 创建时间
) ENGINE = InnoDB DEFAULT CHARSET = utf8mb4 COMMENT '审计日志表';

