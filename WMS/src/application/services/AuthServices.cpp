#include "AuthServices.h"
#include "IAuthRepository.h"
#include "PasswordHasher.h"
#include <memory>

AuthService::AuthService(
    IAuthRepository& repository,
    SessionManager& sessionManager,
    QObject* parent)
    : repository_ { repository }
    , sessionManager_ { sessionManager }
    , QObject(parent)
{
}
// 是否认证
bool AuthService::isAuthenticated() const
{
    return sessionManager_.isAuthenticated();
}
// 是否有指定权限
bool AuthService::hasPermission(Permission permission) const noexcept
{
    return sessionManager_.hasPermission(permission);
}
// 当前用户
std::optional<AuthenticatedUser> AuthService::currentUser() const
{
    return sessionManager_.currentUser();
}
// 进行权限认证
std::optional<AppError> AuthService::authorize(Permission permission) const
{
    if (!isAuthenticated()) {
        return AppError {
            AppErrorCategory::Auth,
            AppErrorCode::NotAuthenticated,
            QStringLiteral("用户未登录")
        };
    }
    if (!hasPermission(permission)) {
        return AppError::permissionDenied();
    }
    return std::nullopt;
}
// 校验登录请求
std::optional<AppError> AuthService::validateLoginRequest(const LoginRequest& loginRequest) const
{
    if (loginRequest.userName.isEmpty() || loginRequest.password.isEmpty()) {
        return AppError {
            AppErrorCategory::Validation,
            AppErrorCode::InvalidInput,
            QStringLiteral("用户名或密码不能为空")
        };
    }
    return std::nullopt;
}
// 记录转为认证用户
AuthenticatedUser AuthService::buildAuthenticatedUser(const AuthCredentialRecord& record) const
{
    if (record.active)
        return AuthenticatedUser {
            record.userId,
            record.userName,
            record.realName,
            record.role
        };
    return {};
}
// 登录
void AuthService::login(const QString& userName, const QString& password, QObject* owner, Callback callback)
{
    LoginRequest request { userName, password };
    QPointer<QObject> ownerPtr(owner);
    // 判断生命周期
    if (ownerPtr.isNull())
        return;
    // 如果未通过登录请求验证
    if (auto error = validateLoginRequest(request);
        error.has_value()) {
        if (callback != nullptr)
            callback(LoginResult {
                false,
                std::nullopt,
                error.value() });
        return;
    }
    // 使用repository接口findCredentialByUserName()获取认证凭证记录
    repository_.findCredentialByUserName(
        userName,
        owner,
        [this, ownerPtr, password, callback = std::move(callback)](const FindCredentialResult& repoResult) mutable {
            // 确保ownerPtr未被销毁
            if (ownerPtr.isNull())
                return;
            // 如果查询失败
            if (repoResult.error.has_value()) {
                if (callback != nullptr)
                    callback(LoginResult {
                        false,
                        std::nullopt,
                        repoResult.error.value() });
                return;
            }
            // 如果未找到认证凭证记录
            if (!repoResult.credential.has_value()) {
                if (callback != nullptr)
                    callback(LoginResult {
                        false,
                        std::nullopt,
                        AppError::invalidCredentials() });
                return;
            }
            const auto& record = repoResult.credential.value();
            // 如果未激活
            if (!record.active) {
                if (callback != nullptr) {
                    callback(LoginResult {
                        false,
                        std::nullopt,
                        AppError::userDisabled() });
                }
                return;
            }
            PasswordHasher hasher;
            auto passwordValid = hasher.verifyPassword(password, record.passwordHashRecord);
            if (passwordValid != PasswordVerifyError::None) {
                if (callback != nullptr)
                    callback(LoginResult {
                        false,
                        std::nullopt,
                        AppError::invalidCredentials() });
                return;
            }
            // 密码正确开启会话,回调结果
            auto user = buildAuthenticatedUser(record);
            sessionManager_.startSession(user);
            if (callback != nullptr)
                callback(LoginResult {
                    true,
                    std::make_optional(user),
                    std::nullopt });
        });
}
// 登出
void AuthService::logout()
{
    sessionManager_.endSession();
}
