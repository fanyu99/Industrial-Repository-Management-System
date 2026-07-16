#include "DatabaseWorker.h"

// 数据库工作线程:负责执行数据库任务
/*
1.数据库初始化:根据配置初始化数据库连接
2.数据库任务执行:根据任务类型执行数据库操作
3.数据库关闭:关闭数据库连接
*/


// 数据库工作线日志分类
Q_LOGGING_CATEGORY(databaseWorkerLog, "wms.database.worker")

namespace {
QString odbcValue(QString value)
{
    value.replace(QLatin1Char('}'), QStringLiteral("}}"));
    return QStringLiteral("{%1}").arg(value);
}
}
// 构造
DatabaseWorker::DatabaseWorker(QObject* parent)
    : QObject(parent)
{
}
// 析构:关闭数据库连接
DatabaseWorker::~DatabaseWorker()
{
    closeConnection();
}

// 初始化数据库工作线程
void DatabaseWorker::initialize(const DatabaseConfig& config)
{
    // 如果已经初始化/关闭,直接返回
    if (initialized_ || shuttingDown_) {
        return;
    }
    // 设置数据库配置
    config_ = config;
    // 如果配置无效
    if (!config_.isValid()) {
        DatabaseError error;
        error.code = DatabaseErrorCode::ConnectionFailed;
        error.message = QStringLiteral("数据库配置无效，第一版仅支持 QODBC");
        emit initializationFailed(error);// 释放信号,输出错误信息
        return;
    }

    connectionName_ = QStringLiteral("wms_database_worker_%1") // 字面量
                          .arg(QUuid::createUuid().toString(QUuid::WithoutBraces)); // 生成Uuid
    database_ = QSqlDatabase::addDatabase(config_.qtDriver, connectionName_); // 创建数据库连接
    database_.setDatabaseName(QStringLiteral("DRIVER=%1;SERVER=%2;PORT=%3;DATABASE=%4;") // 设置数据库配置
                                  .arg(odbcValue(config_.odbcDriver),
                                      odbcValue(config_.hostName),
                                      QString::number(config_.port),
                                      odbcValue(config_.databaseName)));
    database_.setUserName(config_.userName);
    database_.setPassword(config_.password);
    const int timeoutSeconds = qMax(1, (config_.connectionTimeoutMs + 999) / 1000); // 超时时间sec
    database_.setConnectOptions(QStringLiteral("SQL_ATTR_LOGIN_TIMEOUT=%1").arg(timeoutSeconds)); // 设置登录超时时间

    // 如果连接未能打开
    DatabaseError error;
    if (!ensureConnectionOpen(error)) {
        closeConnection(); // 关闭连接
        emit initializationFailed(error); // 释放信号,输出错误信息
        return;
    }
    // 启动健康检查定时器
    healthTimer_ = new QTimer(this);
    healthTimer_->setInterval(config_.healthCheckIntervalMs);
    connect(healthTimer_, &QTimer::timeout, this, &DatabaseWorker::checkHealth); // 连接信号,槽函数
    healthTimer_->start();
    initialized_ = true; 
    emit initialized(); // 初始化完成
}
// 执行数据库任务
void DatabaseWorker::executeTask(const DatabaseTask& task)
{
    // 如果正在关闭/任务无效
    if (shuttingDown_ || !task.isValid()) {
        DatabaseResult result;
        result.requestId = task.requestId;
        result.error.code = shuttingDown_ ? DatabaseErrorCode::ShuttingDown
                                          : DatabaseErrorCode::InvalidTask;
        result.error.message = shuttingDown_ ? QStringLiteral("数据库工作线程正在关闭")
                                             : QStringLiteral("数据库任务无效");
        emit taskCompleted(result); // 释放完成信号/任务无效信号
        return;
    }
    // 正在执行任务
    busy_ = true;
    DatabaseError connectionError;
    DatabaseResult result;
    // 如果还未初始化/数据库连接无效
    if (!initialized_ || !ensureConnectionOpen(connectionError)) {
        result.requestId = task.requestId;
        result.error = connectionError;
        // 如果连接无效时并未设置其他错误
        if (result.error.code == DatabaseErrorCode::None) { 
            result.error.code = DatabaseErrorCode::ConnectionFailed;
            result.error.message = QStringLiteral("数据库工作线程尚未初始化");
        }
    }
    // 如果是事务
    else if (task.type == DatabaseTaskType::Transaction) {
        result = executeTransaction(task);
    }
    // 如果是单语句
    else {
        result = executeSingle(task);
    }
    // 执行完成
    busy_ = false;
    emit taskCompleted(result);
}
// 关闭数据库工作线程
void DatabaseWorker::shutdown()
{
    // 避免重复关闭
    if (shuttingDown_) {
        return;
    }
    shuttingDown_ = true; 
    closeConnection(); // 关闭连接
    emit shutdownCompleted(); // 释放信号
}
// 检查健康状态
void DatabaseWorker::checkHealth()
{
    // 如果正在执行/正在关闭/未初始化,直接返回
    if (busy_ || shuttingDown_ || !initialized_) {
        return;
    }
    DatabaseError error;
    // 如果连接无效
    if (!ensureConnectionOpen(error)) {
        // 输出数据库工作线程日志
        qCWarning(databaseWorkerLog) << error.message << error.databaseText;
        return;
    }
    // 进行健康查询
    QSqlQuery query(database_);
    if (!query.exec(QStringLiteral("SELECT 1"))) {
        qCWarning(databaseWorkerLog) << "数据库健康检查失败:" << query.lastError().text();
        database_.close(); // 关闭连接
    }
}
// 数据库连接,成功返回true,失败false,并设置错误信息(如果有效,则为None)
bool DatabaseWorker::ensureConnectionOpen(DatabaseError& error)
{
    // 连接无效
    if (!database_.isValid()) {
        error.code = DatabaseErrorCode::ConnectionFailed;
        error.message = QStringLiteral("数据库连接对象无效");
        return false;
    }
    if (database_.isOpen() || database_.open()) {
        return true;
    }
    error = sqlError(DatabaseErrorCode::ConnectionFailed,
                     QStringLiteral("连接数据库失败"), database_.lastError());
    return false;
}
// 执行单语句
DatabaseResult DatabaseWorker::executeSingle(const DatabaseTask& task)
{
    DatabaseResult result;
    result.requestId = task.requestId;
    StatementResult statementResult;
    if (!executeStatement(task.statements.constFirst(), statementResult, result.error)) {
        result.failedStatementIndex = 0;
        return result;
    }
    result.statementResults.append(statementResult);
    result.success = true;
    return result;
}

