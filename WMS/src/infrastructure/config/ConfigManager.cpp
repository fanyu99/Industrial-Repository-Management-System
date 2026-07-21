#include "ConfigManager.h"


// 配置管理器
ConfigManager::ConfigManager(const QString& filePath)
    : configPath_(filePath)
    , errorMessage_(QString())
{
}

// 获取错误信息
QString ConfigManager::errorMessage() const
{
    return errorMessage_;
}

// 加载数据库配置
DatabaseConfig ConfigManager::loadDatabaseConfig()
{
    return databaseConfigLoader_.load(configPath_, errorMessage_);
}