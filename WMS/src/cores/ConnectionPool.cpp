#include "ConnectionPool.h"
#include <QSqlError>
#include <QSqlQuery>
#include <stdexcept>

// 静态成员定义
std::optional<ConnectionPoolConfig> ConnectionPool::Configs;
bool ConnectionPool::isInit = false;

// ── Init：仅保存配置 ──
void ConnectionPool::Init(const ConnectionPoolConfig& Configs_)
{
    if (isInit) {
        qWarning() << "ConnectionPool::Init: 连接池已初始化!";
        return;
    }
    Configs = Configs_;
    isInit = true;
}

// ── Instance: 获取单例实例 ──
ConnectionPool& ConnectionPool::Instance()
{
    if (!isInit) {
        throw std::runtime_error("ConnectionPool::Instance: 连接池未初始化! 请先调用 ConnectionPool::Init()");
    }
    static ConnectionPool instance;
    return instance;
}

// ── 构造函数：创建连接、启动定时器 ──
ConnectionPool::ConnectionPool(QObject* parent)
    : QObject(parent)
{
    const DbConfig& Db = Configs->DbConfig;
    PoolConnectionNamePrefix = Db.database + "_";

    // 创建初始连接
    for (int i = 0; i < Configs->InitConnectionsNumber; ++i) {
        auto meta = CreateNewConnection();
        if (meta.has_value()) {
            IdleConnections.enqueue(*meta);
        }
    }

    qDebug() << "ConnectionPool initialized:"
             << Configs->InitConnectionsNumber << "connections,"
             << "max:" << Configs->MaxConnectionsNumber;

    // 健康检查定时器
    pingTimer = new QTimer(this);
    connect(pingTimer, &QTimer::timeout, this, &ConnectionPool::onPingTimer);
    pingTimer->start(Configs->PingIntervalMs);

    // 空闲回收定时器
    shrinkTimer = new QTimer(this);
    connect(shrinkTimer, &QTimer::timeout, this, &ConnectionPool::onShrinkTimer);
    shrinkTimer->start(Configs->ShrinkIntervalMs);
}

// ── 析构函数: 关闭所有连接 ──
ConnectionPool::~ConnectionPool()
{
    ShutdownAll();
}

// ── GetConnection：返回ConnectionMeta ──
std::optional<ConnectionMeta> ConnectionPool::GetConnection()
{
    QMutexLocker locker(&Mutex);

    // 池不空
    if (!IdleConnections.isEmpty()) {
        ConnectionMeta meta = IdleConnections.dequeue();
        meta.LastUsed = QDateTime::currentDateTime();
        ActiveCount.fetchAndAddRelaxed(1);
        return meta;
    }

    // 池未满,新建连接，直接返回（不入队列）
    if (ActiveCount.loadRelaxed() < Configs->MaxConnectionsNumber) {
        auto meta = CreateNewConnection();
        if (!meta.has_value()) {
            return std::nullopt;
        }
        ActiveCount.fetchAndAddRelaxed(1);
        return meta;
    }

    // 池已满，等待其他线程归还
    emit PoolExhausted();

    auto deadline = std::chrono::steady_clock::now()
                  + std::chrono::milliseconds(Configs->AcquireTimeoutMs);

    // while 循环处理虚假唤醒：只要队列空且无法新建，就继续等
    while (IdleConnections.isEmpty()
           && ActiveCount.loadRelaxed() >= Configs->MaxConnectionsNumber) {
        auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
            deadline - std::chrono::steady_clock::now());
        if (remaining.count() <= 0
            || !WaitCV.wait(&Mutex, static_cast<unsigned long>(remaining.count()))) {
            return std::nullopt; // 超时
        }
    }

    // 唤醒后队列有可用连接
    if (!IdleConnections.isEmpty()) {
        ConnectionMeta meta = IdleConnections.dequeue();
        meta.LastUsed = QDateTime::currentDateTime();
        ActiveCount.fetchAndAddRelaxed(1);
        return meta;
    }

    // 队列仍空但容量有余（等待期间有线程释放了 ActiveCount），尝试新建
    auto meta = CreateNewConnection();
    if (!meta.has_value()) {
        return std::nullopt;
    }
    ActiveCount.fetchAndAddRelaxed(1);
    return meta;
}

// ── ReleaseConnection：接收ConnectionMeta入队 ──
void ConnectionPool::ReleaseConnection(const ConnectionMeta& meta)
{
    QMutexLocker locker(&Mutex);

    IdleConnections.enqueue(meta);             

    ActiveCount.fetchAndSubRelaxed(1);
    WaitCV.wakeOne();
}

// ── ShutdownAll: 关闭所有连接 ──
void ConnectionPool::ShutdownAll()
{
    if (ActiveCount.loadRelaxed() > 0) {
        qWarning() << "ConnectionPool::ShutdownAll: 连接池中还有" << ActiveCount.loadRelaxed() << "个连接未归还!";
    }
    if (pingTimer!=nullptr)   pingTimer->stop();
    if (shrinkTimer!=nullptr) shrinkTimer->stop();

    QMutexLocker locker(&Mutex);

    while (!IdleConnections.isEmpty()) {
        ConnectionMeta meta = IdleConnections.dequeue();
        QSqlDatabase::database(meta.Name).close();
        QSqlDatabase::removeDatabase(meta.Name);
    }
    WaitCV.wakeAll();
}

// ── CreateNewConnection：创建连接，返回完整 meta，不入队（由调用方决定）──
std::optional<ConnectionMeta> ConnectionPool::CreateNewConnection()
{
    int id = nextConnectionId.fetchAndAddRelaxed(1);
    QString name = PoolConnectionNamePrefix + QString::number(id);
    const DbConfig& Db = Configs->DbConfig;

    // 创建数据库连接
    QSqlDatabase db_connection = QSqlDatabase::addDatabase("QODBC", name);
    db_connection.setDatabaseName(QString(
        "DRIVER={MariaDB Unicode};"
        "SERVER=%1;PORT=%2;DATABASE=%3;UID=%4;PWD=%5;")
        .arg(Db.HostName).arg(Db.port)
        .arg(Db.database).arg(Db.User).arg(Db.Password));

    if (!db_connection.open()) {
        qWarning() << "ConnectionPool::CreateNewConnection: 连接数据库失败!"
                   << db_connection.lastError().text();
        QSqlDatabase::removeDatabase(name);
        return std::nullopt;
    }

    ConnectionMeta meta;
    meta.Name = name;
    meta.CreateTime = QDateTime::currentDateTime();
    meta.LastUsed = meta.CreateTime;
    return meta;
}

// ── onPingTimer：健康检查（骨架，待实现）──
void ConnectionPool::onPingTimer()
{
    QMutexLocker locker(&Mutex);
    // TODO: 遍历 IdleConnections，SELECT 1 检查连接状态
    // 失败的连接关闭并移除 → emit connectionBroken
}

// ── onShrinkTimer：空闲回收（骨架，待实现）──
void ConnectionPool::onShrinkTimer()
{
    QMutexLocker locker(&Mutex);
    // TODO: 遍历 IdleConnections：
    //   - CreateTime 距今 > MaxLifetimeSec → 关闭并移除
    //   - LastUsed 距今 > ShrinkIntervalSec → 关闭并移除
    //   但至少保留 InitConnectionsNumber 条连接
}
