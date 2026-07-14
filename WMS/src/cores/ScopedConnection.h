#pragma once
#include <QSqlDatabase>
#include <QString>
#include <optional>
#include "ConnectionPool.h"

class ScopedConnection {
public:
    // 构造时自动从连接池中获取连接
    explicit ScopedConnection(ConnectionPool& pool);
    // 析构时自动释放归还连接
    ~ScopedConnection();

    // 禁止拷贝
    ScopedConnection(const ScopedConnection&) = delete;
    ScopedConnection& operator=(const ScopedConnection&) = delete;

    // 允许移动
    ScopedConnection(ScopedConnection&& other) noexcept;
    ScopedConnection& operator=(ScopedConnection&& other) noexcept;

    // 获取 QSqlDatabase 对象, 方便直接进行查询
    QSqlDatabase db() const;

    // 手动释放连接, db() 返回无效连接
    void release();

    // 检查连接是否有效
    explicit operator bool() const noexcept;

    // 获取连接名
    std::optional<QString> connectionName() const noexcept;

private:
    ConnectionPool* pool_;                       // 所属连接池
    std::optional<ConnectionMeta> meta_;          // 持有的连接完整元数据
};
