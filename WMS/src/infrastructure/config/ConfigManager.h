// 配置管理器 ConfigManager
#pragma once
#include "DatabaseConfigLoader.h"


// 配置管理器
class ConfigManager {
public:
    explicit ConfigManager(const QString& filePath);
    ~ConfigManager()=default;
    [[nodiscard]] QString errorMessage() const; // 获取错误信息
     DatabaseConfig loadDatabaseConfig(); // 加载数据库配置
private:
    QString configPath_; // 配置文件路径
    QString errorMessage_; // 错误信息
    DatabaseConfigLoader databaseConfigLoader_;
};