#include "MySqlAuthRepository.h"
#include <QHash>
#include <QPointer>
#include <QString>
#include <QUuid>
#include <utility>

MySqlAuthRepository::MySqlAuthRepository(DatabaseExecutor& executor, QObject* parent)
    : QObject(parent)
    , executor_(executor)
{
    connect(&executor_, &DatabaseExecutor::taskFinished, this, &MySqlAuthRepository::onTaskFinished);
}
// 处理数据库任务完成信号
void MySqlAuthRepository::onTaskFinished(const DatabaseResult& result)
{
    auto ite = pending_.find(result.requestId);
    if (ite == pending_.end()) {
        return;
    }
    auto pending = std::move(ite.value());
    pending_.erase(ite);
    if (pending.owner.isNull())
        return;
    if (pending.handler)
        pending.handler(result);
}
// 将数据库错误映射为应用错误
AppError MySqlAuthRepository::mapDatabaseErrorToAppError(DatabaseError error) const
{
    switch (error.code) {
    case DatabaseErrorCode::QueueFull:
        return AppError::repositoryFailure(QStringLiteral("数据库任务队列已满,请稍后重试"));
    case DatabaseErrorCode::InvalidTask:
        return AppError::repositoryFailure(QStringLiteral("任务无效"));
    case DatabaseErrorCode::ShuttingDown:
        return AppError {
            AppErrorCategory::System,
            AppErrorCode::UnknownError,
            QStringLiteral("系统正在关闭,请稍后重试")
        };
    case DatabaseErrorCode::PrepareFailed:
    case DatabaseErrorCode::ExecuteFailed:
    case DatabaseErrorCode::TransactionFailed:
        return AppError::databaseFailure(QStringLiteral("数据库操作失败"));
    case DatabaseErrorCode::Cancelled:
        return AppError::databaseFailure(QStringLiteral("操作已取消"));
    case DatabaseErrorCode::ConnectionFailed:
        return AppError::databaseFailure(QStringLiteral("数据库连接失败"));
    case DatabaseErrorCode::ResultTooLarge:
        return AppError::databaseFailure(QStringLiteral("查询结果集过大"));
    default:
        return AppError {
            AppErrorCategory::Database,
            AppErrorCode::UnknownError,
            QStringLiteral("数据库错误")
        };
    }
}
// 将数据库结果映射为认证凭证记录
std::optional<AuthCredentialRecord> MySqlAuthRepository::mapStatementResultToFindCredentialResult(
    const QStringList& columns,
    const QVariantList& row) const
{
    AuthCredentialRecord record;
    for (int i = 0; i < columns.size() && i < row.size(); ++i) {
        const QString& colName = columns[i];
        if (colName == "userId") {
            record.userId = row[i].toUInt();
        } else if (colName == "userName") {
            record.userName = row[i].toString();
        } else if (colName == "realName") {
            record.realName = row[i].toString();
        } else if (colName == "role") {
            record.role = static_cast<Role>(row[i].toInt());
        } else if (colName == "active") {
            record.active = row[i].toBool();
        } else if (colName == "hashName") {
            record.passwordHashRecord.hashName = row[i].toString();
        } else if (colName == "algorithmVersion") {
            record.passwordHashRecord.algorithmVersion = row[i].toInt();
        } else if (colName == "iterations") {
            record.passwordHashRecord.iterations = row[i].toInt();
        } else if (colName == "salt") {
            record.passwordHashRecord.salt = row[i].toByteArray();
        } else if (colName == "hash") {
            record.passwordHashRecord.hash = row[i].toByteArray();
        }
    }
    if (!record.passwordHashRecord.isValid()) {
        return std::nullopt;
    }
    return record;
}
// 通过用户名查询认证凭证记录
void MySqlAuthRepository::findCredentialByUserName(const QString& userName,
    QObject* owner, FindCredentialCallback callback)
{
    // 校验参数
    QPointer<QObject> ownerPtr(owner);
    if (ownerPtr.isNull() || !callback)
        return;
    if (userName.trimmed().isEmpty()) {
        callback(FindCredentialResult { false,
            std::nullopt,
            AppError { AppErrorCategory::Auth,
                AppErrorCode::InvalidInput,
                QStringLiteral("用户名为空") } });
        return;
    }
    // 创建语句
    DatabaseStatement statement;
    statement.type = StatementType::Query;
    statement.sql = QStringLiteral(
        R"(SELECT id AS userId, username AS userName, real_name AS realName, role, is_active AS active, hash_name AS hashName, algorithm_version AS algorithmVersion, iterations, salt, hash FROM users WHERE username = :userName OR real_name = :userName LIMIT 1)");
    statement.parameters.insert("userName", userName);
    // 创建任务
    DatabaseTask task;
    task.type = DatabaseTaskType::Single;
    task.statements.append(statement);
    task.requestId = QUuid::createUuid();
    // 添加到pending_
    pending_.insert(
        task.requestId,
        PendingRequest { ownerPtr,
            [this, ownerPtr, callback = std::move(callback)](
                const DatabaseResult& result) {
                if (ownerPtr.isNull() || !callback)
                    return;
                if (!result.isSucceeded()) {
                    callback(FindCredentialResult {
                        false, std::nullopt,
                        mapDatabaseErrorToAppError(result.error) });
                    return;
                }
                if (result.statementResults.isEmpty()) {
                    callback(FindCredentialResult {
                        false, std::nullopt,
                        AppError::repositoryFailure(QStringLiteral("数据库结果为空")) });
                    return;
                }
                const auto& statementResult = result.statementResults.constFirst();
                if (statementResult.rows.isEmpty()) {
                    callback(FindCredentialResult {
                        true, std::nullopt, std::nullopt });
                    return;
                }
                const auto& columns = statementResult.columns;
                const auto& row = statementResult.rows.constFirst();
                if (row.isEmpty()) {
                    callback(FindCredentialResult {
                        true, std::nullopt, AppError{AppErrorCategory::Auth, AppErrorCode::UserNotFound,QStringLiteral("用户不存在") }});
                    return;
                }
                const auto findCredentialResult = mapStatementResultToFindCredentialResult(columns, row);
                if (!findCredentialResult.has_value()) {
                    callback(FindCredentialResult {
                        false, std::nullopt,
                        AppError::repositoryFailure(QStringLiteral("用户凭证映射失败")) });
                    return;
                }
                callback(FindCredentialResult { true, findCredentialResult.value(), std::nullopt });
            } });
    executor_.submitTask(task);
}

