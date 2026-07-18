#include "ConnectionPool.h"
#include <QSqlError>
#include <QSqlQuery>
#include <stdexcept>

// 静态成员定义
std::optional<ConnectionPoolConfig> ConnectionPool::Configs;
bool ConnectionPool::isInit = false;

//  Init：仅保存配置
void ConnectionPool::Init(const ConnectionPoolConfig& Configs_)
{
    // 已经初始化,警告并返回(仅初始化一次)
    if (isInit) {
        qWarning() << "ConnectionPool::Init: 连接池已初始化!";
        return;
    }
    Configs = Configs_;
    isInit = true;
}

//  Instance: 获取单例实例
ConnectionPool& ConnectionPool::Instance()
{
    // 未初始化,抛出异常
    if (!isInit) {
        throw std::runtime_error("ConnectionPool::Instance: 连接池未初始化!\n");
    }
    static ConnectionPool instance;
    return instance;
}

//  构造函数：创建连接
ConnectionPool::ConnectionPool(QObject* parent)
    : QObject(parent)
{
    // 设置数据库设置
    const DbConfig& Db = ConnectionPool::Configs->DbConfig;
    PoolConnectionNamePrefix = Db.database + "_"; // 连接名前缀

    // 创建初始连接
    for (int i = 0; i < ConnectionPool::Configs->InitConnectionsNumber; ++i) {
        auto meta = CreateNewConnection();
        if (meta.has_value()) {
            IdleConnections.enqueue(*meta);
            ++TotalCount;
        }
    }
    qDebug() << "ConnectionPool initialized: "
             << ConnectionPool::Configs->InitConnectionsNumber << "connections, "
             << "max connections: " << ConnectionPool::Configs->MaxConnectionsNumber;
}

//  析构函数: 关闭所有连接
ConnectionPool::~ConnectionPool()
{
    ShutdownAll();
}

//  GetConnection：返回ConnectionMeta
//  获取路径:
//  1.池空闲连接不为空直接获取
//  2.空闲连接空但池未满,新建连接
//  3.池已满,超时等待并返回
std::optional<ConnectionMeta> ConnectionPool::GetConnection()
{
    QMutexLocker locker(&Mutex);

    // 1.池有空闲连接
    if (!IdleConnections.isEmpty()) {
        ConnectionMeta meta = IdleConnections.dequeue();
        meta.LastUsed = QDateTime::currentDateTime();
        ++ActiveCount;
        return meta;
    }

    // 2.无空闲连接但池未满,新建连接
    if (TotalCount < ConnectionPool::Configs->MaxConnectionsNumber) {
        locker.unlock();
        auto meta = CreateNewConnection();
        locker.relock();
        if (meta.has_value()) {
            meta->LastUsed = QDateTime::currentDateTime();
            ++TotalCount;
            ++ActiveCount;
            return meta;
        }
        return std::nullopt;
    }

    // 3.池已满, 简单返回空（桌面应用低并发，不阻塞等待）
    emit PoolExhausted();
    return std::nullopt;
}

//  ReleaseConnection：接收ConnectionMeta入队
void ConnectionPool::ReleaseConnection(const ConnectionMeta& meta)
{
    QMutexLocker locker(&Mutex);

    IdleConnections.enqueue(meta);
    --ActiveCount;
}

//  ShutdownAll: 关闭所有连接
void ConnectionPool::ShutdownAll()
{
    if (ActiveCount > 0) {
        qWarning() << "ConnectionPool::ShutdownAll: 连接池中还有" << ActiveCount << "个活跃连接未归还!";
    }

    emit AbouttoShutdownAll();

    QMutexLocker locker(&Mutex);

    // 关闭所有空闲连接
    while (!IdleConnections.isEmpty()) {
        ConnectionMeta meta = IdleConnections.dequeue();
        auto db = QSqlDatabase::database(meta.Name);
        if (db.isValid()) {
            db.close();
        }
        QSqlDatabase::removeDatabase(meta.Name);
    }
}

//  CreateNewConnection：创建连接，返回完整 meta(不入空闲队列,仅创建)
std::optional<ConnectionMeta> ConnectionPool::CreateNewConnection()
{
    int id = nextConnectionId++; // 生成新的连接ID
    QString name = PoolConnectionNamePrefix + QString::number(id);
    const DbConfig& Db = ConnectionPool::Configs->DbConfig;

    // 创建数据库连接
    QSqlDatabase db_connection = QSqlDatabase::addDatabase("QODBC", name);
    db_connection.setDatabaseName(QString(
        "DRIVER={%1};"
        "SERVER=%2;PORT=%3;DATABASE=%4;UID=%5;PWD=%6;")
                                      .arg(Db.Driver)
                                      .arg(Db.HostName)
                                      .arg(Db.port)
                                      .arg(Db.database)
                                      .arg(Db.User)
                                      .arg(Db.Password));

    if (!db_connection.open()) {
        qWarning() << "ConnectionPool::CreateNewConnection: 连接数据库失败!"
                   << db_connection.lastError().text();
        db_connection = QSqlDatabase();
        QSqlDatabase::removeDatabase(name);
        return std::nullopt;
    }

    ConnectionMeta meta;
    meta.Name = name;
    meta.CreateTime = QDateTime::currentDateTime();
    meta.LastUsed = meta.CreateTime;
    return meta;
}
