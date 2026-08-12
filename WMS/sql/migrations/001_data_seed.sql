-- 版本: 1
-- 条件: 已执行 001_initial.sql 建表, 且 001_admin_seed.sql 已写入管理员
-- 用途: 产品页/入库页/出库页 真实 MySQL 测试种子数据
-- 说明: 外键 ID 通过 @变量 + 唯一编码反查绑定, 不依赖自增顺序, 可重复执行前的清表后再次导入
-- 回滚: 001_initial_rollback.sql
-- 验证: 001_initial_verify.sql

USE wms;

-- ============================================================
-- 清表(逆序, 子表先删, 便于重复执行)
-- ============================================================
DELETE FROM audit_logs;

DELETE FROM stock_movements;

DELETE FROM stock_balance;

DELETE FROM outbound_details;

DELETE FROM outbound_orders;

DELETE FROM inbound_details;

DELETE FROM inbound_orders;

DELETE FROM products;

DELETE FROM units;

DELETE FROM categories;

DELETE FROM warehouses;

-- ============================================================
-- 仓库(5): 3 激活 + 2 停用
-- ============================================================
INSERT INTO
    warehouses (code, name, is_active)
VALUES ('WH-01', '主仓库', 1),
    ('WH-02', '副仓库', 1),
    ('WH-03', '备用仓库', 1),
    ('WH-04', '废弃仓库', 0),
    ('WH-05', '临时仓库', 0);

-- ============================================================
-- 分类(9): 7 激活 + 2 停用
-- ============================================================
INSERT INTO
    categories (code, name, is_active)
VALUES ('CAT-ELC', '电子产品', 1),
    ('CAT-MEC', '机械配件', 1),
    ('CAT-CON', '耗材', 1),
    ('CAT-PKG', '包装材料', 1),
    ('CAT-TOO', '工具', 1),
    ('CAT-CHE', '化工原料', 1),
    ('CAT-OFF', '办公用品', 1),
    ('CAT-FUR', '家具', 0),
    ('CAT-LAB', '实验室耗材', 0);

-- ============================================================
-- 单位(9): 7 激活 + 2 停用
-- ============================================================
INSERT INTO
    units (code, name, is_active)
VALUES ('U-PCS', '个', 1),
    ('U-BOX', '箱', 1),
    ('U-M', '米', 1),
    ('U-KG', '千克', 1),
    ('U-SET', '套', 1),
    ('U-ROL', '卷', 1),
    ('U-L', '升', 1),
    ('U-PCK', '包', 0),
    ('U-G', '克', 0);

-- ============================================================
-- 通过唯一编码反查外键 ID
-- ============================================================
SET @wh01 := ( SELECT id FROM warehouses WHERE code = 'WH-01' );

SET @wh02 := ( SELECT id FROM warehouses WHERE code = 'WH-02' );

SET @wh03 := ( SELECT id FROM warehouses WHERE code = 'WH-03' );

SET @wh04 := ( SELECT id FROM warehouses WHERE code = 'WH-04' );

SET @wh05 := ( SELECT id FROM warehouses WHERE code = 'WH-05' );

SET
    @cat_elec := (
        SELECT id
        FROM categories
        WHERE
            code = 'CAT-ELC'
    );

SET @cat_mec := ( SELECT id FROM categories WHERE code = 'CAT-MEC' );

SET @cat_con := ( SELECT id FROM categories WHERE code = 'CAT-CON' );

SET @cat_pkg := ( SELECT id FROM categories WHERE code = 'CAT-PKG' );

SET @cat_too := ( SELECT id FROM categories WHERE code = 'CAT-TOO' );

SET @cat_che := ( SELECT id FROM categories WHERE code = 'CAT-CHE' );

SET @cat_off := ( SELECT id FROM categories WHERE code = 'CAT-OFF' );

SET @cat_fur := ( SELECT id FROM categories WHERE code = 'CAT-FUR' );

SET @cat_lab := ( SELECT id FROM categories WHERE code = 'CAT-LAB' );

SET @u_pcs := ( SELECT id FROM units WHERE code = 'U-PCS' );

SET @u_box := ( SELECT id FROM units WHERE code = 'U-BOX' );

SET @u_m := ( SELECT id FROM units WHERE code = 'U-M' );

SET @u_kg := ( SELECT id FROM units WHERE code = 'U-KG' );

SET @u_set := ( SELECT id FROM units WHERE code = 'U-SET' );

SET @u_rol := ( SELECT id FROM units WHERE code = 'U-ROL' );

SET @u_l := ( SELECT id FROM units WHERE code = 'U-L' );

SET @u_pck := ( SELECT id FROM units WHERE code = 'U-PCK' );

SET @u_g := ( SELECT id FROM units WHERE code = 'U-G' );

SET
    @admin_id := (
        SELECT id
        FROM users
        WHERE
            username = 'Admin_001'
    );

-- ============================================================
-- 物资(50): 40 启用 + 10 停用, 覆盖全部 9 分类 / 7 激活单位
-- PAGESIZE = 20, 故 50 条可分 3 页(第 1 页 20, 第 2 页 20, 第 3 页 10)
-- ============================================================
INSERT INTO
    products (
        code,
        name,
        category_id,
        unit_id,
        specification,
        safety_stock,
        is_active
    )
