#include "DatabaseConfigLoader.h"

// 数据库配置加载器
DatabaseConfigLoader::DatabaseConfigLoader()
{
}
// 加载数据库配置
DatabaseConfig DatabaseConfigLoader::load(const QString& filePath, QString& errorMessage)
{
    IniHelper helper(filePath);
    errorMessage.clear();
    // 检查配置文件是否存在
    if (!helper.isValid()) {
        errorMessage = QStringLiteral("数据库配置文件加载失败");
        return DatabaseConfig();
    }
    DatabaseConfig databaseConfig_;
    // 读取数据库配置
    // qtDriver
    databaseConfig_.qtDriver = helper.readString("Database/qtDriver");
    if (databaseConfig_.qtDriver.isEmpty()) {
        errorMessage = QStringLiteral("数据库配置文件中缺少必填字段:qtDriver");
        return DatabaseConfig();
    }
    // odbcDriver
    databaseConfig_.odbcDriver = helper.readString("Database/odbcDriver");
    if (databaseConfig_.odbcDriver.isEmpty()) {
        errorMessage = QStringLiteral("数据库配置文件中缺少必填字段:odbcDriver");
        return DatabaseConfig();
    }
    // hostName
    databaseConfig_.hostName = helper.readString("Database/hostName");
    if (databaseConfig_.hostName.isEmpty()) {
        errorMessage = QStringLiteral("数据库配置文件中缺少必填字段:hostName");
        return DatabaseConfig();
    }
    // port
    databaseConfig_.port = helper.readInt("Database/port");
    if (databaseConfig_.port <= 0) {
        errorMessage = QStringLiteral("数据库配置文件中port字段值无效");
        return DatabaseConfig();
    }
    // userName
    databaseConfig_.userName = helper.readString("Database/userName");
    if (databaseConfig_.userName.isEmpty()) {
        errorMessage = QStringLiteral("数据库配置文件中缺少必填字段:userName");
        return DatabaseConfig();
    }
    // password
    databaseConfig_.password = helper.readString("Database/password");
    if (databaseConfig_.password.isEmpty()) {
        errorMessage = QStringLiteral("数据库配置文件中缺少必填字段:password");
        return DatabaseConfig();
    }
    // databaseName
    databaseConfig_.databaseName = helper.readString("Database/databaseName");
    if (databaseConfig_.databaseName.isEmpty()) {
        errorMessage = QStringLiteral("数据库配置文件中缺少必填字段:databaseName");
        return DatabaseConfig();
    }
    // connectionTimeoutMs
    databaseConfig_.connectionTimeoutMs = helper.readInt("Database/connectionTimeoutMs", 5000);
    if (databaseConfig_.connectionTimeoutMs <= 0) {
        errorMessage = QStringLiteral("数据库配置文件中connectionTimeoutMs字段值无效");
        return DatabaseConfig();
    }
    // healthCheckIntervalMs
    databaseConfig_.healthCheckIntervalMs = helper.readInt("Database/healthCheckIntervalMs", 30000);
    if (databaseConfig_.healthCheckIntervalMs <= 0) {
        errorMessage = QStringLiteral("数据库配置文件中healthCheckIntervalMs字段值无效");
        return DatabaseConfig();
    }
    // queueCapacity
    databaseConfig_.queueCapacity = helper.readInt("Database/queueCapacity", 1000);
    if (databaseConfig_.queueCapacity <= 0) {
        errorMessage = QStringLiteral("数据库配置文件中queueCapacity字段值无效");
        return DatabaseConfig();
    }
    // maxResultRows
    databaseConfig_.maxResultRows = helper.readInt("Database/maxResultRows", 10000);
    if (databaseConfig_.maxResultRows <= 0) {
        errorMessage = QStringLiteral("数据库配置文件中maxResultRows字段值无效");
        return DatabaseConfig();
    }
    // shutdownDrainTimeoutMs
    databaseConfig_.shutdownDrainTimeoutMs = helper.readInt("Database/shutdownDrainTimeoutMs", 3000);
    if (databaseConfig_.shutdownDrainTimeoutMs <= 0) {
        errorMessage = QStringLiteral("数据库配置文件中shutdownDrainTimeoutMs字段值无效");
        return DatabaseConfig();
    }   
    // 最后对数据库配置进行验证
    if (!databaseConfig_.isValid(errorMessage)) {
        errorMessage = QStringLiteral("数据库配置无效");
        return DatabaseConfig();
    }
    return databaseConfig_;
}