DatabaseResult DatabaseWorker::executeTransaction(const DatabaseTask& task)
{
    DatabaseResult result;
    result.requestId = task.requestId;
    if (!database_.transaction()) {
        result.error = sqlError(DatabaseErrorCode::TransactionFailed,
                                QStringLiteral("启动数据库事务失败"), database_.lastError());
        return result;
    }

    for (qsizetype index = 0; index < task.statements.size(); ++index) {
        StatementResult statementResult;
        DatabaseError statementError;
        if (!executeStatement(task.statements.at(index), statementResult, statementError)) {
            database_.rollback();
            result.statementResults.clear();
            result.failedStatementIndex = static_cast<int>(index);
            result.error = statementError;
            return result;
        }
        result.statementResults.append(statementResult);
    }

    if (!database_.commit()) {
        const QSqlError commitError = database_.lastError();
        database_.rollback();
        result.statementResults.clear();
        result.error = sqlError(DatabaseErrorCode::TransactionFailed,
                                QStringLiteral("提交数据库事务失败"), commitError);
        return result;
    }
    result.success = true;
    return result;
}
// 执行语句
bool DatabaseWorker::executeStatement(const DatabaseStatement& statement,
                                      StatementResult& result,
                                      DatabaseError& error)
{
    QSqlQuery query(database_);
    // 如果query预处理失败
    if (!query.prepare(statement.sql)) {
        error = sqlError(DatabaseErrorCode::PrepareFailed,
                         QStringLiteral("预处理 SQL 失败"), query.lastError());
        return false;
    }
    for (auto iterator = statement.parameters.cbegin();
         iterator != statement.parameters.cend(); ++iterator) {
        QString placeholder = iterator.key();
        if (!placeholder.startsWith(QLatin1Char(':'))) {
            placeholder.prepend(QLatin1Char(':'));
        }
        query.bindValue(placeholder, iterator.value()); // 绑定参数
    }
    if (!query.exec()) {
        error = sqlError(DatabaseErrorCode::ExecuteFailed,
                         QStringLiteral("执行 SQL 失败"), query.lastError());
        return false;
    }

    result.affectedRows = query.numRowsAffected();
    result.lastInsertId = query.lastInsertId();
    if (statement.type != StatementType::Query) {
        return true;
    }

    // 添加处理结果
    const QSqlRecord record = query.record();
    for (int column = 0; column < record.count(); ++column) {
        result.columns.append(record.fieldName(column));
    }
    while (query.next()) {
        // 结果过大,返回错误
        if (result.rows.size() >= config_.maxResultRows) {
            result = StatementResult {};
            error.code = DatabaseErrorCode::ResultTooLarge;
            error.message = QStringLiteral("查询结果超过最大允许行数 %1，请使用 LIMIT 或分页")
                                .arg(config_.maxResultRows);
            return false;
        }
        // 添加结果行
        QVariantList row;
        row.reserve(record.count());
        for (int column = 0; column < record.count(); ++column) {
            row.append(query.value(column));
        }
        result.rows.append(row);
    }
    return true;
}
// 数据库的sql错误码转换为数据库错误
DatabaseError DatabaseWorker::sqlError(DatabaseErrorCode code, const QString& message,const QSqlError& error) const
{
    DatabaseError result;
    result.code = code;
    result.message = message;
    result.nativeErrorCode = error.nativeErrorCode();
    result.databaseText = error.databaseText();
    result.driverText = error.driverText();
    return result;
}
// 关闭数据库连接
void DatabaseWorker::closeConnection()
{
    // 停止定时器
    if (healthTimer_ != nullptr) {
        healthTimer_->stop();
    }
    // 关闭连接
    const QString name = connectionName_;
    if (database_.isValid()) {
        database_.close();
    }
    // 置空连接,防止连接名泄露
    database_ = QSqlDatabase();
    // 删除数据库连接
    if (!name.isEmpty() && QSqlDatabase::contains(name)) {
        QSqlDatabase::removeDatabase(name);
    }
    connectionName_.clear();
    initialized_ = false;
}