VALUES (
        'P0001',
        '螺丝 M4x10',
        @cat_mec,
        @u_pcs,
        'M4x10 不锈钢',
        5000,
        1
    ),
    (
        'P0002',
        '螺母 M4',
        @cat_mec,
        @u_pcs,
        'M4 不锈钢',
        3000,
        1
    ),
    (
        'P0003',
        '垫圈 M4',
        @cat_mec,
        @u_pcs,
        'M4 平垫',
        2000,
        1
    ),
    (
        'P0004',
        '轴承 6204',
        @cat_mec,
        @u_pcs,
        '6204-2RS',
        100,
        1
    ),
    (
        'P0005',
        '电缆 2.5平方',
        @cat_con,
        @u_m,
        'BV 2.5mm²',
        800,
        1
    ),
    (
        'P0006',
        '电缆 4平方',
        @cat_con,
        @u_m,
        'BV 4mm²',
        500,
        1
    ),
    (
        'P0007',
        '绝缘胶带',
        @cat_con,
        @u_pcs,
        '18mm 黑色',
        1000,
        1
    ),
    (
        'P0008',
        '焊锡丝',
        @cat_con,
        @u_kg,
        '0.8mm 含助焊剂',
        50,
        1
    ),
    (
        'P0009',
        '万用表',
        @cat_too,
        @u_pcs,
        '数字式 自动量程',
        20,
        1
    ),
    (
        'P0010',
        '电烙铁',
        @cat_too,
        @u_pcs,
        '60W 可调温',
        30,
        1
    ),
    (
        'P0011',
        '十字螺丝刀',
        @cat_too,
        @u_set,
        'PH1/PH2',
        50,
        1
    ),
    (
        'P0012',
        '内六角扳手',
        @cat_too,
        @u_set,
        '1.5-10mm 9件套',
        40,
        1
    ),
    (
        'P0013',
        '电容 100uF',
        @cat_elec,
        @u_pcs,
        '100uF/25V 电解',
        2000,
        1
    ),
    (
        'P0014',
        '电阻 10K',
        @cat_elec,
        @u_pcs,
        '1/4W 1% 10K',
        5000,
        1
    ),
    (
        'P0015',
        'LED 红色',
        @cat_elec,
        @u_pcs,
        '5mm 红色',
        3000,
        1
    ),
    (
        'P0016',
        '继电器 5V',
        @cat_elec,
        @u_pcs,
        'SRD-05VDC',
        500,
        1
    ),
    (
        'P0017',
        '纸箱 中号',
        @cat_pkg,
        @u_pcs,
        '400x300x250mm',
        800,
        1
    ),
    (
        'P0018',
        '纸箱 大号',
        @cat_pkg,
        @u_pcs,
        '500x400x350mm',
        500,
        1
    ),
    (
        'P0019',
        '气泡膜',
        @cat_pkg,
        @u_m,
        '宽60cm',
        300,
        1
    ),
    (
        'P0020',
        '封箱胶带',
        @cat_pkg,
        @u_pcs,
        '48mm 透明',
        1000,
        1
    ),
    (
        'P0021',
        'USB数据线',
        @cat_elec,
        @u_pcs,
        'Type-C 1m',
        300,
        1
    ),
    (
        'P0022',
        '网线 5米',
        @cat_elec,
        @u_pcs,
        'CAT6 5m',
        200,
        1
    ),
    (
        'P0023',
        '电源适配器',
        @cat_elec,
        @u_pcs,
        '12V 2A',
        150,
        1
    ),
    (
        'P0024',
        '锂电池 18650',
        @cat_elec,
        @u_pcs,
        '3.7V 2500mAh',
        400,
        1
    ),
    (
        'P0025',
        '扎带',
        @cat_con,
        @u_pcs,
        '200mm 白色',
        2000,
        1
    ),
    (
        'P0026',
        '热缩管',
        @cat_con,
        @u_m,
        'Φ3mm 黑色',
        500,
        1
    ),
    (
        'P0031',
        '无水乙醇',
        @cat_che,
        @u_l,
        'AR 500ml',
        50,
        1
    ),
    (
        'P0032',
        '丙酮',
        @cat_che,
        @u_l,
        'AR 500ml',
        30,
        1
    ),
    (
        'P0033',
        '硫酸铜',
        @cat_che,
        @u_kg,
        'CuSO4·5H2O 工业级',
        20,
        1
    ),
    (
        'P0034',
        'A4打印纸',
        @cat_off,
        @u_box,
        '70g 500张/包 5包/箱',
        200,
        1
    ),
    (
        'P0035',
        '签字笔',
        @cat_off,
        @u_pcs,
        '0.5mm 黑色',
        500,
        1
    ),
    (
        'P0036',
        '文件夹',
        @cat_off,
        @u_pcs,
        'A4 蓝色',
        300,
        1
    ),
    (
        'P0037',
        '双面胶带',
        @cat_con,
        @u_rol,
        '宽12mm 长10m',
        200,
        1
    ),
    (
        'P0038',
        '生料带',
        @cat_con,
        @u_rol,
        '宽20mm 长15m',
        150,
        1
    ),
    (
        'P0039',
        '酒精棉片',
        @cat_che,
        @u_pcs,
        '6x6cm 100片/盒',
        80,
        1
    ),
    (
        'P0040',
        '白板笔',
        @cat_off,
        @u_pcs,
        '2.0mm 黑色',
        200,
        1
    ),
    (
        'P0027',
        '指针万用表(停产)',
        @cat_too,
        @u_pcs,
        NULL,
        5,
        0
    ),
    (
        'P0028',
        '串口线 DB9(停产)',
        @cat_elec,
        @u_pcs,
        'DB9 公母 2m',
        0,
        0
    ),
    (
        'P0029',
        '木箱(停用)',
        @cat_pkg,
        @u_pcs,
        NULL,
        0,
        0
    ),
    (
        'P0030',
        '英制扳手(停用)',
        @cat_too,
        @u_set,
        NULL,
        0,
        0
    ),
    (
        'P0041',
        '旧式办公桌(停用)',
        @cat_fur,
        @u_pcs,
        '1200x600x750mm',
        0,
        0
    ),
    (
        'P0042',
        '铁皮柜(停用)',
        @cat_fur,
        @u_pcs,
        NULL,
        0,
        0
    ),
    (
        'P0043',
        '试管架(停用)',
        @cat_lab,
        @u_pcs,
        'Φ18mm 40孔',
        0,
        0
    ),
    (
        'P0044',
        '烧杯(停用)',
        @cat_lab,
        @u_pcs,
        '500ml 硼硅玻璃',
        0,
        0
    ),
    (
        'P0045',
        '布袋(停用)',
        @cat_pkg,
        @u_pck,
        NULL,
        0,
        0
    ),
    (
        'P0046',
        '散装螺丝(停用)',
        @cat_mec,
        @u_g,
        'M3 自攻',
        0,
        0
    );

