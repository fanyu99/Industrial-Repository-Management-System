// 认证用户(仅保存用户认证阶段的信息)
#pragma once
#include "PasswordHasher.h"
#include "Role.h"
#include <QString>
// 已经登录的用户信息
struct AuthenticatedUser {
    quint32 id { 0 };
    QString userName { "" };
    QString realName { "" };
    Role role { Role::Operator };
    [[nodiscard]] bool isValid() const
    {
        return id > 0 && !userName.trimmed().isEmpty();
    }
    void reset()
    {
        id = 0;
        userName = "";
        realName = "";
        role = Role::Operator;
    }
};
