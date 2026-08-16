// 物资仓库接口
#pragma once
#include "AppError.h"
#include "AuditContext.h"
#include "ProductDto.h"
#include "product.h"

#include <QObject>
#include <QString>
#include <functional>
#include <optional>
// 操作结果
struct ProductOperationResult {
    bool success { false };
    std::optional<Product> product;
    std::optional<AppError> error;
};
// 分页结果
struct ProductPageResult {
    bool success { false };
    PageResult<ProductListItemDto> page;
    std::optional<AppError> error;
};
// 选择结果
struct ProductOptionsResult {
    bool success { false };
    QVector<ProductOptionDto> productOptions; // 产品选项
    std::optional<AppError> error;
};
class IProductRepository {
public:
    using PageCallback = std::function<void(const ProductPageResult&)>; // 分页回调
    using OperateCallback = std::function<void(const ProductOperationResult&)>; // 操作回调
    using ActiveCallback = std::function<void(std::optional<AppError> error)>; // 状态设置回调
    using OptionsCallback = std::function<void(const ProductOptionsResult&)>; // 选项回调
    virtual ~IProductRepository() = default;
    // 根据条件分页列出产品
    virtual void listProducts(
        const ProductFilter& filter,
        const PageRequest& pageRequest,
        QObject* owner,
        PageCallback callback_)
        = 0;
    // 列出所有产品选项(默认仅激活)
    virtual void listProductOptions(QObject* owner, OptionsCallback callback, bool activeOnly = true) = 0;
    // 通过编码找物品,让ProductService检查编码唯一性
    virtual void findByCode(
        const QString& code,
        QObject* owner,
        OperateCallback callback)
        = 0;
    // 创建物品,由ProductService校验是否合法
    virtual void createProduct(
        const Product& product,
        const AuditContext& auditContext,
        QObject* owner,
        OperateCallback callback)
        = 0;
    // 更新物品,由ProductService校验是否合法与编码唯一性
    virtual void updateProduct(
        const Product& product,
        const AuditContext& auditContext,
        QObject* owner,
        OperateCallback callback)
        = 0;
    // 设置物品状态,软删除
    virtual void setProductActive(
        quint32 id,
        bool active,
        const AuditContext& auditContext,
        QObject* owner,
        ActiveCallback callback)
        = 0;
};