-- ============================================================
-- 库存余额(20): 覆盖 3 个激活仓库, 含高于/低于安全库存与 0 库存
-- ============================================================
INSERT INTO
    stock_balance (
        product_id,
        warehouse_id,
        quantity
    )
VALUES (
        (
            SELECT id
            FROM products
            WHERE
                code = 'P0001'
        ),
        @wh01,
        6200
    ),
    (
        (
            SELECT id
            FROM products
            WHERE
                code = 'P0001'
        ),
        @wh02,
        1000
    ),
    (
        (
            SELECT id
            FROM products
            WHERE
                code = 'P0004'
        ),
        @wh01,
        60
    ),
    (
        (
            SELECT id
            FROM products
            WHERE
                code = 'P0005'
        ),
        @wh01,
        1200
    ),
    (
        (
            SELECT id
            FROM products
            WHERE
                code = 'P0008'
        ),
        @wh01,
        12
    ),
    (
        (
            SELECT id
            FROM products
            WHERE
                code = 'P0009'
        ),
        @wh01,
        25
    ),
    (
        (
            SELECT id
            FROM products
            WHERE
                code = 'P0013'
        ),
        @wh01,
        1500
    ),
    (
        (
            SELECT id
            FROM products
            WHERE
                code = 'P0013'
        ),
        @wh02,
        800
    ),
    (
        (
            SELECT id
            FROM products
            WHERE
                code = 'P0017'
        ),
        @wh01,
        900
    ),
    (
        (
            SELECT id
            FROM products
            WHERE
                code = 'P0021'
        ),
        @wh01,
        0
    ),
    (
        (
            SELECT id
            FROM products
            WHERE
                code = 'P0024'
        ),
        @wh02,
        400
    ),
    (
        (
            SELECT id
            FROM products
            WHERE
                code = 'P0027'
        ),
        @wh01,
        3
    ),
    (
        (
            SELECT id
            FROM products
            WHERE
                code = 'P0031'
        ),
        @wh01,
        80
    ),
    (
        (
            SELECT id
            FROM products
            WHERE
                code = 'P0031'
        ),
        @wh03,
        20
    ),
    (
        (
            SELECT id
            FROM products
            WHERE
                code = 'P0034'
        ),
        @wh01,
        300
    ),
    (
        (
            SELECT id
            FROM products
            WHERE
                code = 'P0034'
        ),
        @wh02,
        150
    ),
    (
        (
            SELECT id
            FROM products
            WHERE
                code = 'P0035'
        ),
        @wh01,
        1000
    ),
    (
        (
            SELECT id
            FROM products
            WHERE
                code = 'P0037'
        ),
        @wh01,
        350
    ),
    (
        (
            SELECT id
            FROM products
            WHERE
                code = 'P0039'
        ),
        @wh01,
        30
    ),
    (
        (
            SELECT id
            FROM products
            WHERE
                code = 'P0002'
        ),
        @wh03,
        4500
    );

-- ============================================================
-- 入库单(10): 4 待处理 + 4 已确认 + 2 已取消, 覆盖 3 个激活仓库
-- ============================================================
INSERT INTO
    inbound_orders (
        order_no,
        supplier,
        status,
        operator_id,
        warehouse_id,
        confirmed_at,
        remark
    )
VALUES (
        'INB-20260801-000001',
        '华东电子科技有限公司',
        'confirmed',
        @admin_id,
        @wh01,
        '2026-08-01 10:30:00.000',
        '第一批电子元件入库'
    ),
    (
        'INB-20260802-000001',
        '华南机械配件厂',
        'confirmed',
        @admin_id,
        @wh01,
        '2026-08-02 14:00:00.000',
        '轴承批量入库'
    ),
    (
        'INB-20260803-000001',
        '深圳包装材料有限公司',
        'confirmed',
        @admin_id,
        @wh02,
        '2026-08-03 09:15:00.000',
        '纸箱入库'
    ),
    (
        'INB-20260804-000001',
        '广州化工原料供应站',
        'confirmed',
        @admin_id,
        @wh03,
        '2026-08-04 16:45:00.000',
        '实验室试剂入库'
    ),
    (
        'INB-20260805-000001',
        '北京办公用品批发中心',
        'draft',
        @admin_id,
        @wh01,
        NULL,
        '办公用品待入库'
    ),
    (
        'INB-20260806-000001',
        '上海电缆有限公司',
        'draft',
        @admin_id,
        @wh02,
        NULL,
        '电缆线材待入库'
    ),
    (
        'INB-20260807-000001',
        '南京工具制造厂',
        'draft',
        @admin_id,
        @wh01,
        NULL,
        '工具套装待入库'
    ),
    (
        'INB-20260808-000001',
        '武汉电子元器件市场',
        'draft',
        @admin_id,
        @wh03,
        NULL,
        '电子元器件待入库'
    ),
    (
        'INB-20260809-000001',
        '成都五金配件商行',
        'cancelled',
        @admin_id,
        @wh01,
        NULL,
        '供应商取消供货'
    ),
    (
        'INB-20260810-000001',
        '杭州包装制品厂',
        'cancelled',
        @admin_id,
        @wh02,
        NULL,
        '数量不符取消'
    );

