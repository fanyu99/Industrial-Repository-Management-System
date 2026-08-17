#pragma once
// TODO: 应用级页面协调器,对几个页面进行调度
#include <QLineEdit>
#include <QPushButton>
#include <QString>
#include <QWidget>
#include "AuthServices.h"
#include "AuthenticatedUser.h"
#include "LoginConfig.h"
enum class LoginPageState {
    Idle,
    Logging,
    Error,
    LoginSucceeded,
};
class LoginPage : public QWidget
{
    Q_OBJECT
public:
    LoginPage(AuthService* authService, const LoginConfig& loginConfig, QWidget* parent = nullptr);
    ~LoginPage() = default;

    // 显示信息
    void showInformationMessage(const QString& message);
    // 显示错误信息
    void showErrorMessage(const QString& message);
    // 设置页面状态
    void setPageState(LoginPageState state);
    
    signals:
    void loginSucceeded(const AuthenticatedUser& user); // 登录成功信号
    public slots:
    // 登录
    void onLoginClicked();
    void onLoginSuccess(); // 登录成功,销毁
private:
    QPushButton* loginBtn{nullptr};          // 登录按钮
    QLineEdit* userNameEdit{nullptr};        // 用户名输入
    QPushButton* clearUserNameBtn{nullptr};  // 清除用户名输入
    QLineEdit* passwordEdit{nullptr};        // 密码输入
    QPushButton* clearPasswordBtn{nullptr};  // 清除密码输入
    AuthService* authService_ { nullptr }; // 校验服务,查找用户信息
    LoginPageState state_ { LoginPageState::Idle }; // 当前页面状态
    LoginConfig loginConfig_; // 登录配置
};
