#include "ProductService.h"
#include "IProductRepository.h"
#include <utility>

ProductService::ProductService(IProductRepository& repository, SessionManager& sessionManager, QObject* parent)
    : repository_(repository)
    , sessionManager_(sessionManager)
    , QObject(parent)
{
}
// 校验权限
bool ProductService::hasPermission(Permission permission) const noexcept
{
    return sessionManager_.hasPermission(permission);
}
std::optional<AppError> ProductService::authorize(Permission permission) const
{
    if (!sessionManager_.isAuthenticated()) {
        return AppError {
            AppErrorCategory::Auth,
            AppErrorCode::NotAuthenticated,
            QStringLiteral("用户未登录")
        };
    }
    if (!hasPermission(permission)) {
        return AppError::permissionDenied();
    }
    return std::nullopt;
}

// 校验产品是否合法
// 更新的产品是否合法
std::optional<AppError> ProductService::validateUpdateProduct(const Product& product) const noexcept
{
    if (product.id <= 0)
        return AppError {
            AppErrorCategory::Validation,
            AppErrorCode::InvalidProduct,
            QStringLiteral("产品ID不合法")
        };
    if (product.code.trimmed().isEmpty())
        return AppError {
            AppErrorCategory::Validation,
            AppErrorCode::InvalidProduct,
            QStringLiteral("产品编码不能为空")
        };
    if (product.name.trimmed().isEmpty())
        return AppError {
            AppErrorCategory::Validation,
            AppErrorCode::InvalidProduct,
            QStringLiteral("产品名称不能为空")
        };
    if (product.unitId <= 0) {
        return AppError {
            AppErrorCategory::Validation,
            AppErrorCode::InvalidProduct,
            QStringLiteral("产品单位ID不合法")
        };
    }
    if (product.safetyStock < 0)
        return AppError {
            AppErrorCategory::Validation,
            AppErrorCode::InvalidProduct,
            QStringLiteral("产品安全库存数量不合法")
        };
    if (product.categoryId <= 0)
        return AppError {
            AppErrorCategory::Validation,
            AppErrorCode::InvalidProduct,
            QStringLiteral("产品分类ID不合法")
        };
    return std::nullopt;
}
// 创建的产品是否合法
std::optional<AppError> ProductService::validateCreateProduct(const Product& product) const noexcept
{
    if (product.name.trimmed().isEmpty())
        return AppError {
            AppErrorCategory::Validation,
            AppErrorCode::InvalidProduct,
            QStringLiteral("产品名称不能为空")
        };
    if (product.unitId <= 0) {
        return AppError {
            AppErrorCategory::Validation,
            AppErrorCode::InvalidProduct,
            QStringLiteral("产品单位ID不合法")
        };
    }
    if (product.safetyStock < 0)
        return AppError {
            AppErrorCategory::Validation,
            AppErrorCode::InvalidProduct,
            QStringLiteral("产品安全库存数量不合法")
        };
    if (product.categoryId <= 0)
        return AppError {
            AppErrorCategory::Validation,
            AppErrorCode::InvalidProduct,
            QStringLiteral("产品分类ID不合法")
        };
    return std::nullopt;
}

