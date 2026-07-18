#include "ScopedConnection.h"

// ── 构造：获取完整 ConnectionMeta ──
ScopedConnection::ScopedConnection(ConnectionPool& pool)
    : pool_(&pool)
{
    meta_ = pool_->GetConnection();   // 返回连接元数据
}

// ── 移动构造：转移所有权 ──
ScopedConnection::ScopedConnection(ScopedConnection&& other) noexcept
    : pool_(other.pool_)
    , meta_(std::move(other.meta_))
{
    other.pool_ = nullptr;
}

// ── 移动赋值：先归还当前，再接管新的 ──
ScopedConnection& ScopedConnection::operator=(ScopedConnection&& other) noexcept
{
    if (this != &other) {
        release();
        pool_ = other.pool_;
        meta_ = std::move(other.meta_);
        other.pool_ = nullptr;
    }
    return *this;
}

// ── 析构：自动归还 ──
ScopedConnection::~ScopedConnection()
{
    release();
}

// ── db()：从 meta 中取 Name，返回 QSqlDatabase ──
QSqlDatabase ScopedConnection::db() const
{
    if (!meta_.has_value()) return {};
    return QSqlDatabase::database(meta_->Name);
}

// ── release()：归还完整 meta，CreateTime 不丢失 ──
void ScopedConnection::release()
{
    if (meta_.has_value() && pool_ != nullptr) {
        pool_->ReleaseConnection(*meta_);   // 归还完整 ConnectionMeta
        meta_.reset();
    }
}

// ── operator bool ──
ScopedConnection::operator bool() const noexcept
{
    return meta_.has_value();
}

// ── connectionName()：从 meta 中提取连接名 ──
std::optional<QString> ScopedConnection::connectionName() const noexcept
{
    if (!meta_.has_value()) return std::nullopt;
    return meta_->Name;
}
