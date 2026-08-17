#pragma once
#include "LoginConfig.h"
class LoginConfigLoader {
public:
    explicit LoginConfigLoader();
    ~LoginConfigLoader() = default;
    [[nodiscard]] LoginConfig load(const QString& filePath, QString& errorMessage); // 加载登录配置

};
