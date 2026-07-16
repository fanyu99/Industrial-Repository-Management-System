#pragma once

#include <QList>
#include <QMetaType>
#include <QString>
#include <QStringList>
#include <QUuid>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>

// 数据库类型头文件:
/*
1.StatementType 语句类型
2.DatabaseTaskType 数据库任务类型
3.DatabaseErrorCode 数据库错误码
4.DatabaseExecutorState 数据库执行器状态
5.DatabaseConfig 数据库配置
6.DatabaseStatement 数据库语句
7.DatabaseTask 数据库任务
8.DatabaseError 数据库错误
9.StatementResult 语句结果
10.DatabaseResult 数据库结果
*/


// 1.语句类型
enum class StatementType { Query,
    Command };
// 2.数据库任务类型
enum class DatabaseTaskType { Single,
    Transaction };
// 3.数据库错误码       
enum class DatabaseErrorCode {
    None,
    InvalidTask, // 无效任务
    QueueFull, // 队列满
    ShuttingDown, // 关闭中
    ConnectionFailed, // 连接失败
    PrepareFailed, // 预处理失败
    ExecuteFailed, // 语句执行失败
    TransactionFailed, // 事务失败
    ResultTooLarge, // 结果过大
    Cancelled // 已取消
};
// 4.数据库执行器状态
enum class DatabaseExecutorState { Starting,
    Ready, // 就绪
    ShuttingDown, // 关闭中
    Stopped, // 已停止
    Failed // 失败
};
// 5.数据库配置
struct DatabaseConfig {
    QString qtDriver { QStringLiteral("QODBC") }; // Qt 数据库驱动(使用QStringLiteral直接作为静态常量接收字面量,提高性能,仅读)
    QString odbcDriver { QStringLiteral("MariaDB Unicode") }; // ODBC 数据库驱动
    QString hostName { QStringLiteral("127.0.0.1") }; // 主机名
    int port { 3306 }; // 端口号
    QString userName; // 用户名
    QString password; // 密码
    QString databaseName; // 数据库名
    int connectionTimeoutMs { 5000 }; // 连接超时时间
    int healthCheckIntervalMs { 30000 }; // 健康检查间隔
    int queueCapacity { 1000 }; // 任务队列容量
    int maxResultRows { 10000 }; // 最大结果行数
    int shutdownDrainTimeoutMs { 3000 }; // 关闭超时时间
    // 强制检查配置是否有效
    [[nodiscard]] bool isValid() const
    {
        return qtDriver == QStringLiteral("QODBC")
            && !odbcDriver.trimmed().isEmpty()
            && !hostName.trimmed().isEmpty()
            && port > 0 && port <= 65535
            && !databaseName.trimmed().isEmpty()
            && connectionTimeoutMs > 0
            && healthCheckIntervalMs > 0
            && queueCapacity > 0
            && maxResultRows > 0
            && shutdownDrainTimeoutMs >= 0;
    }
};
// 6.数据库语句
struct DatabaseStatement {
    StatementType type { StatementType::Query }; // 语句类型
    QString sql; // SQL 语句
    QVariantMap parameters; // 参数
    // 强制检查是否有效
    [[nodiscard]] bool isValid() const
    {
        if (sql.trimmed().isEmpty()
            || (type != StatementType::Query && type != StatementType::Command)) {
            return false;
        }
        for (auto iterator = parameters.cbegin(); iterator != parameters.cend(); ++iterator) {
            if (iterator.key().trimmed().isEmpty()) {
                return false;
            }
        }
        return true;
    }
};
// 7.数据库任务
struct DatabaseTask {
    QUuid requestId; // 请求 ID
    DatabaseTaskType type { DatabaseTaskType::Single }; // 任务类型
    QList<DatabaseStatement> statements; // 语句
    // 强制检查是否有效
    [[nodiscard]] bool isValid() const
    {
        if ((type != DatabaseTaskType::Single && type != DatabaseTaskType::Transaction)
            || statements.isEmpty()
            || (type == DatabaseTaskType::Single && statements.size() != 1)) {
            return false;
        }
        for (const DatabaseStatement& statement : statements) {
            if (!statement.isValid()) {
                return false;
            }
        }
        return true;
    }
};
// 8.数据库错误
struct DatabaseError {
    DatabaseErrorCode code { DatabaseErrorCode::None }; // 错误码
    QString message; // 错误消息
    QString nativeErrorCode; // 本地错误码
    QString databaseText; // 数据库错误文本
    QString driverText; // 驱动错误文本
};
// 9.语句结果
struct StatementResult {
    QStringList columns; // 列名
    QList<QVariantList> rows; // 行数据
    qint64 affectedRows { 0 }; // 行数
    QVariant lastInsertId; // 最后插入 ID
};
// 10.数据库结果
struct DatabaseResult {
    QUuid requestId; // 请求ID
    bool success { false }; // 是否成功
    QList<StatementResult> statementResults; // 语句结果
    int failedStatementIndex { -1 }; // 失败语句索引
    DatabaseError error;
};

// 注册元类型,用于信号槽传递等
Q_DECLARE_METATYPE(StatementType)
Q_DECLARE_METATYPE(DatabaseTaskType)
Q_DECLARE_METATYPE(DatabaseErrorCode)
Q_DECLARE_METATYPE(DatabaseExecutorState)
Q_DECLARE_METATYPE(DatabaseConfig)
Q_DECLARE_METATYPE(DatabaseStatement)
Q_DECLARE_METATYPE(DatabaseTask)
Q_DECLARE_METATYPE(DatabaseError)
Q_DECLARE_METATYPE(StatementResult)
Q_DECLARE_METATYPE(DatabaseResult)
