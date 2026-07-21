#include "DatabaseConfigLoader.h"

// 数据库配置加载器
DatabaseConfigLoader::DatabaseConfigLoader()
    : databaseConfig_()
{
}
// 加载数据库配置
DatabaseConfig DatabaseConfigLoader::load(const QString& filePath, QString& errorMessage)
{
    IniHelper helper(filePath);
    errorMessage.clear();
    if (!helper.isValid()) {
        errorMessage = QStringLiteral("数据库配置文件加载失败");
        return DatabaseConfig();
    }
    DatabaseConfig databaseConfig_;
    // 读取数据库配置
    databaseConfig_.qtDriver = helper.readString("Database/qtDriver", "QODBC");
    if (databaseConfig_.qtDriver.isEmpty()) {
        errorMessage = QStringLiteral("数据库配置文件中缺少必填字段:qtDriver");
        return DatabaseConfig();
    }
    databaseConfig_.odbcDriver = helper.readString("Database/odbcDriver", "MariaDB Unicode");
    if (databaseConfig_.odbcDriver.isEmpty()) {
        errorMessage = QStringLiteral("数据库配置文件中缺少必填字段:odbcDriver");
        return DatabaseConfig();
    }
    if (databaseConfig_.hostName.isEmpty()) {
        errorMessage = QStringLiteral("数据库配置文件中缺少必填字段:hostName");
        return DatabaseConfig();
    }
    if (databaseConfig_.port <= 0) {
        errorMessage = QStringLiteral("数据库配置文件中port字段值无效");
        return DatabaseConfig();
    }
    if (databaseConfig_.userName.isEmpty()) {
        errorMessage = QStringLiteral("数据库配置文件中缺少必填字段:userName");
        return DatabaseConfig();
    }
    if (databaseConfig_.password.isEmpty()) {
        errorMessage = QStringLiteral("数据库配置文件中缺少必填字段:password");
        return DatabaseConfig();
    }
    if (databaseConfig_.databaseName.isEmpty()) {
        errorMessage = QStringLiteral("数据库配置文件中缺少必填字段:databaseName");
        return DatabaseConfig();
    }
    databaseConfig_.connectionTimeoutMs = helper.readInt("Database/connectionTimeoutMs", 5000);
    if (databaseConfig_.connectionTimeoutMs <= 0) {
        errorMessage = QStringLiteral("数据库配置文件中connectionTimeoutMs字段值无效");
        return DatabaseConfig();
    }
    databaseConfig_.healthCheckIntervalMs = helper.readInt("Database/healthCheckIntervalMs", 30000);
    if (databaseConfig_.healthCheckIntervalMs <= 0) {
        errorMessage = QStringLiteral("数据库配置文件中healthCheckIntervalMs字段值无效");
        return DatabaseConfig();
    }
    databaseConfig_.queueCapacity = helper.readInt("Database/queueCapacity", 1000);
    if (databaseConfig_.queueCapacity <= 0) {
        errorMessage = QStringLiteral("数据库配置文件中queueCapacity字段值无效");
        return DatabaseConfig();
    }
    databaseConfig_.maxResultRows = helper.readInt("Database/maxResultRows", 10000);
    if (databaseConfig_.maxResultRows <= 0) {
        errorMessage = QStringLiteral("数据库配置文件中maxResultRows字段值无效");
        return DatabaseConfig();
    }
    databaseConfig_.shutdownDrainTimeoutMs = helper.readInt("Database/shutdownDrainTimeoutMs", 3000);
    if (databaseConfig_.shutdownDrainTimeoutMs <= 0) {
        errorMessage = QStringLiteral("数据库配置文件中shutdownDrainTimeoutMs字段值无效");
        return DatabaseConfig();
    }
    if (!databaseConfig_.isValidate(errorMessage)) {
        errorMessage = QStringLiteral("数据库配置无效");
        return DatabaseConfig();
    }
    return databaseConfig_;
}
