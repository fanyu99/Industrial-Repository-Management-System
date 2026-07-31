-- 版本: 1
-- 条件: 已执行 001_initial.sql 建表, 且 001_admin_seed.sql 已写入管理员
-- 用途: 产品页(列表/分页/筛选)真实 MySQL 测试种子数据
-- 说明: 外键 ID 通过 @变量 + 唯一编码反查绑定, 不依赖自增顺序, 可重复执行前的清表后再次导入
-- 回滚: 001_initial_rollback.sql
-- 验证: 001_initial_verify.sql

USE wms;

-- 仓库(2)
INSERT INTO warehouses (code, name, is_active) VALUES
    ('WH-01', '主仓库', 1),
    ('WH-02', '副仓库', 1);

-- 分类(5)
INSERT INTO categories (code, name, is_active) VALUES
    ('CAT-ELC', '电子产品', 1),
    ('CAT-MEC', '机械配件', 1),
    ('CAT-CON', '耗材',     1),
    ('CAT-PKG', '包装材料', 1),
    ('CAT-TOO', '工具',     1);

-- 单位(5)
INSERT INTO units (code, name, is_active) VALUES
    ('U-PCS', '个',     1),
    ('U-BOX', '箱',     1),
    ('U-M',   '米',     1),
    ('U-KG',  '千克',   1),
    ('U-SET', '套',     1);

-- 通过唯一编码反查外键 ID, 避免依赖自增顺序
SET @wh01     := (SELECT id FROM warehouses WHERE code = 'WH-01');
SET @wh02     := (SELECT id FROM warehouses WHERE code = 'WH-02');
SET @cat_elec := (SELECT id FROM categories WHERE code = 'CAT-ELC');
SET @cat_mec  := (SELECT id FROM categories WHERE code = 'CAT-MEC');
SET @cat_con  := (SELECT id FROM categories WHERE code = 'CAT-CON');
SET @cat_pkg  := (SELECT id FROM categories WHERE code = 'CAT-PKG');
SET @cat_too  := (SELECT id FROM categories WHERE code = 'CAT-TOO');
SET @u_pcs    := (SELECT id FROM units WHERE code = 'U-PCS');
SET @u_box    := (SELECT id FROM units WHERE code = 'U-BOX');
SET @u_m      := (SELECT id FROM units WHERE code = 'U-M');
SET @u_kg     := (SELECT id FROM units WHERE code = 'U-KG');
SET @u_set    := (SELECT id FROM units WHERE code = 'U-SET');

