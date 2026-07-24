#pragma once
#include <QString>
// 错误类别
enum class AppErrorCategory {
    None, // 无错误
    Auth, // 认证错误
    Permission, // 权限错误
    Database, // 数据库错误
    Validation, // 校验错误
    System // 系统错误
};
// 错误码
enum class AppErrorCode {
    None, // 无错误

    // Auth
    InvalidCredentials, // 无效的认证凭证
    UserDisabled, // 用户被禁用
    NotAuthenticated, // 未认证
    PermissionDenied, // 权限不足
    // Database / Repository
    RepositoryFailure, // 仓库失败
    DatabaseFailure, // 数据库失败
    // Validation
    InvalidInput, // 无效的输入
    // System
    UnknownError // 未知错误
};
// 应用错误
struct AppError {
    AppErrorCategory category { AppErrorCategory::None };
    AppErrorCode code { AppErrorCode::None };
    QString errorMessage;

    [[nodiscard]] bool hasError() const noexcept
    {
        return code != AppErrorCode::None;
    }

    static AppError none()
    {
        return {};
    }

    static AppError invalidCredentials()
    {
        return {
            AppErrorCategory::Auth,
            AppErrorCode::InvalidCredentials,
            QStringLiteral("用户名或密码错误")
        };
    }

    static AppError userDisabled()
    {
        return {
            AppErrorCategory::Auth,
            AppErrorCode::UserDisabled,
            QStringLiteral("用户已被禁用")
        };
    }

    static AppError permissionDenied()
    {
        return {
            AppErrorCategory::Permission,
            AppErrorCode::PermissionDenied,
            QStringLiteral("没有执行该操作的权限")
        };
    }

    static AppError repositoryFailure(const QString& message)
    {
        return {
            AppErrorCategory::Database,
            AppErrorCode::RepositoryFailure,
            message
        };
    }
};