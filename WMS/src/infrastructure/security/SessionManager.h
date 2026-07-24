// 会话管理器 SessionManager
// 管理会话并保存当前用户的状态
#pragma once
#include "AuthenticatedUser.h"
#include "Permission.h"
#include "Role.h"
#include <QObject>
#include <QString>
#include <optional>
class SessionManager : public QObject {
    Q_OBJECT
public:
    explicit SessionManager(QObject* parent = nullptr);
    ~SessionManager() override= default;
    [[nodiscard]] bool isAuthenticated() const noexcept; // 是否认证
    [[nodiscard]] std::optional<AuthenticatedUser> currentUser() const; // 获取当前认证的用户
    [[nodiscard]] bool hasPermission(Permission permission) const noexcept;
    void startSession(const AuthenticatedUser& user); // 开启会话
    void endSession(); // 关闭会话
signals:
    void sessionStarted(const AuthenticatedUser& user); // 会话开启信号
    void sessionEnded(); // 会话关闭信号
    void sessionChanged(); // 会话更改信号
private:
    std::optional<AuthenticatedUser> currentUser_; // 当前认证的用户
};