-- 入库单ID变量(用于后续明细/流水)
SET
    @inb01 := (
        SELECT id
        FROM inbound_orders
        WHERE
            order_no = 'INB-20260801-000001'
    );

SET
    @inb02 := (
        SELECT id
        FROM inbound_orders
        WHERE
            order_no = 'INB-20260802-000001'
    );

SET
    @inb03 := (
        SELECT id
        FROM inbound_orders
        WHERE
            order_no = 'INB-20260803-000001'
    );

SET
    @inb04 := (
        SELECT id
        FROM inbound_orders
        WHERE
            order_no = 'INB-20260804-000001'
    );

SET
    @inb05 := (
        SELECT id
        FROM inbound_orders
        WHERE
            order_no = 'INB-20260805-000001'
    );

SET
    @inb06 := (
        SELECT id
        FROM inbound_orders
        WHERE
            order_no = 'INB-20260806-000001'
    );

SET
    @inb07 := (
        SELECT id
        FROM inbound_orders
        WHERE
            order_no = 'INB-20260807-000001'
    );

SET
    @inb08 := (
        SELECT id
        FROM inbound_orders
        WHERE
            order_no = 'INB-20260808-000001'
    );

SET
    @inb09 := (
        SELECT id
        FROM inbound_orders
        WHERE
            order_no = 'INB-20260809-000001'
    );

SET
    @inb10 := (
        SELECT id
        FROM inbound_orders
        WHERE
            order_no = 'INB-20260810-000001'
    );

-- 产品ID变量
SET @p0001 := ( SELECT id FROM products WHERE code = 'P0001' );

SET @p0002 := ( SELECT id FROM products WHERE code = 'P0002' );

SET @p0003 := ( SELECT id FROM products WHERE code = 'P0003' );

SET @p0004 := ( SELECT id FROM products WHERE code = 'P0004' );

SET @p0005 := ( SELECT id FROM products WHERE code = 'P0005' );

SET @p0006 := ( SELECT id FROM products WHERE code = 'P0006' );

SET @p0008 := ( SELECT id FROM products WHERE code = 'P0008' );

SET @p0009 := ( SELECT id FROM products WHERE code = 'P0009' );

SET @p0011 := ( SELECT id FROM products WHERE code = 'P0011' );

SET @p0013 := ( SELECT id FROM products WHERE code = 'P0013' );

SET @p0014 := ( SELECT id FROM products WHERE code = 'P0014' );

SET @p0015 := ( SELECT id FROM products WHERE code = 'P0015' );

SET @p0016 := ( SELECT id FROM products WHERE code = 'P0016' );

SET @p0017 := ( SELECT id FROM products WHERE code = 'P0017' );

SET @p0018 := ( SELECT id FROM products WHERE code = 'P0018' );

SET @p0019 := ( SELECT id FROM products WHERE code = 'P0019' );

SET @p0020 := ( SELECT id FROM products WHERE code = 'P0020' );

SET @p0021 := ( SELECT id FROM products WHERE code = 'P0021' );

SET @p0023 := ( SELECT id FROM products WHERE code = 'P0023' );

SET @p0031 := ( SELECT id FROM products WHERE code = 'P0031' );

SET @p0032 := ( SELECT id FROM products WHERE code = 'P0032' );

SET @p0034 := ( SELECT id FROM products WHERE code = 'P0034' );

SET @p0035 := ( SELECT id FROM products WHERE code = 'P0035' );

SET @p0036 := ( SELECT id FROM products WHERE code = 'P0036' );

SET @p0010 := ( SELECT id FROM products WHERE code = 'P0010' );

SET @p0012 := ( SELECT id FROM products WHERE code = 'P0012' );

SET @p0026 := ( SELECT id FROM products WHERE code = 'P0026' );

SET @p0039 := ( SELECT id FROM products WHERE code = 'P0039' );

SET @p0040 := ( SELECT id FROM products WHERE code = 'P0040' );

-- ============================================================
-- 入库单明细(25): 每条入库单 2-3 条明细
-- ============================================================
INSERT INTO
    inbound_details (
        order_id,
        product_id,
        quantity,
        unit_price
    )
VALUES (@inb01, @p0013, 3000, 0.25),
    (@inb01, @p0014, 8000, 0.05),
    (@inb01, @p0015, 5000, 0.10),
    (@inb02, @p0001, 8000, 0.02),
    (@inb02, @p0004, 200, 5.50),
    (@inb03, @p0017, 1000, 1.50),
    (@inb03, @p0018, 600, 2.00),
    (@inb03, @p0020, 1500, 0.50),
    (@inb04, @p0031, 80, 12.00),
    (@inb04, @p0032, 40, 15.00),
    (@inb05, @p0034, 400, 25.00),
    (@inb05, @p0035, 1000, 1.20),
    (@inb05, @p0036, 500, 3.00),
    (@inb06, @p0005, 1000, 2.80),
    (@inb06, @p0006, 600, 3.50),
    (@inb07, @p0009, 30, 85.00),
    (@inb07, @p0011, 80, 12.00),
    (@inb08, @p0016, 600, 3.50),
    (@inb08, @p0021, 400, 5.00),
    (@inb08, @p0023, 200, 18.00),
    (@inb09, @p0002, 5000, 0.01),
    (@inb09, @p0003, 3000, 0.01),
    (@inb10, @p0019, 500, 1.00),
    (@inb10, @p0020, 2000, 0.50),
    (@inb10, @p0017, 800, 1.50);

-- 入库明细ID变量(取每单第一条, 用于后续流水)
SET
    @ind01_1 := (
        SELECT id
        FROM inbound_details
        WHERE
            order_id = @inb01
        LIMIT 1
        OFFSET
            0
    );

SET
    @ind01_2 := (
        SELECT id
        FROM inbound_details
        WHERE
            order_id = @inb01
        LIMIT 1
        OFFSET
            1
    );

