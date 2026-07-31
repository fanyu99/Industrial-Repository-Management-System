// 密码哈希相关类型定义
// 将纯数据结构从 Infrastructure 层提升到 Domain 层，解决循环依赖
#pragma once
#include <QByteArray>
#include <QString>

// 密码hash策略
struct PasswordHashPolicy {
    QString hashName { "" }; // hash算法名称 (空为无效名称)
    int algorithmVersion { 0 }; // hash算法版本号(-1表示无效,0表示默认值,但是无效)
    int iterations { 0 }; // 迭代次数(-1表示无效,0表示默认值,但是无效)
    int saltLength { 16 }; // 盐长度
    int hashLength { 32 }; // hash长度
    [[nodiscard]] bool isValid() const; // 校验策略是否有效
};

// 密码hash记录
struct PasswordHashRecord {
    QString hashName { "" }; // hash算法名称 (空为无效记录)
    int algorithmVersion { -1 }; // hash算法版本号(-1表示无效)
    int iterations { -1 }; // 迭代次数(-1表示无效)
    QByteArray salt; // 随机盐
    QByteArray hash; // 密码hash值
    [[nodiscard]] bool isValid() const; // 校验记录是否有效
};

// 密码hash结果error
enum class PasswordHashError {
    None, // 无错误
    InvalidPolicy, // 策略无效
    InvalidPassword, // 密码无效
    HashGenerateError // hash生成错误
};

// 密码校验错误
enum class PasswordVerifyError {
    None, // 无错误
    WrongPassword, // 密码错误
    InvalidRecord, // 记录无效
    HashGenerateError // hash生成错误
};

// 密码重hash结果
enum class PasswordRehashResult {
    No, // 无需升级
    Yes, // 需要升级
    InvalidPolicy, // 策略无效
    InvalidRecord // 记录无效
};