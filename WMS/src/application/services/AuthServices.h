#pragma once
#include "AppError.h"
#include "AuthCredentialRecord.h"
#include "IAuthRepository.h"
#include "SessionManager.h"
#include <QObject>
#include <QPointer>
#include <functional>
#include <optional>
// 认证服务 AuthServices
// 作为业务协调者,协调PasswordHasher校验密码,SessionManager管理会话,IAuthRepository(MySqlAuthRepository)拿取认证数据
// 负责: 登录/登出/权限边界等
class AuthService : public QObject {
    Q_OBJECT
public:
    explicit AuthService(
        IAuthRepository& repository,
        SessionManager& sessionManager,
        QObject* parent = nullptr);
    ~AuthService() = default;
    using Callback = std::function<void(const LoginResult&)>; // 异步调用回调函数
    void login(const QString& userName,
        const QString& password,
        QObject* owner,
        Callback callback); // 登录
    void logout(); // 登出
    [[nodiscard]] bool isAuthenticated() const; // 是否认证
    [[nodiscard]] std::optional<AuthenticatedUser> currentUser() const; // 当前的用户
    [[nodiscard]] bool hasPermission(Permission permission) const noexcept; // 是否有权限
    [[nodiscard]] std::optional<AppError> authorize(Permission permission) const; // 进行权限认证
    static AppError mapPasswordHashError(PasswordHashError error); // 映射密码哈希错误
    static AppError mapPasswordVerifyError(PasswordVerifyError error); // 映射密码校验错误
private:
    [[nodiscard]] std::optional<AppError> validateLoginRequest(const LoginRequest& loginRequest) const; // 校验登录请求
    [[nodiscard]] AuthenticatedUser buildAuthenticatedUser(const AuthCredentialRecord& record) const; // 创建认证用户
    IAuthRepository& repository_;
    SessionManager& sessionManager_;
};