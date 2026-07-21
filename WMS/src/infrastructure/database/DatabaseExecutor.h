// 数据库执行器 DatabaseExecutor
#pragma once

#include "DatabaseTypes.h"

#include <QObject>
#include <QPointer>
#include <QQueue>
#include <QThread>

class DatabaseWorker;
class QTimer;
// 数据库执行器:负责提交数据库任务并管理数据库工作线程

class DatabaseExecutor final : public QObject {
    Q_OBJECT
public:
    explicit DatabaseExecutor(const DatabaseConfig& config, QObject* parent = nullptr);
    ~DatabaseExecutor() override;

    [[nodiscard]] QUuid submitTask(DatabaseTask task); // 提交数据库任务
    [[nodiscard]] DatabaseExecutorState state() const noexcept; // 获取执行器状态

public slots:
    void shutdown(); // 关闭

signals:
    void ready(); // 就绪
    void startupFailed(const DatabaseError& error); // 启动失败
    void taskFinished(const DatabaseResult& result); // 任务完成
    void stateChanged(DatabaseExecutorState state); // 状态改变
    void shutdownFinished(); // 关闭完成

private slots:
    void onWorkerInitialized(); // 数据库工作线程初始化完成
    void onWorkerInitializationFailed(const DatabaseError& error); // 数据库工作线程初始化失败
    void onTaskCompleted(const DatabaseResult& result); // 任务完成
    void onDrainTimeout(); // 超时
    void onWorkerShutdownCompleted(); // 数据库工作线程关闭完成
    void onWorkerThreadFinished(); // 数据库工作线程完成

private:
    void submitTaskInOwnerThread(const DatabaseTask& task); // 提交数据库任务到主线程
    void dispatchNext(); // 分发下一个任务
    void requestWorkerShutdown(); // 请求数据库工作线程关闭
    void cancelPending(DatabaseErrorCode code, const QString& message); // 取消待处理任务
    void setState(DatabaseExecutorState state); // 设置执行器状态
    [[nodiscard]] DatabaseResult rejectedResult(const QUuid& requestId, DatabaseResultStatus status, DatabaseErrorCode code, const QString& message) const;

    DatabaseConfig config_; // 数据库配置
    QThread workerThread_; // 工作线程
    QPointer<DatabaseWorker> worker_; // 数据库工作线程指针
    QQueue<DatabaseTask> pendingTasks_; // 待处理任务队列
    QTimer* drainTimer_ { nullptr }; // 排空超时定时器
    DatabaseExecutorState state_ { DatabaseExecutorState::Starting }; // 执行器状态
    bool taskInFlight_ { false }; // 任务是否执行中
    bool workerShutdownRequested_ { false }; // 数据库工作线程关闭请求
    bool shutdownFinishedEmitted_ { false }; // 关闭完成信号是否发送
};
