#include "DatabaseConfigLoader.h"
#include "IniHelper.h"
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
    if (!helper.contains("Database/qtDriver")) {
        errorMessage = QStringLiteral("数据库配置文件中缺少必填字段:qtDriver");
        return DatabaseConfig();
    }
    databaseConfig_.qtDriver = helper.readString("Database/qtDriver");
    if (databaseConfig_.qtDriver.isEmpty()) {
        errorMessage = QStringLiteral("数据库配置文件中缺少必填字段:qtDriver");
        return DatabaseConfig();
    }
    // odbcDriver
    if (!helper.contains("Database/odbcDriver")) {
        errorMessage = QStringLiteral("数据库配置文件中缺少必填字段:odbcDriver");
        return DatabaseConfig();
    }
    databaseConfig_.odbcDriver = helper.readString("Database/odbcDriver");
    if (databaseConfig_.odbcDriver.isEmpty()) {
        errorMessage = QStringLiteral("数据库配置文件中缺少必填字段:odbcDriver");
        return DatabaseConfig();
    }
    // hostName
    if (!helper.contains("Database/hostName")) {
        errorMessage = QStringLiteral("数据库配置文件中缺少必填字段:hostName");
        return DatabaseConfig();
    }
    databaseConfig_.hostName = helper.readString("Database/hostName");
    if (databaseConfig_.hostName.isEmpty()) {
        errorMessage = QStringLiteral("数据库配置文件中缺少必填字段:hostName");
        return DatabaseConfig();
    }
    // port
    if (!helper.contains("Database/port")) {
        errorMessage = QStringLiteral("数据库配置文件中缺少必填字段:port");
        return DatabaseConfig();
    }
    auto portResult = helper.readValue("Database/port", ConfigValueType::Int);
    if (!portResult.success) {
        errorMessage = QStringLiteral("数据库配置文件中port字段: %1无效").arg(portResult.value.toString());
        return DatabaseConfig();
    }
    databaseConfig_.port = portResult.value.toInt();
    if (databaseConfig_.port <= 0) {
        errorMessage = QStringLiteral("数据库配置文件中port字段: %1无效").arg(portResult.value.toString());
        return DatabaseConfig();
    }
    // userName
    if (!helper.contains("Database/userName")) {
        errorMessage = QStringLiteral("数据库配置文件中缺少必填字段:userName");
        return DatabaseConfig();
    }
    databaseConfig_.userName = helper.readString("Database/userName");
    if (databaseConfig_.userName.isEmpty()) {
        errorMessage = QStringLiteral("数据库配置文件中缺少必填字段:userName");
        return DatabaseConfig();
    }
    // password
    if (!helper.contains("Database/password")) {
        errorMessage = QStringLiteral("数据库配置文件中缺少必填字段:password");
        return DatabaseConfig();
    }
    databaseConfig_.password = helper.readString("Database/password");
    if (databaseConfig_.password.isEmpty()) {
        errorMessage = QStringLiteral("数据库配置文件中缺少必填字段:password");
        return DatabaseConfig();
    }
    // databaseName
    if (!helper.contains("Database/databaseName")) {
        errorMessage = QStringLiteral("数据库配置文件中缺少必填字段:databaseName");
        return DatabaseConfig();
    }
    databaseConfig_.databaseName = helper.readString("Database/databaseName");
    if (databaseConfig_.databaseName.isEmpty()) {
        errorMessage = QStringLiteral("数据库配置文件中缺少必填字段:databaseName");
        return DatabaseConfig();
    }
    // connectionTimeoutMs
    auto connectionTimeoutMsResult = helper.readValue("Database/connectionTimeoutMs", ConfigValueType::Int);
    if (!connectionTimeoutMsResult.success) {
        errorMessage = QStringLiteral("数据库配置文件中connectionTimeoutMs字段: %1无效").arg(connectionTimeoutMsResult.value.toString());
        return DatabaseConfig();
    }
    databaseConfig_.connectionTimeoutMs = connectionTimeoutMsResult.value.toInt();
    if (databaseConfig_.connectionTimeoutMs <= 0) {
        errorMessage = QStringLiteral("数据库配置文件中connectionTimeoutMs字段: %1无效").arg(connectionTimeoutMsResult.value.toString());
        return DatabaseConfig();
    }
    // healthCheckIntervalMs
    auto healthCheckIntervalMsResult = helper.readValue("Database/healthCheckIntervalMs", ConfigValueType::Int);
    if (!healthCheckIntervalMsResult.success) {
        errorMessage = QStringLiteral("数据库配置文件中healthCheckIntervalMs字段: %1无效").arg(healthCheckIntervalMsResult.value.toString());
        return DatabaseConfig();
    }
    databaseConfig_.healthCheckIntervalMs = healthCheckIntervalMsResult.value.toInt();
    if (databaseConfig_.healthCheckIntervalMs <= 0) {
        errorMessage = QStringLiteral("数据库配置文件中healthCheckIntervalMs字段: %1无效").arg(healthCheckIntervalMsResult.value.toString());
        return DatabaseConfig();
    }
    // queueCapacity
    auto queueCapacityResult = helper.readValue("Database/queueCapacity", ConfigValueType::Int);
    if (!queueCapacityResult.success) {
        errorMessage = QStringLiteral("数据库配置文件中queueCapacity字段: %1无效").arg(queueCapacityResult.value.toString());
        return DatabaseConfig();
    }
    databaseConfig_.queueCapacity = queueCapacityResult.value.toInt();
    if (databaseConfig_.queueCapacity <= 0) {
        errorMessage = QStringLiteral("数据库配置文件中queueCapacity字段: %1无效").arg(queueCapacityResult.value.toString());
        return DatabaseConfig();
    }
    // maxResultRows
    auto maxResultRowsResult = helper.readValue("Database/maxResultRows", ConfigValueType::Int);
    if (!maxResultRowsResult.success) {
        errorMessage = QStringLiteral("数据库配置文件中maxResultRows字段: %1无效").arg(maxResultRowsResult.value.toString());
        return DatabaseConfig();
    }
    databaseConfig_.maxResultRows = maxResultRowsResult.value.toInt();
    if (databaseConfig_.maxResultRows <= 0) {
        errorMessage = QStringLiteral("数据库配置文件中maxResultRows字段: %1无效").arg(maxResultRowsResult.value.toString());
        return DatabaseConfig();
    }
    // shutdownDrainTimeoutMs
    auto shutdownDrainTimeoutMsResult = helper.readValue("Database/shutdownDrainTimeoutMs", ConfigValueType::Int);
    if (!shutdownDrainTimeoutMsResult.success) {
        errorMessage = QStringLiteral("数据库配置文件中shutdownDrainTimeoutMs字段: %1无效").arg(shutdownDrainTimeoutMsResult.value.toString());
        return DatabaseConfig();
    }
    databaseConfig_.shutdownDrainTimeoutMs = shutdownDrainTimeoutMsResult.value.toInt();
    if (databaseConfig_.shutdownDrainTimeoutMs <= 0) {
        errorMessage = QStringLiteral("数据库配置文件中shutdownDrainTimeoutMs字段: %1无效").arg(shutdownDrainTimeoutMsResult.value.toString());
        return DatabaseConfig();
    }
    // 最后对数据库配置进行验证
    if (!databaseConfig_.isValid(errorMessage)) {
        errorMessage = QStringLiteral("数据库配置无效: %1").arg(errorMessage);
        return DatabaseConfig();
    }
    return databaseConfig_;
}
