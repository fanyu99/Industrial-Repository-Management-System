// 数据库执行器 DatabaseExecutor
#include "DatabaseExecutor.h"

#include "DatabaseWorker.h"

#include <QLoggingCategory>
#include <QMetaObject>
#include <QThread>
#include <QTimer>
#include <utility>
// 数据库执行器日志分类
Q_LOGGING_CATEGORY(databaseExecutorLog, "wms.database.executor")

// 注册数据库元类型
namespace {
void registerDatabaseMetaTypes()
{
    qRegisterMetaType<StatementType>();
    qRegisterMetaType<DatabaseTaskType>();
    qRegisterMetaType<DatabaseErrorCode>();
    qRegisterMetaType<DatabaseExecutorState>();
    qRegisterMetaType<DatabaseConfig>();
    qRegisterMetaType<DatabaseStatement>();
    qRegisterMetaType<DatabaseTask>();
    qRegisterMetaType<DatabaseError>();
    qRegisterMetaType<StatementResult>();
    qRegisterMetaType<DatabaseResult>();
}
}
// 构造
DatabaseExecutor::DatabaseExecutor(const DatabaseConfig& config, QObject* parent)
    : QObject(parent)
    , config_ { config }
    , worker_ { new DatabaseWorker() }
    , drainTimer_ { new QTimer(this) }
{
    registerDatabaseMetaTypes(); // 注册元类型

    drainTimer_->setSingleShot(true); // 单次触发

    // 连接信号,槽函数
    connect(drainTimer_, &QTimer::timeout,
        this, &DatabaseExecutor::onDrainTimeout); // 连接超时信号和超时槽函数
    // 转移到工作线程
    worker_->moveToThread(&workerThread_); // 将Worker转移到工作线程
    connect(&workerThread_, &QThread::finished,
        this, &DatabaseExecutor::onWorkerThreadFinished,
        Qt::QueuedConnection); // 工作线程任务结束,异步调用槽函数
    connect(worker_, &DatabaseWorker::initialized,
        this, &DatabaseExecutor::onWorkerInitialized);
    connect(worker_, &DatabaseWorker::initializationFailed,
        this, &DatabaseExecutor::onWorkerInitializationFailed);
    connect(worker_, &DatabaseWorker::taskCompleted,
        this, &DatabaseExecutor::onTaskCompleted);
    connect(worker_, &DatabaseWorker::shutdownCompleted,
        this, &DatabaseExecutor::onWorkerShutdownCompleted);
    connect(worker_, &DatabaseWorker::shutdownCompleted,
        worker_, &QObject::deleteLater, Qt::DirectConnection); // 关闭完成直接删除
    connect(worker_, &QObject::destroyed,
        &workerThread_, &QThread::quit, Qt::DirectConnection); // 删除后立即停止工作线程

    workerThread_.setObjectName(QStringLiteral("WmsDatabaseWorkerThread"));
    workerThread_.start();
    // 启动工作线程函数
    const DatabaseConfig workerConfig = config_;
    // 异步初始化工作线程
    QMetaObject::invokeMethod(
        worker_, [worker = worker_, workerConfig] {
            worker->initialize(workerConfig);
        },
        Qt::QueuedConnection);
}
// 析构
DatabaseExecutor::~DatabaseExecutor()
{
    DatabaseWorker* worker = worker_.data();
    workerShutdownRequested_ = true;
    // 请求工作线程关闭
    if (worker != nullptr && workerThread_.isRunning()) {
        QMetaObject::invokeMethod(
            worker, [worker] {
                worker->shutdown();
            },
            Qt::BlockingQueuedConnection); // 阻塞当前的线程等待shutdown函数完成
    }
    // 如果工作线程未关闭,则等待线程结束
    if (workerThread_.isRunning()) {
        workerThread_.quit();
        workerThread_.wait();
    }
}
// 提交任务到工作函数
QUuid DatabaseExecutor::submitTask(DatabaseTask task)
{
    if (task.requestId.isNull()) {
        task.requestId = QUuid::createUuid();
    }
    const QUuid requestId = task.requestId;
    // 如果当前提交任务的线程是执行器的线程,则直接提交任务,避免事件队列开销
    if (QThread::currentThread() == thread()) {
        submitTaskInOwnerThread(task);
    } else { // 否则异步提交任务
        QMetaObject::invokeMethod(
            this, [this, task = std::move(task)] {
                submitTaskInOwnerThread(task);
            },
            Qt::QueuedConnection);
    }
    return requestId;
}
// 获取数据库执行器状态
DatabaseExecutorState DatabaseExecutor::state() const noexcept
{
    return state_;
}
// 关闭工作线程
void DatabaseExecutor::shutdown()
{
    // 异步关闭
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, &DatabaseExecutor::shutdown, Qt::QueuedConnection);
        return;
    }
    // 如果已经关闭
    if (state_ == DatabaseExecutorState::Stopped || workerShutdownRequested_) {
        return;
    }
    // 如果执行失败,取消任务后关闭
    if (state_ == DatabaseExecutorState::Failed) {
        cancelPending(DatabaseErrorCode::Cancelled,
            QStringLiteral("数据库执行器启动失败，任务已取消"));
        requestWorkerShutdown();
        return;
    }
    // 设置关闭中
    setState(DatabaseExecutorState::ShuttingDown);
    if (config_.shutdownDrainTimeoutMs == 0) {
        onDrainTimeout();
        return;
    }
    drainTimer_->start(config_.shutdownDrainTimeoutMs);
    // 如果没有任务执行且任务队列空,关闭
    if (!taskInFlight_ && pendingTasks_.isEmpty()) {
        requestWorkerShutdown();
    } else if (!taskInFlight_) { // 还有任务未执行,派发任务
        dispatchNext();
    }
}
// 数据库工作线程初始化完成
void DatabaseExecutor::onWorkerInitialized()
{
    if (state_ == DatabaseExecutorState::Starting) {
        setState(DatabaseExecutorState::Ready);
        emit ready();
    }
    if (state_ == DatabaseExecutorState::Ready
        || state_ == DatabaseExecutorState::ShuttingDown) {
        dispatchNext();
    }
}
// 数据库工作线程初始化失败
void DatabaseExecutor::onWorkerInitializationFailed(const DatabaseError& error)
{
    qCWarning(databaseExecutorLog) << "Worker 初始化失败:" << error.message;
    setState(DatabaseExecutorState::Failed);
    emit startupFailed(error);
    cancelPending(DatabaseErrorCode::ConnectionFailed,
        QStringLiteral("数据库工作线程初始化失败"));
    requestWorkerShutdown();
}
// 数据库工作线程任务完成
void DatabaseExecutor::onTaskCompleted(const DatabaseResult& result)
{
    taskInFlight_ = false;
    emit taskFinished(result);
    // 如果执行器状态正常,继续分发
    if (state_ == DatabaseExecutorState::Ready) {
        dispatchNext();
        return;
    }
    // 如果正在关闭
    if (state_ == DatabaseExecutorState::ShuttingDown) {
        if (pendingTasks_.isEmpty()) { // 空了就关闭
            requestWorkerShutdown();
        } else {
            dispatchNext(); // 没空就下一个任务
        }
    }
}
// 排空超时
void DatabaseExecutor::onDrainTimeout()
{
    if (state_ != DatabaseExecutorState::ShuttingDown) {
        return;
    }
    // 取消派发剩下的任务
    qCWarning(databaseExecutorLog) << "关闭排空超时，取消"
                                   << pendingTasks_.size() << "个未派发任务";
    cancelPending(DatabaseErrorCode::Cancelled,
        QStringLiteral("数据库执行器关闭排空超时，任务未派发"));
    if (!taskInFlight_) { // 如果没有执行任务了.关闭
        requestWorkerShutdown();
    }
}
// 完成Shutdown,停止排空计时器
void DatabaseExecutor::onWorkerShutdownCompleted()
{
    drainTimer_->stop();
}
// 数据库工作线程完成
void DatabaseExecutor::onWorkerThreadFinished()
{
    if (state_ != DatabaseExecutorState::Failed) {
        setState(DatabaseExecutorState::Stopped);
    }
    if (!shutdownFinishedEmitted_) {
        shutdownFinishedEmitted_ = true;
        emit shutdownFinished();
    }
}
// 提交任务到执行器线程进入任务队列
void DatabaseExecutor::submitTaskInOwnerThread(const DatabaseTask& task)
{
    if (!task.isValid()) {
        emit taskFinished(RejectedResult(task.requestId, DatabaseErrorCode::InvalidTask,
            QStringLiteral("数据库任务无效")));
        return;
    }
    if (state_ == DatabaseExecutorState::ShuttingDown
        || state_ == DatabaseExecutorState::Stopped
        || state_ == DatabaseExecutorState::Failed) {
        emit taskFinished(RejectedResult(task.requestId, DatabaseErrorCode::ShuttingDown,
            QStringLiteral("数据库执行器不再接收新任务")));
        return;
    }
    if (pendingTasks_.size() >= config_.queueCapacity) {
        emit taskFinished(RejectedResult(task.requestId, DatabaseErrorCode::QueueFull,
            QStringLiteral("数据库任务队列已满")));
        return;
    }

    pendingTasks_.enqueue(task);
    if (state_ == DatabaseExecutorState::Ready && !taskInFlight_) {
        dispatchNext();
    }
}
// 分发下一个任务异步执行
void DatabaseExecutor::dispatchNext()
{
    if (taskInFlight_ || pendingTasks_.isEmpty() || workerShutdownRequested_
        || worker_ == nullptr
        || (state_ != DatabaseExecutorState::Ready
            && state_ != DatabaseExecutorState::ShuttingDown)) {
        return;
    }
    // 分发下一个任务异步执行
    const DatabaseTask task = pendingTasks_.dequeue();
    taskInFlight_ = true;
    const bool invoked = QMetaObject::invokeMethod(
        worker_, [worker = worker_, task] {
            worker->executeTask(task);
        },
        Qt::QueuedConnection);
    // 如果执行成功直接返回
    if (invoked) {
        return;
    }
    // 任务失败
    taskInFlight_ = false;
    emit taskFinished(RejectedResult(task.requestId, DatabaseErrorCode::ConnectionFailed,
        QStringLiteral("无法向数据库工作线程派发任务")));
    setState(DatabaseExecutorState::Failed);
    cancelPending(DatabaseErrorCode::Cancelled,
        QStringLiteral("数据库工作线程不可用，任务已取消"));
    requestWorkerShutdown();
}
// 请求数据库工作线程关闭
void DatabaseExecutor::requestWorkerShutdown()
{
    if (workerShutdownRequested_) {
        return;
    }
    workerShutdownRequested_ = true;
    drainTimer_->stop();

    if (worker_ == nullptr || !workerThread_.isRunning()) {
        onWorkerThreadFinished();
        return;
    }
    QMetaObject::invokeMethod(
        worker_, [worker = worker_] {
            worker->shutdown();
        },
        Qt::QueuedConnection);
}
// 取消所有未派发任务
void DatabaseExecutor::cancelPending(DatabaseErrorCode code, const QString& message)
{
    while (!pendingTasks_.isEmpty()) {
        const DatabaseTask task = pendingTasks_.dequeue();
        emit taskFinished(RejectedResult(task.requestId, code, message));
    }
}
// 设置执行器状态
void DatabaseExecutor::setState(DatabaseExecutorState state)
{
    if (state_ == state) {
        return;
    }
    state_ = state;
    emit stateChanged(state_);
}
// 创建拒绝结果
DatabaseResult DatabaseExecutor::RejectedResult(const QUuid& requestId,
    DatabaseErrorCode code,
    const QString& message) const
{
    DatabaseResult result;
    result.requestId = requestId;
    result.error.code = code;
    result.error.message = message;
    return result;
}