SET
    @ind01_3 := (
        SELECT id
        FROM inbound_details
        WHERE
            order_id = @inb01
        LIMIT 1
        OFFSET
            2
    );

SET
    @ind02_1 := (
        SELECT id
        FROM inbound_details
        WHERE
            order_id = @inb02
        LIMIT 1
        OFFSET
            0
    );

SET
    @ind02_2 := (
        SELECT id
        FROM inbound_details
        WHERE
            order_id = @inb02
        LIMIT 1
        OFFSET
            1
    );

SET
    @ind03_1 := (
        SELECT id
        FROM inbound_details
        WHERE
            order_id = @inb03
        LIMIT 1
        OFFSET
            0
    );

SET
    @ind03_2 := (
        SELECT id
        FROM inbound_details
        WHERE
            order_id = @inb03
        LIMIT 1
        OFFSET
            1
    );

SET
    @ind03_3 := (
        SELECT id
        FROM inbound_details
        WHERE
            order_id = @inb03
        LIMIT 1
        OFFSET
            2
    );

SET
    @ind04_1 := (
        SELECT id
        FROM inbound_details
        WHERE
            order_id = @inb04
        LIMIT 1
        OFFSET
            0
    );

SET
    @ind04_2 := (
        SELECT id
        FROM inbound_details
        WHERE
            order_id = @inb04
        LIMIT 1
        OFFSET
            1
    );

-- ============================================================
-- 出库单(8): 4 已确认 + 3 待处理 + 1 已取消
-- ============================================================
INSERT INTO
    outbound_orders (
        order_no,
        recipient,
        status,
        operator_id,
        warehouse_id,
        confirmed_at,
        remark
    )
VALUES (
        'OUT-20260801-000001',
        '张工',
        'confirmed',
        @admin_id,
        @wh01,
        '2026-08-01 11:00:00.000',
        '生产线领用电子元件'
    ),
    (
        'OUT-20260802-000001',
        '李主管',
        'confirmed',
        @admin_id,
        @wh01,
        '2026-08-02 15:30:00.000',
        '维修车间领用轴承'
    ),
    (
        'OUT-20260803-000001',
        '王经理',
        'confirmed',
        @admin_id,
        @wh02,
        '2026-08-03 17:00:00.000',
        '发货纸箱'
    ),
    (
        'OUT-20260804-000001',
        '赵主任',
        'confirmed',
        @admin_id,
        @wh03,
        '2026-08-05 08:30:00.000',
        '实验室领用试剂'
    ),
    (
        'OUT-20260806-000001',
        '陈工',
        'draft',
        @admin_id,
        @wh01,
        NULL,
        '待确认出库电缆'
    ),
    (
        'OUT-20260807-000001',
        '刘采购',
        'draft',
        @admin_id,
        @wh02,
        NULL,
        '待确认出库包装材料'
    ),
    (
        'OUT-20260808-000001',
        '孙组长',
        'draft',
        @admin_id,
        @wh01,
        NULL,
        '待确认出库工具'
    ),
    (
        'OUT-20260809-000001',
        '周工',
        'cancelled',
        @admin_id,
        @wh03,
        NULL,
        '客户取消订单'
    );

SET
    @out01 := (
        SELECT id
        FROM outbound_orders
        WHERE
            order_no = 'OUT-20260801-000001'
    );

SET
    @out02 := (
        SELECT id
        FROM outbound_orders
        WHERE
            order_no = 'OUT-20260802-000001'
    );

SET
    @out03 := (
        SELECT id
        FROM outbound_orders
        WHERE
            order_no = 'OUT-20260803-000001'
    );

SET
    @out04 := (
        SELECT id
        FROM outbound_orders
        WHERE
            order_no = 'OUT-20260804-000001'
    );

SET
    @out06 := (
        SELECT id
        FROM outbound_orders
        WHERE
            order_no = 'OUT-20260806-000001'
    );

SET
    @out07 := (
        SELECT id
        FROM outbound_orders
        WHERE
            order_no = 'OUT-20260807-000001'
    );

SET
    @out08 := (
        SELECT id
        FROM outbound_orders
        WHERE
            order_no = 'OUT-20260808-000001'
    );

SET
    @out09 := (
        SELECT id
        FROM outbound_orders
        WHERE
            order_no = 'OUT-20260809-000001'
    );

-- ============================================================
-- 出库单明细(20): 每条出库单 2-3 条明细
-- ============================================================
INSERT INTO
    outbound_details (
        order_id,
        product_id,
        quantity,
        unit_price
    )
VALUES (@out01, @p0013, 1500, 0.50),
    (@out01, @p0014, 3000, 0.10),
    (@out01, @p0015, 2000, 0.20),
    (@out02, @p0001, 3000, 0.03),
    (@out02, @p0004, 80, 8.00),
    (@out03, @p0017, 500, 2.00),
    (@out03, @p0018, 300, 2.50),
    (@out04, @p0031, 30, 18.00),
    (@out04, @p0032, 10, 22.00),
    (@out06, @p0005, 400, 4.00),
    (@out06, @p0006, 200, 5.00),
    (@out06, @p0026, 300, 1.50),
    (@out07, @p0019, 200, 1.80),
    (@out07, @p0020, 800, 0.80),
    (@out08, @p0010, 20, 45.00),
    (@out08, @p0012, 30, 25.00),
    (@out09, @p0039, 50, 8.00),
    (@out09, @p0040, 100, 3.50),
    (@out09, @p0035, 200, 2.00);

SET
    @outd01_1 := (
        SELECT id
        FROM outbound_details
        WHERE
            order_id = @out01
        LIMIT 1
        OFFSET
            0
    );

SET
    @outd01_2 := (
        SELECT id
        FROM outbound_details
        WHERE
            order_id = @out01
        LIMIT 1
        OFFSET
            1
    );

