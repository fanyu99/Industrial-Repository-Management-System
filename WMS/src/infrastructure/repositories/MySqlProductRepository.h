// 真实的MySqlProductRepository
#pragma once
#include "AppError.h"
#include "DatabaseExecutor.h"
#include "DatabaseTypes.h"
#include "IProductRepository.h"
#include <QHash>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QUuid>
#include <functional>
#include <optional>
#include <utility>
// 待处理请求上下文
struct PendingRequest {
    QPointer<QObject> owner;
    std::function<void(const DatabaseResult&)> handler;
};

class MySqlProductRepository : public QObject, public IProductRepository {
    Q_OBJECT
public:
    explicit MySqlProductRepository(DatabaseExecutor& executor, QObject* parent = nullptr);
    ~MySqlProductRepository() = default;

    // 列出产品
    void listProducts(
        const ProductFilter& filter,
        const PageRequest& request,
        QObject* owner,
        PageCallback callback_) override;

    // 通过编码查找产品
    void findByCode(
        const QString& code,
        QObject* owner,
        OperateCallback callback) override;
    // 创建产品
    void createProduct(
        const Product& product,
        QObject* owner,
        OperateCallback callback_) override;
    // 更新产品
    void updateProduct(
        const Product& product,
        QObject* owner,
        OperateCallback callback_) override;
    // 设置产品状态
    void setProductActive(
        quint32 id,
        bool active,
        QObject* owner,
        ActiveCallback callback) override;

    // 将数据库错误映射为应用错误
    static AppError mapDatabaseErrorToAppError(const DatabaseError& error);
private slots:
    // 数据库任务完成
    void onTaskFinished(const DatabaseResult& result);

private:
    // 通过ID查找产品
    void findById(
        quint32 id,
        QObject* owner,
        OperateCallback callback);
    // 映射产品行到产品
    static std::optional<Product> mapProductRow(
        const QStringList& columns,
        const QVariantList& row);
    DatabaseExecutor& executor_;
    // 待处理请求,通过id查找对应的任务
    QHash<QUuid, PendingRequest> pending_;
};