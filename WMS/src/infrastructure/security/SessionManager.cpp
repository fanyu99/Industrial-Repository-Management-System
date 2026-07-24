#include "SessionManager.h"
SessionManager::SessionManager(QObject* parent)
    : QObject(parent)
{
}
// 获取当前认证的用户(无/未认证则返回空optional)
std::optional<AuthenticatedUser> SessionManager::currentUser() const
{
    return currentUser_;
}
// 是否认证
bool SessionManager::isAuthenticated() const noexcept
{
    return currentUser_.has_value() && currentUser_->isValid();
}
// 是否有指定权限
bool SessionManager::hasPermission(Permission permission) const noexcept
{
    if (!isAuthenticated()) {
        return false;
    }
    return roleHasPermission(currentUser_.value().role, permission);
}
// 开启会话
void SessionManager::startSession(const AuthenticatedUser& user)
{
    currentUser_ = user;
    if (isAuthenticated()) {
        emit sessionChanged();
        emit sessionStarted(user);
    }
}
// 关闭会话
void SessionManager::endSession()
{
    if (isAuthenticated()) // 如果认证了,才发送会话改变信号
    {
        currentUser_.reset();
        emit sessionEnded();
        emit sessionChanged();
    }
}