SET
    @outd01_3 := (
        SELECT id
        FROM outbound_details
        WHERE
            order_id = @out01
        LIMIT 1
        OFFSET
            2
    );

SET
    @outd02_1 := (
        SELECT id
        FROM outbound_details
        WHERE
            order_id = @out02
        LIMIT 1
        OFFSET
            0
    );

SET
    @outd02_2 := (
        SELECT id
        FROM outbound_details
        WHERE
            order_id = @out02
        LIMIT 1
        OFFSET
            1
    );

SET
    @outd03_1 := (
        SELECT id
        FROM outbound_details
        WHERE
            order_id = @out03
        LIMIT 1
        OFFSET
            0
    );

SET
    @outd03_2 := (
        SELECT id
        FROM outbound_details
        WHERE
            order_id = @out03
        LIMIT 1
        OFFSET
            1
    );

SET
    @outd04_1 := (
        SELECT id
        FROM outbound_details
        WHERE
            order_id = @out04
        LIMIT 1
        OFFSET
            0
    );

SET
    @outd04_2 := (
        SELECT id
        FROM outbound_details
        WHERE
            order_id = @out04
        LIMIT 1
        OFFSET
            1
    );

-- ============================================================
-- 库存流水(25): 对应已确认入库/出库, 含调拨/盘点/冲销, 避免 uk_movement_idem 冲突
-- ============================================================
INSERT INTO
    stock_movements (
        movement_no,
        product_id,
        warehouse_id,
        lot_id,
        movement_type,
        quantity_delta,
        source_type,
        source_id,
        source_line_id,
        movement_role,
        operator_id,
        reason,
        created_at
    )
VALUES
    -- 入库流水(INB-20260801-000001)
    (
        'MOV-001',
        @p0013,
        @wh01,
        NULL,
        'inbound',
        3000,
        'inbound',
        @inb01,
        @ind01_1,
        'normal',
        @admin_id,
        '电子元件入库',
        '2026-08-01 10:30:00.000'
    ),
    (
        'MOV-002',
        @p0014,
        @wh01,
        NULL,
        'inbound',
        8000,
        'inbound',
        @inb01,
        @ind01_2,
        'normal',
        @admin_id,
        '电子元件入库',
        '2026-08-01 10:30:00.000'
    ),
    (
        'MOV-003',
        @p0015,
        @wh01,
        NULL,
        'inbound',
        5000,
        'inbound',
        @inb01,
        @ind01_3,
        'normal',
        @admin_id,
        '电子元件入库',
        '2026-08-01 10:30:00.000'
    ),
    -- 入库流水(INB-20260802-000001)
    (
        'MOV-004',
        @p0001,
        @wh01,
        NULL,
        'inbound',
        8000,
        'inbound',
        @inb02,
        @ind02_1,
        'normal',
        @admin_id,
        '轴承批量入库',
        '2026-08-02 14:00:00.000'
    ),
    (
        'MOV-005',
        @p0004,
        @wh01,
        NULL,
        'inbound',
        200,
        'inbound',
        @inb02,
        @ind02_2,
        'normal',
        @admin_id,
        '轴承批量入库',
        '2026-08-02 14:00:00.000'
    ),
    -- 入库流水(INB-20260803-000001)
    (
        'MOV-006',
        @p0017,
        @wh02,
        NULL,
        'inbound',
        1000,
        'inbound',
        @inb03,
        @ind03_1,
        'normal',
        @admin_id,
        '纸箱入库',
        '2026-08-03 09:15:00.000'
    ),
    (
        'MOV-007',
        @p0018,
        @wh02,
        NULL,
        'inbound',
        600,
        'inbound',
        @inb03,
        @ind03_2,
        'normal',
        @admin_id,
        '纸箱入库',
        '2026-08-03 09:15:00.000'
    ),
    (
        'MOV-008',
        @p0020,
        @wh02,
        NULL,
        'inbound',
        1500,
        'inbound',
        @inb03,
        @ind03_3,
        'normal',
        @admin_id,
        '纸箱入库',
        '2026-08-03 09:15:00.000'
    ),
    -- 入库流水(INB-20260804-000001)
    (
        'MOV-009',
        @p0031,
        @wh03,
        NULL,
        'inbound',
        80,
        'inbound',
        @inb04,
        @ind04_1,
        'normal',
        @admin_id,
        '实验室试剂入库',
        '2026-08-04 16:45:00.000'
    ),
    (
        'MOV-010',
        @p0032,
        @wh03,
        NULL,
        'inbound',
        40,
        'inbound',
        @inb04,
        @ind04_2,
        'normal',
        @admin_id,
        '实验室试剂入库',
        '2026-08-04 16:45:00.000'
    ),
    -- 出库流水(OUT-20260801-000001)
    (
        'MOV-011',
        @p0013,
        @wh01,
        NULL,
        'outbound',
        -1500,
        'outbound',
        @out01,
        @outd01_1,
        'normal',
        @admin_id,
        '生产线领用',
        '2026-08-01 11:00:00.000'
    ),
    (
        'MOV-012',
        @p0014,
        @wh01,
        NULL,
        'outbound',
        -3000,
        'outbound',
        @out01,
        @outd01_2,
        'normal',
        @admin_id,
        '生产线领用',
        '2026-08-01 11:00:00.000'
    ),
    (
        'MOV-013',
        @p0015,
        @wh01,
        NULL,
        'outbound',
        -2000,
        'outbound',
        @out01,
        @outd01_3,
        'normal',
        @admin_id,
        '生产线领用',
        '2026-08-01 11:00:00.000'
    ),
    -- 出库流水(OUT-20260802-000001)
    (
        'MOV-014',
        @p0001,
        @wh01,
        NULL,
        'outbound',
        -3000,
        'outbound',
        @out02,
        @outd02_1,
        'normal',
        @admin_id,
        '维修车间领用',
        '2026-08-02 15:30:00.000'
    ),
    (
        'MOV-015',
        @p0004,
        @wh01,
        NULL,
        'outbound',
        -80,
        'outbound',
        @out02,
        @outd02_2,
        'normal',
        @admin_id,
        '维修车间领用',
        '2026-08-02 15:30:00.000'
    ),
    -- 出库流水(OUT-20260803-000001)
    (
        'MOV-016',
        @p0017,
        @wh02,
        NULL,
        'outbound',
        -500,
        'outbound',
        @out03,
        @outd03_1,
        'normal',
        @admin_id,
        '发货纸箱',
        '2026-08-03 17:00:00.000'
    ),
    (
        'MOV-017',
        @p0018,
        @wh02,
        NULL,
        'outbound',
        -300,
        'outbound',
        @out03,
        @outd03_2,
        'normal',
        @admin_id,
        '发货纸箱',
        '2026-08-03 17:00:00.000'
    ),
    -- 出库流水(OUT-20260804-000001)
    (
        'MOV-018',
        @p0031,
        @wh03,
        NULL,
        'outbound',
        -30,
        'outbound',
        @out04,
        @outd04_1,
        'normal',
        @admin_id,
        '实验室领用',
        '2026-08-05 08:30:00.000'
    ),
    (
        'MOV-019',
        @p0032,
        @wh03,
        NULL,
        'outbound',
        -10,
        'outbound',
        @out04,
        @outd04_2,
        'normal',
        @admin_id,
        '实验室领用',
        '2026-08-05 08:30:00.000'
    ),
    -- 调拨(独立 source_id, transfer_out + transfer_in 不冲突)
    (
        'MOV-020',
        @p0001,
        @wh01,
        NULL,
        'transfer',
        -2000,
        'transfer',
        901,
        901,
        'transfer_out',
        @admin_id,
        '调拨至副仓库',
        '2026-08-03 10:00:00.000'
    ),
    (
        'MOV-021',
        @p0001,
        @wh02,
        NULL,
        'transfer',
        2000,
        'transfer',
        901,
        901,
        'transfer_in',
        @admin_id,
        '从主仓库调入',
        '2026-08-03 10:00:00.000'
    ),
    -- 盘点调整(独立 source_id)
    (
        'MOV-022',
        @p0008,
        @wh01,
        NULL,
        'adjust',
        -5,
        'adjust',
        902,
        902,
        'normal',
        @admin_id,
        '盘点亏损调整',
        '2026-08-06 09:00:00.000'
    ),
    (
        'MOV-023',
        @p0035,
        @wh01,
        NULL,
        'adjust',
        50,
        'adjust',
        903,
        903,
        'normal',
        @admin_id,
        '盘点盈余调整',
        '2026-08-06 09:30:00.000'
    ),
    -- 冲销(独立 source_id)
    (
        'MOV-024',
        @p0013,
        @wh01,
        NULL,
        'reversal',
        100,
        'reversal',
        904,
        904,
        'reversal',
        @admin_id,
        '冲销出库流水',
        '2026-08-02 08:00:00.000'
    ),
    (
        'MOV-025',
        @p0014,
        @wh01,
        NULL,
        'reversal',
        200,
        'reversal',
        905,
        905,
        'reversal',
        @admin_id,
        '冲销出库流水',
        '2026-08-02 08:30:00.000'
    );