void MySqlAuthRepository::findCredentialByUserId(quint32 userId,
    QObject* owner, FindCredentialCallback callback)
{
    QPointer<QObject> ownerPtr(owner);
    if (ownerPtr.isNull() || !callback)
        return;
    if (userId == 0) {
        callback(FindCredentialResult { false,
            std::nullopt,
            AppError { AppErrorCategory::Auth,
                AppErrorCode::InvalidInput,
                QStringLiteral("用户ID无效") } });
        return;
    }
    DatabaseStatement statement;
    statement.type = StatementType::Query;
    statement.sql = QStringLiteral(
        R"(SELECT id AS userId, username AS userName, real_name AS realName, role, is_active AS active, hash_name AS hashName, algorithm_version AS algorithmVersion, iterations, salt, hash FROM users WHERE id = :userId LIMIT 1)");
    statement.parameters.insert("userId", userId);

    DatabaseTask task;
    task.type = DatabaseTaskType::Single;
    task.statements.append(statement);
    task.requestId = QUuid::createUuid();

    pending_.insert(
        task.requestId,
        PendingRequest {
            ownerPtr,
            [this, ownerPtr, callback = std::move(callback)](
                const DatabaseResult& result) {
                if (ownerPtr.isNull() || !callback)
                    return;
                if (!result.isSucceeded()) {
                    callback(FindCredentialResult {
                        false, std::nullopt,
                        mapDatabaseErrorToAppError(result.error) });
                    return;
                }
                if (result.statementResults.isEmpty()) {
                    callback(FindCredentialResult {
                        false, std::nullopt,
                        AppError::repositoryFailure(QStringLiteral("数据库结果为空")) });
                    return;
                }
                const auto& statementResult = result.statementResults.constFirst();
                if (statementResult.rows.isEmpty()) {
                    callback(FindCredentialResult {
                        true, std::nullopt, std::nullopt });
                    return;
                }
                const auto& columns = statementResult.columns;
                const auto& row = statementResult.rows.constFirst();
                if (row.isEmpty()) {
                    callback(FindCredentialResult {
                        true, std::nullopt, AppError{AppErrorCategory::Auth, AppErrorCode::UserNotFound,QStringLiteral("用户不存在") }});
                    return;
                }
                const auto findCredentialResult = mapStatementResultToFindCredentialResult(columns, row);
                if (!findCredentialResult.has_value()) {
                    callback(FindCredentialResult {
                        false, std::nullopt,
                        AppError::repositoryFailure(QStringLiteral("用户凭证映射失败")) });
                    return;
                }
                callback(FindCredentialResult { true, findCredentialResult.value(), std::nullopt });
            } });
    executor_.submitTask(task);
}