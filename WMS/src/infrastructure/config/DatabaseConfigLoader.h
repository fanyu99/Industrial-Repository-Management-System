// 数据库配置加载器 DatabaseConfigLoader
#pragma once
#include <QString>
#include "DatabaseTypes.h"
#include "IniHelper.h"
// 数据库配置加载器
class DatabaseConfigLoader {
public:
    explicit DatabaseConfigLoader();
    ~DatabaseConfigLoader() = default;
    [[nodiscard]] DatabaseConfig load(const QString& filePath, QString& errorMessage); // 加载数据库配置
private:
};
