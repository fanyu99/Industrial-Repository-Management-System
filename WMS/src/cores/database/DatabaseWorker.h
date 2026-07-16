#pragma once

#include "DatabaseTypes.h"
#include <QObject>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTimer>
#include <QMetaType>
#include <QString>
#include <QLoggingCategory>
#include <QSqlRecord>

class DatabaseWorker final : public QObject { // 设置终类
    Q_OBJECT
public:
    explicit DatabaseWorker(QObject* parent = nullptr);
    ~DatabaseWorker() override;
// 槽函数
public slots:
    void initialize(const DatabaseConfig& config); // 初始化工作线程
    void executeTask(const DatabaseTask& task); // 执行数据库任务
    void shutdown(); // 关闭工作线程
// 信号
signals:
    void initialized(); // 初始化完成
    void initializationFailed(const DatabaseError& error); // 初始化失败
    void taskCompleted(const DatabaseResult& result); // 任务完成
    void shutdownCompleted(); // 关闭完成
// 槽函数
private slots:
    void checkHealth(); // 检查数据库健康状态
private:
    [[nodiscard]] bool ensureConnectionOpen(DatabaseError& error); // 确保数据库连接打开
    [[nodiscard]] DatabaseResult executeSingle(const DatabaseTask& task); // 执行单条语句任务,返回结果
    [[nodiscard]] DatabaseResult executeTransaction(const DatabaseTask& task); // 执行事务任务,返回结果
    [[nodiscard]] bool executeStatement(const DatabaseStatement& statement, StatementResult& result, DatabaseError& error); // 执行数据库语句,返回状态
    [[nodiscard]] DatabaseError sqlError(DatabaseErrorCode code, const QString& message, const QSqlError& error) const; // sql错误码转换为数据库错误
    void closeConnection(); // 关闭数据库连接

    DatabaseConfig config_; // 数据库配置
    QSqlDatabase database_; // 数据库连接
    QString connectionName_; // 数据库连接名
    QTimer* healthTimer_ { nullptr }; // 健康检查定时器
    bool initialized_ { false }; // 是否初始化
    bool busy_ { false }; // 是否正在执行任务
    bool shuttingDown_ { false }; // 是否正在关闭
};
