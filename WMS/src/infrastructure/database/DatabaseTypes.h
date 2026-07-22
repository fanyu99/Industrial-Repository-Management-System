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
#pragma once

#include <QMetaType>
#include <QString>
#include <QStringList>
#include <QUuid>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#include <QtCore/QList>
#include <optional>
// 1.语句类型
enum class StatementType {
    Query, // 查询(SELECT等),业务层面需要注意处理rows,columns
    Command // 命令(INSERT,UPDATE,DELETE等),业务层面需要注意影响的行数:AffectedRows和最后插入的ID:LastInsertId
};
// 2.数据库任务类型
enum class DatabaseTaskType { Single,
    Transaction };
// 3.数据库错误码
enum class DatabaseErrorCode {
    None, // 无错误
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
enum class DatabaseExecutorState {
    Starting, // 启动中
    Ready, // 就绪
    ShuttingDown, // 关闭中
    Stopped, // 已停止
    Failed // 失败
};
// 5.数据库配置
struct DatabaseConfig {
    QString qtDriver { QStringLiteral("") }; // Qt 数据库驱动(使用QStringLiteral直接作为静态常量接收字面量,提高性能,仅读)
    QString odbcDriver { QStringLiteral("") }; // ODBC 数据库驱动
    QString hostName { QStringLiteral("") }; // 主机名
    int port { 0 }; // 端口号
    QString userName{""}; // 用户名
    QString password{""}; // 密码
    QString databaseName{""}; // 数据库名
    int connectionTimeoutMs { 0 }; // 连接超时时间
    int healthCheckIntervalMs { 0 }; // 健康检查间隔
    int queueCapacity { 0 }; // 任务队列容量
    int maxResultRows { 0 }; // 最大结果行数
    int shutdownDrainTimeoutMs { 0 }; // 关闭超时时间
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
    [[nodiscard]] bool isValid(QString& errormessage) const
    {
        if(qtDriver != QStringLiteral("QODBC"))
        {
            errormessage = QStringLiteral("Qt 数据库驱动必须为 QODBC");
            return false;
        }
        if(odbcDriver.trimmed().isEmpty())
        {
            errormessage = QStringLiteral("ODBC 数据库驱动不能为空");
            return false;
        }
        if(hostName.trimmed().isEmpty())
        {
            errormessage = QStringLiteral("主机名不能为空");
            return false;
        }
        if(port <= 0 || port > 65535)
        {
            errormessage = QStringLiteral("端口号必须在 1-65535 范围内");
            return false;
        }
        if(userName.trimmed().isEmpty())
        {
            errormessage = QStringLiteral("用户名不能为空");
            return false;
        }
        if(password.trimmed().isEmpty())
        {
            errormessage = QStringLiteral("密码不能为空");
            return false;
        }
        if(databaseName.trimmed().isEmpty())
        {
            errormessage = QStringLiteral("数据库名不能为空");
            return false;
        }
        if(connectionTimeoutMs <= 0)
        {
            errormessage = QStringLiteral("连接超时时间必须大于 0");
            return false;
        }
        if(healthCheckIntervalMs <= 0)
        {
            errormessage = QStringLiteral("健康检查间隔必须大于 0");
            return false;
        }
        if(queueCapacity <= 0)
        {
            errormessage = QStringLiteral("任务队列容量必须大于 0");
            return false;
        }
        if(maxResultRows <= 0)
        {
            errormessage = QStringLiteral("最大结果行数必须大于 0");
            return false;
        }
        if(shutdownDrainTimeoutMs <= 0)
        {
            errormessage = QStringLiteral("关闭超时时间必须大于等于 0");
            return false;
        }
        return true;
    }
};
// 6.数据库SQL语句
struct DatabaseStatement {
    StatementType type { StatementType::Query }; // 语句类型
    QString sql; // SQL 语句
    QVariantMap parameters; // 参数
    // expectedAffectedRows 仅用于命令语句,如果optional为空,不参与检查
    std::optional<int> expectedAffectedRows; // 用于命令语句的预期影响行数,作为守卫,检查语句结果是否符合预期
    // 强制检查是否有效
    [[nodiscard]] bool isValid() const
    {
        if (sql.trimmed().isEmpty()
            || (type != StatementType::Query && type != StatementType::Command)) {
            return false;
        } else if (type == StatementType::Query && expectedAffectedRows.has_value())
            return false;
        else if (type == StatementType::Command && expectedAffectedRows.has_value() && expectedAffectedRows.value() < 0)
            return false;
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
    DatabaseErrorCode code { DatabaseErrorCode::None }; // DatabaseErrorCode错误码
    QString message; // 错误消息
    QString nativeErrorCode; // 本地错误码
    QString databaseText; // 数据库错误文本
    QString driverText; // 驱动错误文本
};
// 9.语句结果
struct StatementResult {
    QStringList columns; // 列名
    QList<QVariantList> rows; // 行数据
    qint64 affectedRows { 0 }; // 真实影响的行数(用于affctedRows守卫)
    QVariant lastInsertId; // 最后插入 ID
};
// 10. 数据库事务执行结果的状态
enum class DatabaseResultStatus {
    SingleSucceeded, // 单条语句成功
    Committed, // 已提交
    SqlExecutionFailed, // SQL执行失败
    TransactionBeginFailed, // 事务开始失败
    TransactionCommitFailed, // 事务提交失败
    TransactionRolledBack, // 事务进行回滚
    TransactionRollbackFailed, // 事务回滚失败
    AffectedRowsConditionNotMet, // 影响行数不符合预期条件
    Canceled, // 已取消
    Timeout // 超时
};
// 11.数据库结果
struct DatabaseResult { // 注意:表示已经尝试执行并获得数据库的结果,是否提交需要查看DatabaseResultStatus
    QUuid requestId; // 请求ID
    DatabaseResultStatus status { DatabaseResultStatus::SqlExecutionFailed }; // 结果状态
    QList<StatementResult> statementResults; // 语句结果
    int failedStatementIndex { -1 }; // 失败语句索引
    DatabaseError error; // 错误信息
    bool isCommitted() const // 是否提交
    {
        return status == DatabaseResultStatus::Committed;
    }
    bool isSucceeded() const // 判断是否成功
    {
        return status == DatabaseResultStatus::SingleSucceeded || isCommitted();
    }
    bool isRolledback() const // 是否回滚
    {
        return status == DatabaseResultStatus::TransactionRolledBack ||
            status == DatabaseResultStatus::TransactionRollbackFailed ||
            status == DatabaseResultStatus::AffectedRowsConditionNotMet ||
            status == DatabaseResultStatus::TransactionCommitFailed ||
            status == DatabaseResultStatus::Timeout ||
            status == DatabaseResultStatus::SqlExecutionFailed;
    }
};

// 注册元类型,用于信号槽传递等
// 只要自定义的类型会经过跨线程的信号槽,就要注册元类型
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