// 校验产品请求
std::optional<AppError> ProductService::validateRequest(const PageRequest& request) const noexcept
{
    if (request.page <= 0 || request.pageSize <= 0) {
        return AppError {
            AppErrorCategory::Validation,
            AppErrorCode::InvalidInput,
            QStringLiteral("页数/每页大小不合法")
        };
    }
    return std::nullopt;
}
// 校验产品的过滤器
std::optional<AppError> ProductService::validateFilter(const ProductFilter& filter) const noexcept
{
    // 校验分类ID是否合法
    if ((filter.categoryId.has_value() && filter.categoryId.value() <= 0))
        return AppError {
            AppErrorCategory::Validation,
            AppErrorCode::InvalidInput,
            QStringLiteral("分类ID不合法")
        };
    return std::nullopt;
}
// 列出产品
void ProductService::listProducts(
    const ProductFilter& filter,
    const PageRequest& pageRequest,
    QObject* owner,
    PageCallback callback)
{
    // 校验所属对象
    QPointer<QObject> ownerPtr(owner);
    if (ownerPtr.isNull())
        return;
    // 校验是否有ViewProducts权限
    if (auto error = authorize(Permission::ViewProducts); error.has_value()) {
        if (callback != nullptr) {
            callback(ProductPageResult { false, PageResult<ProductListItemDto> {}, error });
        }
        return;
    }
    // 校验请求和过滤器
    if (auto error1 = validateRequest(pageRequest); error1.has_value()) {
        if (callback != nullptr) {
            callback(ProductPageResult { false, PageResult<ProductListItemDto> {}, error1 });
        }
        return;
    }
    if (auto error2 = validateFilter(filter); error2.has_value()) {
        if (callback != nullptr) {
            callback(ProductPageResult { false, PageResult<ProductListItemDto> {}, error2 });
        }
        return;
    }
    // 通过IProductRepository接口列出产品并回调
    repository_.listProducts(filter, pageRequest, ownerPtr.data(), [ownerPtr, callback = std::move(callback)](const ProductPageResult& repoResult) mutable {
        if (ownerPtr.isNull() || !callback)
            return;
        // 如果查询失败
        if (repoResult.error.has_value()) {
            callback(ProductPageResult { false, PageResult<ProductListItemDto> {}, repoResult.error.value() });
            return;
        }

        // 找到产品
        callback(repoResult);
    });
}
// 获取产品选项
void ProductService::listProductOptions(
    QObject* owner,
    OptionsCallback callback,
    bool activeOnly)
{
    // 校验参数
    QPointer<QObject> ownerPtr(owner);
    if (ownerPtr.isNull() || !callback)
        return;
    // 校验是否有权限
    if (auto error = authorize(Permission::ViewProducts); error.has_value()) {
        callback(ProductOptionsResult { false,{}, error });
        return;
    }
    repository_.listProductOptions(ownerPtr.data(), [ownerPtr, this, callback = std::move(callback)](const ProductOptionsResult& repoResult) {
        if (ownerPtr.isNull() || !callback)
            return;
        if (!repoResult.success) {
            callback(ProductOptionsResult { false, {},repoResult.error.has_value()?repoResult.error.value(): AppError::repositoryFailure(QStringLiteral("查询产品选项失败")) });
            return;
        }
        if (repoResult.error.has_value()) {
            callback(ProductOptionsResult { false, {},repoResult.error.value() });
            return;
        }
        callback(ProductOptionsResult {
            true,
            repoResult.productOptions,
            std::nullopt
        });
    }, activeOnly);
}
// 创建产品
void ProductService::createProduct(
    const Product& product,
    QObject* owner,
    OperateCallback callback)
{
    QPointer<QObject> ownerPtr(owner);
    if (ownerPtr.isNull() || !callback)
        return;
    // 校验产品
    if (auto error = authorize(Permission::CreateProducts); error.has_value()) {
        callback(ProductOperationResult { false, std::nullopt, error });
        return;
    }
    if (auto error = validateCreateProduct(product); error.has_value()) {
        callback(ProductOperationResult { false, std::nullopt, error });
        return;
    }
    if (product.code.trimmed().isEmpty()) {
        const auto auditCtx = buildAuditContext();
        repository_.createProduct(product, auditCtx, ownerPtr.data(), [ownerPtr, callback = std::move(callback)](const ProductOperationResult& repoResult) {
            if (ownerPtr.isNull() || !callback)
                return;
            if (repoResult.error.has_value()) {
                callback(ProductOperationResult { false, std::nullopt, repoResult.error.value() });
                return;
            }
            if (!repoResult.product.has_value()) {
                callback(ProductOperationResult { false, std::nullopt, AppError { AppErrorCategory::Validation, AppErrorCode::InvalidProduct, QStringLiteral("产品创建后未返回有效数据") } });
                return;
            }
            callback(ProductOperationResult { true, repoResult.product.value(), std::nullopt });
        });
        return;
    }
    // 寻找产品编码是否已存在
    repository_.findByCode(product.code, ownerPtr.data(), [ownerPtr, product, callback, this](const ProductOperationResult& repoResult) mutable {
        if (ownerPtr.isNull() || !callback) {
            return;
        }
        // 如果查询失败
        if (repoResult.error.has_value()) {
            callback(ProductOperationResult { false, std::nullopt, repoResult.error.value() });
            return;
        }
        // 如果已存在,创建失败
        if (repoResult.product.has_value()) {
            callback(ProductOperationResult { false, std::nullopt, AppError { AppErrorCategory::Validation, AppErrorCode::InvalidProduct, QStringLiteral("产品已存在") } });
            return;
        }
        // 不存在,就创建新产品并回调结果
        const auto auditCtx = buildAuditContext();
        repository_.createProduct(product, auditCtx, ownerPtr.data(), [ownerPtr, callback = std::move(callback)](const ProductOperationResult& repoResult) {
            if (ownerPtr.isNull() || !callback)
                return;
            // 如果创建失败
            if (repoResult.error.has_value()) {
                callback(ProductOperationResult { false, std::nullopt, repoResult.error.value() });
                return;
            }
            // 如果创建成功,但未返回有效数据
            if (!repoResult.product.has_value()) {
                callback(
                    ProductOperationResult { false, std::nullopt, AppError { AppErrorCategory::Validation, AppErrorCode::InvalidProduct, QStringLiteral("产品创建后未返回有效数据") } });
                return;
            }
            // 回调创建成功
            callback(ProductOperationResult { true, repoResult.product.value(), std::nullopt });
        });
    });
}
// 更新产品
void ProductService::updateProduct(
    const Product& product,
    QObject* owner,
    OperateCallback callback)
{
    QPointer<QObject> ownerPtr(owner);
    if (ownerPtr.isNull())
        return;
    // 校验产品
    if (auto error = authorize(Permission::EditProducts); error.has_value()) {
        if (callback != nullptr)
            callback(ProductOperationResult { false, std::nullopt, error });
        return;
    }
    if (auto error = validateUpdateProduct(product); error.has_value()) {
        if (callback != nullptr)
            callback(ProductOperationResult { false, std::nullopt, error });
        return;
    }
    // 查找编号是否存在对应产品
    repository_.findByCode(product.code, ownerPtr.data(), [ownerPtr, product, callback, this](const ProductOperationResult& repoResult) mutable {
        if (ownerPtr.isNull() || !callback) {
            return;
        }
        // 如果查询失败
        if (repoResult.error.has_value()) {
            callback(ProductOperationResult { false, std::nullopt, repoResult.error.value() });
            return;
        }
        // 如果已存在且编码不同(不是自己)
        if (repoResult.product.has_value() && repoResult.product.value().id != product.id) {
            callback(ProductOperationResult { false, std::nullopt, AppError { AppErrorCategory::Validation, AppErrorCode::InvalidProduct, QStringLiteral("产品编码已存在") } });
            return;
        }
        // 对应编码的产品不存在,就更新产品
        const auto auditCtx = buildAuditContext();
        repository_.updateProduct(product, auditCtx, ownerPtr.data(), [ownerPtr, callback = std::move(callback)](const ProductOperationResult& repoResult) {
            if (ownerPtr.isNull() || !callback)
                return;
            // 如果更新失败
            if (repoResult.error.has_value()) {
                callback(ProductOperationResult { false, std::nullopt, repoResult.error.value() });
                return;
            }
            // 如果更新成功,但未返回有效数据
            if (!repoResult.product.has_value()) {
                callback(ProductOperationResult { false, std::nullopt, AppError { AppErrorCategory::Validation, AppErrorCode::InvalidProduct, QStringLiteral("产品更新后未返回有效数据") } });
                return;
            }
            // 回调更新成功
            callback(ProductOperationResult { true, repoResult.product.value(), std::nullopt });
        });
    });
}

