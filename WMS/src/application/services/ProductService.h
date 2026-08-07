#pragma once
#include "AuditContext.h"
#include "IProductRepository.h"
#include "Permission.h"
#include "SessionManager.h"
#include <QObject>
#include <QPointer>
#include <optional>

// 产品校验:业务规则(创建,查找等)及权限边界
class ProductService : public QObject {
    Q_OBJECT
public:
    explicit ProductService(
        IProductRepository& repository,
        SessionManager& sessionManager,
        QObject* parent = nullptr);
    ~ProductService() = default;
    using PageCallback = IProductRepository::PageCallback; // 分页回调
    using OperateCallback = IProductRepository::OperateCallback; // 操作回调
    using ActiveCallback = IProductRepository::ActiveCallback; // 状态设置回调
                                                               // 获取当前的用户
    std::optional<AuthenticatedUser> currentUser() const noexcept;
    // 校验用户是否有权限操作产品
    [[nodiscard]] bool hasPermission(Permission permission) const noexcept;
    [[nodiscard]] std::optional<AppError> authorize(Permission permission) const;
    // 校验产品是否合法
    [[nodiscard]] std::optional<AppError> validateUpdateProduct(const Product& product) const noexcept;
    [[nodiscard]] std::optional<AppError> validateCreateProduct(const Product& product) const noexcept;
    // 校验产品请求是否合法
    [[nodiscard]] std::optional<AppError> validateRequest(
        const PageRequest& request) const noexcept;
    // 校验用户的过滤器
    [[nodiscard]] std::optional<AppError> validateFilter(
        const ProductFilter& filter) const noexcept;
    // 构建审计上下文
    [[nodiscard]] AuditContext buildAuditContext() const noexcept;
    // 列出产品
    void listProducts(
        const ProductFilter& filter,
        const PageRequest& pageRequest,
        QObject* owner,
        PageCallback callback);
    // 创建产品
    void createProduct(
        const Product& product,
        QObject* owner,
        OperateCallback callback);
    // 更新产品
    void updateProduct(
        const Product& product,
        QObject* owner,
        OperateCallback callback);
    // 启用/禁用产品
    void setProductActive(quint32 id,
        bool active, QObject* owner,
        ActiveCallback callback);

private:
    IProductRepository& repository_;
    SessionManager& sessionManager_;
};