// 登录DTO
#pragma once
#include <QString>
#include "AppError.h"
#include "AuthenticatedUser.h"
// 登录请求DTO
struct LoginRequest{
    QString userName;
    QString password;
};
// 登录结果DTO
struct LoginResult {
    bool success { false };
    std::optional<AuthenticatedUser> user;
    std::optional<AppError> error;
};
