#include "LoginPage.h"
#include <QHBoxLayout>
#include <QMessageBox>
#include <QVBoxLayout>
LoginPage::LoginPage(AuthService* authService, const LoginConfig& loginConfig, QWidget* parent)
    : authService_ { authService }
    , loginConfig_ {loginConfig}
    , QWidget(parent)
{

    setWindowTitle(QStringLiteral("登录"));
    // 创建布局
    QVBoxLayout* mainLayout = new QVBoxLayout();
    // 用户名输入
    userNameEdit = new QLineEdit(this);
    userNameEdit->setObjectName(QStringLiteral("LoginPage_userNameEdit"));
    userNameEdit->setPlaceholderText(QStringLiteral("请输入用户名/姓名"));
    userNameEdit->setMaxLength(loginConfig_.maxUserNameLength);
    clearUserNameBtn = new QPushButton(QStringLiteral("\u2715"), this);
    clearUserNameBtn->setFixedSize(24, 24);
    clearUserNameBtn->setToolTip(QStringLiteral("清除输入"));
    clearUserNameBtn->setVisible(false);
    clearUserNameBtn->setCursor(Qt::PointingHandCursor);
    QWidget* userNameContainer = new QWidget(this);
    QHBoxLayout* userNameLayout = new QHBoxLayout();
    userNameLayout->setContentsMargins(0, 0, 0, 0);
    userNameLayout->setSpacing(2);
    userNameLayout->addWidget(userNameEdit);
    userNameLayout->addWidget(clearUserNameBtn);
    userNameContainer->setLayout(userNameLayout);


    // 密码输入
    passwordEdit = new QLineEdit(this);
    passwordEdit->setObjectName(QStringLiteral("LoginPage_passwordEdit"));
    passwordEdit->setPlaceholderText(QStringLiteral("请输入密码"));
    passwordEdit->setMaxLength(loginConfig_.maxPasswordLength);
    passwordEdit->setEchoMode(QLineEdit::Password); // 屏蔽密码
    clearPasswordBtn = new QPushButton(QStringLiteral("\u2715"), this);
    clearPasswordBtn->setFixedSize(24, 24);
    clearPasswordBtn->setToolTip("清除输入");
    clearPasswordBtn->setVisible(false);
    clearPasswordBtn->setCursor(Qt::PointingHandCursor);
    QWidget* passwordContainer = new QWidget(this);
    QHBoxLayout *passwordLayout = new QHBoxLayout();
    passwordLayout->setContentsMargins(0, 0, 0, 0);
    passwordLayout->setSpacing(2);
    passwordLayout->addWidget(passwordEdit);
    passwordLayout->addWidget(clearPasswordBtn);
    passwordContainer->setLayout(passwordLayout);

    // 登录按钮
    loginBtn= new QPushButton(QStringLiteral("登录"), this);
    loginBtn->setObjectName(QStringLiteral("LoginPage_loginBtn"));
    loginBtn->setDefault(true);

    mainLayout->addWidget(userNameContainer);
    mainLayout->addWidget(passwordContainer);
    mainLayout->addWidget(loginBtn);
    mainLayout->setStretch(0, 2);
    mainLayout->setStretch(1, 2);
    mainLayout->setStretch(2, 3);
    // 信号槽
    connect(loginBtn, &QPushButton::clicked, this, &LoginPage::onLoginClicked);
    connect(userNameEdit, &QLineEdit::returnPressed, this, &LoginPage::onLoginClicked);
    connect(passwordEdit, &QLineEdit::returnPressed, this, &LoginPage::onLoginClicked);
    connect(userNameEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
        if (clearUserNameBtn)
            clearUserNameBtn->setVisible(!text.trimmed().isEmpty());
    });
    connect(passwordEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
        if(clearPasswordBtn)
        clearPasswordBtn->setVisible(!text.trimmed().isEmpty());
    });
    connect(clearUserNameBtn, &QPushButton::clicked, this, [this]() {
        if(userNameEdit){
userNameEdit->clear();
        userNameEdit->setFocus();
    }});
    connect(clearPasswordBtn, &QPushButton::clicked, this, [this]() {
        if(passwordEdit){
            passwordEdit->clear();
            clearPasswordBtn->setVisible(!passwordEdit->text().isEmpty());
        }
    });
    connect(this, &LoginPage::loginSucceeded, this, &LoginPage::onLoginSuccess);

    this->setLayout(mainLayout);
    setPageState(LoginPageState::Idle);
}
// 登录
void LoginPage::onLoginClicked()
{
    if (!authService_ ||!userNameEdit || !passwordEdit)
        return;
    const QString userName = userNameEdit->text().trimmed();
    const QString password = passwordEdit->text().trimmed();
    if (userName.isEmpty() || password.isEmpty())
    {
        showInformationMessage(QStringLiteral("请输入用户名和密码"));
        setPageState(LoginPageState::Idle);
        return;
    }
    if (userName.length() < loginConfig_.minUserNameLength || userName.length() > loginConfig_.maxUserNameLength)
    {
        showInformationMessage(QStringLiteral("用户名长度必须在%1到%2之间").arg(loginConfig_.minUserNameLength).arg(loginConfig_.maxUserNameLength));
        setPageState(LoginPageState::Idle);
        return;
    }
    if (password.length() < loginConfig_.minPasswordLength || password.length() > loginConfig_.maxPasswordLength)
    {
        showInformationMessage(QStringLiteral("密码长度必须在%1到%2之间").arg(loginConfig_.minPasswordLength).arg(loginConfig_.maxPasswordLength));
        setPageState(LoginPageState::Idle);
        return;
    }
    setPageState(LoginPageState::Logging);
    // 获取登录结果
    authService_->login(userName, password, this, [this](const LoginResult& result) {
        if(!result.success){
            showInformationMessage(result.error.has_value()?result.error->errorMessage:QStringLiteral("登录失败,未知错误"));
            setPageState(LoginPageState::Idle);
            return;
            }
        if (result.error.has_value()) {
            showInformationMessage(result.error->errorMessage);
            setPageState(LoginPageState::Idle);
            return;
        }
        if (!result.user.has_value()) {
            showInformationMessage(QStringLiteral("登录失败,用户不存在"));
            setPageState(LoginPageState::Idle);
            return;
        }
        const auto& user = result.user.value();
        if (!user.isValid()) {
            showInformationMessage(QStringLiteral("登录失败,用户无效"));
            setPageState(LoginPageState::Idle);
            return;
        }
        if (!authService_->currentUser().has_value()) {
            showErrorMessage(QStringLiteral("登录失败,未获取用户信息"));
            setPageState(LoginPageState::Idle);
            return;
        }

        showInformationMessage(QStringLiteral("登录成功"));
        
            emit loginSucceeded(authService_->currentUser().value());
        
        setPageState(LoginPageState::LoginSucceeded);
    });

}

// 显示信息
void LoginPage::showInformationMessage(const QString& message)
{
    if (!message.isEmpty())
        QMessageBox::information(this, QStringLiteral("信息"), message);
}
// 显示错误信息
void LoginPage::showErrorMessage(const QString& message)
{
    if (!message.isEmpty())
        QMessageBox::warning(this, QStringLiteral("错误"), message);
}
// 设置页面状态
void LoginPage::setPageState(LoginPageState state)
{
    state_ = state;
    bool canLogin = state == LoginPageState::Idle;
    loginBtn->setEnabled(canLogin);
    userNameEdit->setEnabled(canLogin);
    passwordEdit->setEnabled(canLogin);
}
// 登录成功,销毁页面并传递认证用户
void LoginPage::onLoginSuccess()
{
    this->close();
}
