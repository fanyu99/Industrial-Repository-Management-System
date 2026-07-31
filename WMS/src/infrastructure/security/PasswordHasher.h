// WMS安全模块 PasswordHasher

// 将密码转为可存储的hash记录
// 校验输入密码和存储记录是否匹配
// 判断旧记录是否需要升级重hash

/* TODO: 未来可能需要PasswordHasher类中创建一个工厂用来创建不同算法下的生成器
PasswordHash::hashPassword()用作入口
工厂用于创建和选择具体算法类
*/

#pragma once
#include "PasswordHashTypes.h"
#include <QByteArray>
#include <QPair>
#include <QPasswordDigestor>
#include <QRandomGenerator>
#include <QString>

// 目前仅支持pbkdf2_hmac_sha256算法:
constexpr auto kPbkdf2Sha256Name = "pbkdf2_hmac_sha256";
constexpr int minSaltLength = 16; // 最小盐长度
constexpr int minHashLength = 32; // 最小的hash长度
constexpr int nowAlgorithmVersion = 1; // 当前算法版本号

// 密码hash器
class PasswordHasher {
public:
    explicit PasswordHasher() = default;
    ~PasswordHasher() = default;
    QPair<PasswordHashRecord, PasswordHashError> hashPassword(const QString& password, const PasswordHashPolicy& policy) const; // 获取密码hash记录与hash结果error,无错误则密码hash成功
    PasswordVerifyError verifyPassword(const QString& inputPassword, const PasswordHashRecord& record) const; // 校验密码是否匹配
    PasswordRehashResult needRehash(const PasswordHashRecord& oldRecord, const PasswordHashPolicy& nowPolicy) const; // 判断旧记录是否需要升级重hash
private:
    QByteArray generateSalt(const PasswordHashPolicy& policy) const; // 生成随机盐
};