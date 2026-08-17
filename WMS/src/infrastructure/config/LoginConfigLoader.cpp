#include "LoginConfigLoader.h"
#include "IniHelper.h"
LoginConfigLoader::LoginConfigLoader()
{
}
LoginConfig LoginConfigLoader::load(const QString& filePath, QString& errorMessage)
{
    IniHelper iniHelper(filePath);
    errorMessage.clear();
    if(!iniHelper.isValid())
    {
        errorMessage = QStringLiteral("登录配置文件加载失败");
        return LoginConfig();
    }
    LoginConfig loginConfig;
    
    if (!iniHelper.contains(QStringLiteral("Login/maxUserNameLength"))) {
        errorMessage = QStringLiteral("登录配置文件中没有maxUserNameLength");
        return LoginConfig();
    }
    if (!iniHelper.contains(QStringLiteral("Login/minUserNameLength"))) {
        errorMessage = QStringLiteral("登录配置文件中没有minUserNameLength");
        return LoginConfig();
    }
    if (!iniHelper.contains(QStringLiteral("Login/maxPasswordLength"))) {
        errorMessage = QStringLiteral("登录配置文件中没有maxPasswordLength");
        return LoginConfig();
    }
    if (!iniHelper.contains(QStringLiteral("Login/minPasswordLength"))) {
        errorMessage = QStringLiteral("登录配置文件中没有minPasswordLength");
        return LoginConfig();
    }
    loginConfig.maxUserNameLength = iniHelper.readInt(QStringLiteral("Login/maxUserNameLength"));
    loginConfig.minUserNameLength = iniHelper.readInt(QStringLiteral("Login/minUserNameLength"));
    loginConfig.maxPasswordLength = iniHelper.readInt(QStringLiteral("Login/maxPasswordLength"));
    loginConfig.minPasswordLength = iniHelper.readInt(QStringLiteral("Login/minPasswordLength"));
    if (!loginConfig.isValid()) {
        errorMessage = QStringLiteral("登录配置文件中配置无效");
        return LoginConfig();
    }
    return loginConfig;
}