-- ============================================================
-- 审计日志(10): 覆盖登录/创建/确认/取消/停用/导出等操作
-- ============================================================
INSERT INTO
    audit_logs (
        operator_id,
        username,
        action,
        target_type,
        target_id,
        detail,
        created_at
    )
VALUES (
        @admin_id,
        'Admin_001',
        'login',
        'user',
        '1',
        JSON_OBJECT(
            'ip',
            '192.168.1.100',
            'result',
            'success'
        ),
        '2026-08-01 08:00:00.000'
    ),
    (
        @admin_id,
        'Admin_001',
        'create',
        'inbound',
        '1',
        JSON_OBJECT(
            'orderNo',
            'INB-20260801-000001',
            'supplier',
            '华东电子科技有限公司'
        ),
        '2026-08-01 10:00:00.000'
    ),
    (
        @admin_id,
        'Admin_001',
        'confirm',
        'inbound',
        '1',
        JSON_OBJECT(
            'orderNo',
            'INB-20260801-000001'
        ),
        '2026-08-01 10:30:00.000'
    ),
    (
        @admin_id,
        'Admin_001',
        'create',
        'outbound',
        '1',
        JSON_OBJECT(
            'orderNo',
            'OUT-20260801-000001',
            'recipient',
            '张工'
        ),
        '2026-08-01 10:45:00.000'
    ),
    (
        @admin_id,
        'Admin_001',
        'confirm',
        'outbound',
        '1',
        JSON_OBJECT(
            'orderNo',
            'OUT-20260801-000001'
        ),
        '2026-08-01 11:00:00.000'
    ),
    (
        @admin_id,
        'Admin_001',
        'cancel',
        'inbound',
        '9',
        JSON_OBJECT(
            'orderNo',
            'INB-20260809-000001',
            'reason',
            '供应商取消供货'
        ),
        '2026-08-09 15:00:00.000'
    ),
    (
        @admin_id,
        'Admin_001',
        'disable',
        'product',
        '27',
        JSON_OBJECT(
            'code',
            'P0027',
            'name',
            '指针万用表(停产)'
        ),
        '2026-08-06 10:00:00.000'
    ),
    (
        @admin_id,
        'Admin_001',
        'update',
        'product',
        '1',
        JSON_OBJECT(
            'code',
            'P0001',
            'field',
            'safety_stock',
            'old',
            4000,
            'new',
            5000
        ),
        '2026-08-07 09:00:00.000'
    ),
    (
        @admin_id,
        'Admin_001',
        'export',
        'inventory',
        NULL,
        JSON_OBJECT(
            'warehouse',
            'WH-01',
            'count',
            50
        ),
        '2026-08-10 17:00:00.000'
    ),
    (
        @admin_id,
        'Admin_001',
        'logout',
        'user',
        '1',
        JSON_OBJECT('duration', '8h30m'),
        '2026-08-10 17:30:00.000'
    );