-- 物资(30): 26 启用 + 4 停用, 覆盖 5 分类 / 4 单位, 含 NULL 规格与 0 安全库存
-- PAGESIZE = 20, 故 30 条可分 2 页(第 1 页 20 条, 第 2 页 10 条)
INSERT INTO products (code, name, category_id, unit_id, specification, safety_stock, is_active) VALUES
    -- 机械配件
    ('P0001', '螺丝 M4x10',        @cat_mec,  @u_pcs, 'M4x10 不锈钢',     5000, 1),
    ('P0002', '螺母 M4',           @cat_mec,  @u_pcs, 'M4 不锈钢',         3000, 1),
    ('P0003', '垫圈 M4',           @cat_mec,  @u_pcs, 'M4 平垫',           2000, 1),
    ('P0004', '轴承 6204',         @cat_mec,  @u_pcs, '6204-2RS',           100, 1),
    -- 耗材
    ('P0005', '电缆 2.5平方',      @cat_con,  @u_m,   'BV 2.5mm²',          800, 1),
    ('P0006', '电缆 4平方',        @cat_con,  @u_m,   'BV 4mm²',            500, 1),
    ('P0007', '绝缘胶带',          @cat_con,  @u_pcs, '18mm 黑色',         1000, 1),
    ('P0008', '焊锡丝',            @cat_con,  @u_kg,  '0.8mm 含助焊剂',       50, 1),
    -- 工具
    ('P0009', '万用表',            @cat_too,  @u_pcs, '数字式 自动量程',      20, 1),
    ('P0010', '电烙铁',            @cat_too,  @u_pcs, '60W 可调温',          30, 1),
    ('P0011', '十字螺丝刀',        @cat_too,  @u_set, 'PH1/PH2',             50, 1),
    ('P0012', '内六角扳手',        @cat_too,  @u_set, '1.5-10mm 9件套',      40, 1),
    -- 电子产品
    ('P0013', '电容 100uF',        @cat_elec, @u_pcs, '100uF/25V 电解',    2000, 1),
    ('P0014', '电阻 10K',          @cat_elec, @u_pcs, '1/4W 1% 10K',       5000, 1),
    ('P0015', 'LED 红色',          @cat_elec, @u_pcs, '5mm 红色',          3000, 1),
    ('P0016', '继电器 5V',         @cat_elec, @u_pcs, 'SRD-05VDC',          500, 1),
    -- 包装材料
    ('P0017', '纸箱 中号',         @cat_pkg,  @u_pcs, '400x300x250mm',      800, 1),
    ('P0018', '纸箱 大号',         @cat_pkg,  @u_pcs, '500x400x350mm',      500, 1),
    ('P0019', '气泡膜',            @cat_pkg,  @u_m,   '宽60cm',             300, 1),
    ('P0020', '封箱胶带',          @cat_pkg,  @u_pcs, '48mm 透明',         1000, 1),
    -- 电子产品(续)
    ('P0021', 'USB数据线',         @cat_elec, @u_pcs, 'Type-C 1m',          300, 1),
    ('P0022', '网线 5米',          @cat_elec, @u_pcs, 'CAT6 5m',            200, 1),
    ('P0023', '电源适配器',        @cat_elec, @u_pcs, '12V 2A',             150, 1),
    ('P0024', '锂电池 18650',      @cat_elec, @u_pcs, '3.7V 2500mAh',       400, 1),
    -- 耗材(续)
    ('P0025', '扎带',              @cat_con,  @u_pcs, '200mm 白色',        2000, 1),
    ('P0026', '热缩管',            @cat_con,  @u_m,   'Φ3mm 黑色',          500, 1),
    -- 停用物资(用于测试 is_active 筛选): 含 NULL 规格与 0 安全库存
    ('P0027', '指针万用表(停产)',  @cat_too,  @u_pcs, NULL,                   5, 0),
    ('P0028', '串口线 DB9(停产)',  @cat_elec, @u_pcs, 'DB9 公母 2m',           0, 0),
    ('P0029', '木箱(停用)',        @cat_pkg,  @u_pcs, NULL,                   0, 0),
    ('P0030', '英制扳手(停用)',    @cat_too,  @u_set, NULL,                   0, 0);

-- 库存余额(12): 覆盖两个仓库, 含高于/低于安全库存与 0 库存, 便于后续低库存视图测试
INSERT INTO stock_balance (product_id, warehouse_id, quantity) VALUES
    ((SELECT id FROM products WHERE code = 'P0001'), @wh01, 6200), -- 高于安全库存 5000
    ((SELECT id FROM products WHERE code = 'P0001'), @wh02, 1000),
    ((SELECT id FROM products WHERE code = 'P0004'), @wh01,   60), -- 低于安全库存 100(低库存)
    ((SELECT id FROM products WHERE code = 'P0005'), @wh01, 1200), -- 高于安全库存 800
    ((SELECT id FROM products WHERE code = 'P0008'), @wh01,   12), -- 低于安全库存 50(低库存)
    ((SELECT id FROM products WHERE code = 'P0009'), @wh01,   25), -- 高于安全库存 20
    ((SELECT id FROM products WHERE code = 'P0013'), @wh01, 1500),
    ((SELECT id FROM products WHERE code = 'P0013'), @wh02,  800),
    ((SELECT id FROM products WHERE code = 'P0017'), @wh01,  900), -- 高于安全库存 800
    ((SELECT id FROM products WHERE code = 'P0021'), @wh01,    0), -- 缺货(安全库存 300)
    ((SELECT id FROM products WHERE code = 'P0024'), @wh02,  400),
    ((SELECT id FROM products WHERE code = 'P0027'), @wh01,    3); -- 停用物资残留库存