// 设置产品状态
void ProductService::setProductActive(
    quint32 productId,
    bool active,
    QObject* owner,
    ActiveCallback callback)
{
    QPointer<QObject> ownerPtr(owner);
    if (ownerPtr.isNull())
        return;
    // 校验产品权限
    if (auto error = authorize(Permission::DisableProducts); error.has_value()) {
        if (callback != nullptr)
            callback(error);
        return;
    }
    if (productId == 0) {
        if (callback != nullptr)
            callback(AppError { AppErrorCategory::Validation, AppErrorCode::InvalidProduct, QStringLiteral("产品ID无效") });
        return;
    }
    // 设置产品状态并回调结果
    const auto auditCtx = buildAuditContext();
    repository_.setProductActive(productId, active, auditCtx, ownerPtr.data(), [ownerPtr, callback = std::move(callback)](const std::optional<AppError>& error) {
        if (ownerPtr.isNull() || !callback)
            return;
        callback(error);
    });
}
AuditContext ProductService::buildAuditContext() const noexcept
{
    const auto user = sessionManager_.currentUser();
    if (!user.has_value()) {
        return {};
    }
    return { user->userName, user->id };
}

// 获取当前用户
std::optional<AuthenticatedUser> ProductService::currentUser() const noexcept
{
    return sessionManager_.currentUser();
}