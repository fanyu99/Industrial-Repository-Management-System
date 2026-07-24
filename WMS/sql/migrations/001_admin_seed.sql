-- 版本: 1
-- 条件: 已有Wms库且存在users表
-- 管理员种子数据
-- 回滚: 001_initial_rollback.sql
-- 验证: 001_initial_verify.sql
Use wms;
INSERT INTO
    users (
        username,
        password_hash,
        password_salt,
        password_algorithm,
        password_iterations,
        real_name,
        role,
        is_active
    )
VALUES (
        'Admin_001',
        X'bc4611fa31fb88180e39a19d834c3c341615c7c9f7ba644c0eb9cf6de9b029ff',
        X'713e6cf3db210b1885ee75adf07baddd',
        'pbkdf2_hmac_sha256',
        10000,
        'FanYu',
        'Admin',
        1
    );
