#pragma once
#include "PasswordHasher.h"
#include "Role.h"
#include <QString>
// 认证凭证记录 -> PasswordHasher::verifyPassword() 校验密码
// 认证凭证记录
struct AuthCredentialRecord {
    quint32 userId; // 用户ID
    QString userName; // 用户名
    QString realName; // 真名
    Role role { Role::Operator }; // 用户类型
    bool active { false }; // 是否激活
    PasswordHashRecord passwordHashRecord; // 密码哈希记录
};
