#pragma once
// Qt API:
#include <QAtomicInt>
#include <QDateTime>
#include <QDebug>
#include <QMutex>
#include <QObject>
#include <QQueue>
#include <QSqlDatabase>
#include <QString>
#include <QTimer>
#include <QWaitCondition>
// std API:
#include <chrono>
#include <optional>
// 数据库配置
struct DbConfig {
    QString HostName;
    int port;
    QString User;
    QString Password;
    QString database;
    DbConfig(const QString& HostName_ = "", int port_ = 3306, const QString& User_ = "", const QString& Password_ = "", const QString& database_ = "")
        : HostName(HostName_)
        , port(port_)
        , User(User_)
        , Password(Password_)
        , database(database_)
    {
    }
};
// 连接池配置
struct ConnectionPoolConfig {
    DbConfig DbConfig;
    int MaxConnectionsNumber; // 最大连接数
    int InitConnectionsNumber; // 初始连接数
    int PingIntervalMs; // 健康检查间隔, 单位毫秒
    int ShrinkIntervalMs; // 空闲连接检查间隔, 单位毫秒
    int AcquireTimeoutMs; // 获取连接超时时间, 单位毫秒
    int ShrinkIntervalSec; // 空闲连接超时时间, 单位秒
    int MaxLifetimeSec; // 最大连接生命周期, 单位秒
    ConnectionPoolConfig(const struct DbConfig& DbConfig_, int MaxConnectionsNumber_, int InitConnectionsNumber_, int PingIntervalMs_, int ShrinkIntervalMs_, int AcquireTimeoutMs_, int ShrinkIntervalSec_, int MaxLifetimeSec_)
        : DbConfig(DbConfig_)
        , MaxConnectionsNumber(MaxConnectionsNumber_)
        , InitConnectionsNumber(InitConnectionsNumber_)
        , PingIntervalMs(PingIntervalMs_)
        , ShrinkIntervalMs(ShrinkIntervalMs_)
        , AcquireTimeoutMs(AcquireTimeoutMs_)
        , ShrinkIntervalSec(ShrinkIntervalSec_)
        , MaxLifetimeSec(MaxLifetimeSec_)
    {
    }
};
// 连接元数据
struct ConnectionMeta {
    QString Name;
    QDateTime CreateTime;
    QDateTime LastUsed;
    ConnectionMeta() = default;
    ConnectionMeta(const QDateTime& LastUsed_, const QString& Name_ = "", const QDateTime& CreateTime_ = QDateTime::currentDateTime())
        : Name(Name_)
        , CreateTime(CreateTime_)
        , LastUsed(LastUsed_)
    {
    }
};
class ConnectionPool : public QObject {
    Q_OBJECT
public:
    // 单例模式
    ConnectionPool(const ConnectionPool& other, QObject* parent = nullptr) = delete;
    ConnectionPool(ConnectionPool&& other) = delete;
    ConnectionPool& operator=(const ConnectionPool& other) = delete;
    static ConnectionPool& Instance(); // 获取单例实例
    static void Init(const ConnectionPoolConfig& Configs_); // 初始化连接池(仅初始化一次!)

    // 连接池功能
    std::optional<ConnectionMeta> GetConnection();               // 获取一个空闲连接（返回完整元数据）
    void ReleaseConnection(const ConnectionMeta& meta);          // 归还连接（带回完整元数据）
    void ShutdownAll();                                          // 关闭所有连接

private:
    std::optional<ConnectionMeta> CreateNewConnection(); // 创建新连接，返回完整元数据（不入队，由调用方决定）
    explicit ConnectionPool(QObject* parent = nullptr);
    ~ConnectionPool() override;
    QMutex Mutex; // 互斥锁
    QWaitCondition WaitCV; // 用于等待连接
    QAtomicInt ActiveCount; // 活跃连接数
    QQueue<ConnectionMeta> IdleConnections; // 空闲连接队列（带完整元数据）
    QTimer* pingTimer { nullptr }; // 健康检查定时器
    QTimer* shrinkTimer { nullptr }; // 空闲回收定时器

    // 上下文
    QAtomicInt nextConnectionId; // 下一个连接Id
    QString PoolConnectionNamePrefix; // 连接名前缀

    static std::optional<ConnectionPoolConfig> Configs; // 单例实例的配置
    static bool isInit; // 是否初始化
    // 信号/槽
public:
signals:
    void connectionBroken(const QString& ConnectionName); // 连接崩溃信号
    void PoolExhausted(); // 连接满信号
private slots:
    void onPingTimer(); // 健康检查
    void onShrinkTimer(); // 空闲回收
};
