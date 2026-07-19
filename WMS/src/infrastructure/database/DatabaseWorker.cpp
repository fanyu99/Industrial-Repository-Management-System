// 数据库工作对象 DatabaseWorker
#include "DatabaseWorker.h"

// 数据库工作对象日志分类
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
        emit initializationFailed(error); // 释放信号,输出错误信息
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
    // 如果正在关闭/任务无效 , 发送信号
    if (shuttingDown_ || !task.isValid()) {
        DatabaseResult result;
        result.success = false;
        result.requestId = task.requestId;
        result.error.code = shuttingDown_ ? DatabaseErrorCode::ShuttingDown
                                          : DatabaseErrorCode::InvalidTask;
        result.error.message = shuttingDown_ ? QStringLiteral("数据库工作线程正在关闭")
                                             : QStringLiteral("数据库任务无效");
        emit taskCompleted(result); // 释放信号
        return;
    }
    // 正在执行任务
    busy_ = true;
    DatabaseError connectionError;
    DatabaseResult result;
    // 如果还未初始化/数据库连接无效
    if (!this->initialized_ || !ensureConnectionOpen(connectionError)) {
        result.success = false;
        result.requestId = task.requestId;
        result.error = connectionError;
        // 如果没有错误,则为连接错误(工作线程未初始化)
        if (result.error.code == DatabaseErrorCode::None) {
            result.error.code = DatabaseErrorCode::ConnectionFailed;
            result.error.message = QStringLiteral("数据库工作线程尚未初始化");
        }
    }
    // 如果状态正常:
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
    // 如果连接无效,输出错误信息
    if (!ensureConnectionOpen(error)) {
        // 输出数据库工作对象日志
        qCWarning(databaseWorkerLog) << error.message<<" " << error.databaseText;
        return;
    }
    // 进行健康查询
    QSqlQuery query(database_);
    if (!query.exec(QStringLiteral("SELECT 1"))) {
        qCWarning(databaseWorkerLog) << "数据库健康检查失败:" << query.lastError().text();
        database_.close(); // 关闭连接
    }
}
// 数据库连接,成功返回true,失败false,通过引用设置错误信息(如果有效,则为None)
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

// 执行语句,引用返回结果和错误信息
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
    // 绑定参数
    for (auto iterator = statement.parameters.cbegin();
         iterator != statement.parameters.cend(); ++iterator) {
        QString placeholder = iterator.key();
        if (!placeholder.startsWith(QLatin1Char(':'))) {
            placeholder.prepend(QLatin1Char(':'));
        }
        query.bindValue(placeholder, iterator.value()); // 绑定参数
    }
    // 执行查询失败
    if (!query.exec()) {
        error = sqlError(DatabaseErrorCode::ExecuteFailed,
            QStringLiteral("执行 SQL 失败"), query.lastError());
        return false;
    }
    // 处理命令结果
    result.affectedRows = query.numRowsAffected();
    result.lastInsertId = query.lastInsertId();
    if (statement.type != StatementType::Query) {
        return true;
    }

    // 处理查询结果集rows,columns
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

// 执行单语句.返回数据库结果
DatabaseResult DatabaseWorker::executeSingle(const DatabaseTask& task)
{
    DatabaseResult result;
    result.requestId = task.requestId;
    StatementResult statementResult;
    if (!executeStatement(task.statements.constFirst(), statementResult, result.error)) {
        result.failedStatementIndex = 0;
        return result;
    }
    // 执行成功
    result.statementResults.append(statementResult);
    result.success = true;
    return result;
}

// 执行事务.返回数据库结果
DatabaseResult DatabaseWorker::executeTransaction(const DatabaseTask& task)
{
    DatabaseResult result;
    result.requestId = task.requestId;
    // 事务启动失败
    if (!database_.transaction()) {
        result.error = sqlError(DatabaseErrorCode::TransactionFailed,
            QStringLiteral("启动数据库事务失败"), database_.lastError());
        return result;
    }
    // 执行事务语句
    for (qsizetype i = 0; i < task.statements.size(); ++i) {
        StatementResult statementResult;
        DatabaseError statementError;
        // 执行语句失败,回滚事务并返回结果
        if (!executeStatement(task.statements.at(i), statementResult, statementError)) {
            database_.rollback();
            result.statementResults.clear();
            result.failedStatementIndex = static_cast<int>(i); // 设置失败语句的序号
            result.error = statementError;
            return result;
        }
        result.statementResults.append(statementResult); // 执行成功后添加到结果
    }
    // 提交事务失败,事务回滚
    if (!database_.commit()) {
        const QSqlError commitError = database_.lastError();
        database_.rollback();
        result.statementResults.clear();
        result.error = sqlError(DatabaseErrorCode::TransactionFailed,
            QStringLiteral("提交数据库事务失败"), commitError);
        return result;
    }
    // 事务成功
    result.success = true;
    return result;
}

// 数据库的QSqlError错误转换为数据库错误DatabaseError
DatabaseError DatabaseWorker::sqlError(DatabaseErrorCode code, const QString& message, const QSqlError& error) const
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