-- ============================================================
-- 补充: 库存余额(10), 覆盖更多物资/仓库组合与边界场景
-- ============================================================
INSERT INTO
    stock_balance (
        product_id,
        warehouse_id,
        quantity
    )
VALUES (
        (
            SELECT id
            FROM products
            WHERE
                code = 'P0003'
        ),
        @wh01,
        0
    ),
    (
        (
            SELECT id
            FROM products
            WHERE
                code = 'P0006'
        ),
        @wh01,
        100
    ),
    (
        (
            SELECT id
            FROM products
            WHERE
                code = 'P0007'
        ),
        @wh02,
        800
    ),
    (
        (
            SELECT id
            FROM products
            WHERE
                code = 'P0010'
        ),
        @wh01,
        0
    ),
    (
        (
            SELECT id
            FROM products
            WHERE
                code = 'P0011'
        ),
        @wh02,
        30
    ),
    (
        (
            SELECT id
            FROM products
            WHERE
                code = 'P0019'
        ),
        @wh01,
        200
    ),
    (
        (
            SELECT id
            FROM products
            WHERE
                code = 'P0022'
        ),
        @wh03,
        50
    ),
    (
        (
            SELECT id
            FROM products
            WHERE
                code = 'P0025'
        ),
        @wh01,
        0
    ),
    (
        (
            SELECT id
            FROM products
            WHERE
                code = 'P0033'
        ),
        @wh01,
        5
    ),
    (
        (
            SELECT id
            FROM products
            WHERE
                code = 'P0038'
        ),
        @wh02,
        80
    );

-- ============================================================
-- 补充: 库存流水(5), 覆盖更多调拨/盘点/冲销场景
-- ============================================================
INSERT INTO
    stock_movements (
        movement_no,
        product_id,
        warehouse_id,
        lot_id,
        movement_type,
        quantity_delta,
        source_type,
        source_id,
        source_line_id,
        movement_role,
        operator_id,
        reason,
        created_at
    )
VALUES
    -- 调拨(副仓库->备用仓库)
    (
        'MOV-026',
        @p0002,
        @wh02,
        NULL,
        'transfer',
        -500,
        'transfer',
        906,
        906,
        'transfer_out',
        @admin_id,
        '调拨至备用仓库',
        '2026-08-04 14:00:00.000'
    ),
    (
        'MOV-027',
        @p0002,
        @wh03,
        NULL,
        'transfer',
        500,
        'transfer',
        906,
        906,
        'transfer_in',
        @admin_id,
        '从副仓库调入',
        '2026-08-04 14:00:00.000'
    ),
    -- 盘点调整(更多)
    (
        'MOV-028',
        @p0004,
        @wh01,
        NULL,
        'adjust',
        -20,
        'adjust',
        907,
        907,
        'normal',
        @admin_id,
        '季度盘点亏损',
        '2026-08-08 10:00:00.000'
    ),
    -- 冲销(更多)
    (
        'MOV-029',
        @p0009,
        @wh01,
        NULL,
        'reversal',
        5,
        'reversal',
        908,
        908,
        'reversal',
        @admin_id,
        '冲销错误出库',
        '2026-08-09 11:00:00.000'
    ),
    -- 盘点调整(盈余)
    (
        'MOV-030',
        @p0034,
        @wh02,
        NULL,
        'adjust',
        30,
        'adjust',
        909,
        909,
        'normal',
        @admin_id,
        '月度盘点盈余',
        '2026-08-11 09:00:00.000'
    );

-- ============================================================
-- 补充: 审计日志(5), 覆盖更多操作类型
-- ============================================================
INSERT INTO
    audit_logs (
        operator_id,
        username,
        action,
        target_type,
        target_id,
        detail,
        created_at
    )
VALUES (
        @admin_id,
        'Admin_001',
        'update',
        'warehouse',
        '4',
        JSON_OBJECT(
            'code',
            'WH-04',
            'name',
            '废弃仓库',
            'field',
            'is_active',
            'old',
            1,
            'new',
            0
        ),
        '2026-08-03 08:00:00.000'
    ),
    (
        @admin_id,
        'Admin_001',
        'update',
        'category',
        '8',
        JSON_OBJECT(
            'code',
            'CAT-FUR',
            'name',
            '家具',
            'field',
            'is_active',
            'old',
            1,
            'new',
            0
        ),
        '2026-08-04 09:00:00.000'
    ),
    (
        @admin_id,
        'Admin_001',
        'update',
        'unit',
        '8',
        JSON_OBJECT(
            'code',
            'U-PCK',
            'name',
            '包',
            'field',
            'is_active',
            'old',
            1,
            'new',
            0
        ),
        '2026-08-05 10:00:00.000'
    ),
    (
        @admin_id,
        'Admin_001',
        'cancel',
        'outbound',
        '4',
        JSON_OBJECT(
            'orderNo',
            'OUT-20260809-000001',
            'reason',
            '客户取消订单'
        ),
        '2026-08-09 14:00:00.000'
    ),
    (
        @admin_id,
        'Admin_001',
        'export',
        'stock_movements',
        NULL,
        JSON_OBJECT(
            'warehouse',
            'WH-01',
            'dateRange',
            '2026-08-01~2026-08-10',
            'count',
            25
        ),
        '2026-08-12 09:00:00.000'
    );