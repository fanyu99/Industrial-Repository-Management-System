#pragma once

#include "IProductRepository.h"

#include <QPointer>
#include <QVector>
#include <algorithm>
#include <optional>
#include <utility>

// 用于 ProductService / ProductPage 单元测试的内存仓库,不连接真实数据库。
//
// 异步测试能力:
//   - 通过 deferXxx 标志可以把 listProducts/createProduct/updateProduct/setProductActive
//     挂起,稍后用 completePendingXxx 显式完成,模拟真实仓储的异步时序。
//   - 挂起期间会记录 owner 的 QPointer。当 owner 在延迟期间被销毁时,
//     completePendingXxx 不会触发回调(避免悬空回调 / 访问已销毁对象),
//     与真实 MySqlProductRepository 中 PendingRequest.owner 的保护行为保持一致。
//   - listProducts 支持多个并行挂起请求(队列),可按下标完成,用于测试
//     "latest-wins"(旧查询结果不覆盖新查询)。
class FakeProductRepository : public IProductRepository {
public:
    QVector<Product> products;
    std::optional<AppError> nextListError;
    std::optional<AppError> nextFindError;
    std::optional<AppError> nextCreateError;
    std::optional<AppError> nextUpdateError;
    std::optional<AppError> nextActiveError;

    std::optional<ProductFilter> lastFilter;
    std::optional<PageRequest> lastPageRequest;
    std::optional<Product> lastCreatedProduct;
    std::optional<Product> lastUpdatedProduct;
    std::optional<quint32> lastActiveProductId;
    std::optional<bool> lastActiveValue;

    bool deferListProducts { false };
    bool deferCreateProduct { false };
    bool deferUpdateProduct { false };
    bool deferSetProductActive { false };

    // ===== 挂起状态查询 =====

    [[nodiscard]] bool hasPendingListProducts() const noexcept
    {
        return !pendingListOps_.isEmpty();
    }

    // 当前挂起的 listProducts 请求数量(支持多个并行挂起)
    [[nodiscard]] int pendingListCount() const noexcept
    {
        return pendingListOps_.size();
    }

    [[nodiscard]] bool hasPendingCreateProduct() const noexcept
    {
        return static_cast<bool>(pendingCreateCallback_);
    }

    [[nodiscard]] bool hasPendingUpdateProduct() const noexcept
    {
        return static_cast<bool>(pendingUpdateCallback_);
    }

    [[nodiscard]] bool hasPendingSetProductActive() const noexcept
    {
        return static_cast<bool>(pendingActiveCallback_);
    }

    // ===== listProducts 完成 =====
    // index 默认为 0(最早挂起的请求),支持多挂起时按下标完成。

    void completePendingListProductsSuccess(int index = 0)
    {
        if (index < 0 || index >= pendingListOps_.size()) {
            return;
        }
        const auto& op = pendingListOps_.at(index);
        completePendingListProductsAt(index,
            buildListProductsSuccessResult(op.filter, op.pageRequest));
    }

    void completePendingListProducts(const ProductPageResult& result, int index = 0)
    {
        completePendingListProductsAt(index, result);
    }

    void completePendingListProductsError(const AppError& error, int index = 0)
    {
        completePendingListProductsAt(index, ProductPageResult { false, {}, error });
    }

    // ===== createProduct 完成 =====

    void completePendingCreateProduct(const ProductOperationResult& result)
    {
        if (!pendingCreateCallback_) {
            return;
        }
        // owner 已在挂起期间销毁:丢弃回调,避免悬空
        if (pendingCreateOwner_.isNull()) {
            resetPendingCreate();
            return;
        }
        auto callback = std::move(pendingCreateCallback_);
        resetPendingCreate();
        callback(result);
    }

    // 以成功完成:同时把产品写入内存仓库(与立即完成路径一致)
    void completePendingCreateProductSuccess()
    {
        if (!pendingCreateCallback_) {
            return;
        }
        if (pendingCreateOwner_.isNull()) {
            resetPendingCreate();
            return;
        }
        Product created = pendingCreateProduct_.value_or(Product {});
        if (created.id == 0) {
            created.id = nextId_++;
        } else {
            nextId_ = std::max(nextId_, created.id + 1);
        }
        if (created.code.trimmed().isEmpty()) {
            int maxNum = 0;
            for (const auto& p : products) {
                if (p.code.length() > 1 && p.code.startsWith(QLatin1Char('P'))) {
                    bool ok = false;
                    int num = p.code.mid(1).toInt(&ok);
                    if (ok && num > maxNum) {
                        maxNum = num;
                    }
                }
            }
            created.code = QStringLiteral("P%1").arg(maxNum + 1, 4, 10, QLatin1Char('0'));
        }
        products.push_back(created);
        auto callback = std::move(pendingCreateCallback_);
        resetPendingCreate();
        callback(ProductOperationResult { true, created, std::nullopt });
    }

    void completePendingCreateProductError(const AppError& error)
    {
        completePendingCreateProduct(ProductOperationResult { false, std::nullopt, error });
    }

    // ===== updateProduct 完成 =====

    void completePendingUpdateProduct(const ProductOperationResult& result)
    {
        if (!pendingUpdateCallback_) {
            return;
        }
        if (pendingUpdateOwner_.isNull()) {
            resetPendingUpdate();
            return;
        }
        auto callback = std::move(pendingUpdateCallback_);
        resetPendingUpdate();
        callback(result);
    }

    // 以成功完成:同时把更新应用到内存仓库
    void completePendingUpdateProductSuccess()
    {
        if (!pendingUpdateCallback_) {
            return;
        }
        if (pendingUpdateOwner_.isNull()) {
            resetPendingUpdate();
            return;
        }
        Product updated = pendingUpdateProduct_.value_or(Product {});
        auto found = std::find_if(products.begin(), products.end(),
            [id = updated.id](const Product& item) { return item.id == id; });
        if (found != products.end()) {
            *found = updated;
        }
        auto callback = std::move(pendingUpdateCallback_);
        resetPendingUpdate();
        callback(ProductOperationResult { true, updated, std::nullopt });
    }

    void completePendingUpdateProductError(const AppError& error)
    {
        completePendingUpdateProduct(ProductOperationResult { false, std::nullopt, error });
    }

    // ===== setProductActive 完成 =====

    void completePendingSetProductActive(const std::optional<AppError>& error)
    {
        if (!pendingActiveCallback_) {
            return;
        }
        if (pendingActiveOwner_.isNull()) {
            resetPendingActive();
            return;
        }
        auto callback = std::move(pendingActiveCallback_);
        resetPendingActive();
        callback(error);
    }

    // 以成功完成:同时把状态变更应用到内存仓库
    void completePendingSetProductActiveSuccess()
    {
        if (!pendingActiveCallback_) {
            return;
        }
        if (pendingActiveOwner_.isNull()) {
            resetPendingActive();
            return;
        }
        const quint32 id = pendingActiveProductId_.value_or(0);
        const bool active = pendingActiveValue_.value_or(false);
        auto found = std::find_if(products.begin(), products.end(),
            [id](const Product& item) { return item.id == id; });
        if (found != products.end()) {
            found->active = active;
        }
        auto callback = std::move(pendingActiveCallback_);
        resetPendingActive();
        callback(std::nullopt);
    }

    void completePendingSetProductActiveError(const AppError& error)
    {
        completePendingSetProductActive(error);
    }

    // ===== 辅助 =====

    void addProduct(const Product& product)
    {
        products.push_back(product);
        nextId_ = std::max(nextId_, product.id + 1);
    }

    // 重置全部状态(不会触发任何挂起回调,直接丢弃)
    void clear()
    {
        products.clear();
        lastFilter.reset();
        lastPageRequest.reset();
        lastCreatedProduct.reset();
        lastUpdatedProduct.reset();
        lastActiveProductId.reset();
        lastActiveValue.reset();
        nextListError.reset();
        nextFindError.reset();
        nextCreateError.reset();
        nextUpdateError.reset();
        nextActiveError.reset();
        deferListProducts = false;
        deferCreateProduct = false;
        deferUpdateProduct = false;
        deferSetProductActive = false;

        pendingListOps_.clear();
        resetPendingCreate();
        resetPendingUpdate();
        resetPendingActive();
        nextId_ = 1;
    }

    // ===== IProductRepository 实现 =====

    void listProducts(
        const ProductFilter& filter,
        const PageRequest& pageRequest,
        QObject* owner,
        PageCallback callback) override
    {
        QPointer<QObject> ownerPtr(owner);
        if (ownerPtr.isNull() || !callback) {
            return;
        }

        lastFilter = filter;
        lastPageRequest = pageRequest;

        if (deferListProducts) {
            PendingListOp op;
            op.callback = std::move(callback);
            op.filter = filter;
            op.pageRequest = pageRequest;
            op.owner = ownerPtr;
            pendingListOps_.append(std::move(op));
            return;
        }

        if (nextListError.has_value()) {
            const auto error = nextListError;
            nextListError.reset();
            callback(ProductPageResult { false, {}, error });
            return;
        }

        callback(buildListProductsSuccessResult(filter, pageRequest));
    }

    void findByCode(
        const QString& code,
        QObject* owner,
        OperateCallback callback) override
    {
        QPointer<QObject> ownerPtr(owner);
        if (ownerPtr.isNull() || !callback) {
            return;
        }

        if (nextFindError.has_value()) {
            const auto error = nextFindError;
            nextFindError.reset();
            callback(ProductOperationResult { false, std::nullopt, error });
            return;
        }

        const auto normalizedCode = code.trimmed();
        const auto found = std::find_if(products.cbegin(), products.cend(),
            [&normalizedCode](const Product& product) {
                return product.code == normalizedCode;
            });

        if (found == products.cend()) {
            callback(ProductOperationResult { true, std::nullopt, std::nullopt });
            return;
        }

        callback(ProductOperationResult { true, *found, std::nullopt });
    }

    void createProduct(
        const Product& product,
        const AuditContext& /* auditContext */,
        QObject* owner,
        OperateCallback callback) override
    {
        QPointer<QObject> ownerPtr(owner);
        if (ownerPtr.isNull() || !callback) {
            return;
        }

        lastCreatedProduct = product;

        if (deferCreateProduct) {
            pendingCreateCallback_ = std::move(callback);
            pendingCreateProduct_ = product;
            pendingCreateOwner_ = ownerPtr;
            return;
        }

        if (nextCreateError.has_value()) {
            const auto error = nextCreateError;
            nextCreateError.reset();
            callback(ProductOperationResult { false, std::nullopt, error });
            return;
        }

        auto created = product;
        if (created.id == 0) {
            created.id = nextId_++;
        } else {
            nextId_ = std::max(nextId_, created.id + 1);
        }
        if (created.code.trimmed().isEmpty()) {
            int maxNum = 0;
            for (const auto& p : products) {
                if (p.code.length() > 1 && p.code.startsWith(QLatin1Char('P'))) {
                    bool ok = false;
                    int num = p.code.mid(1).toInt(&ok);
                    if (ok && num > maxNum) {
                        maxNum = num;
                    }
                }
            }
            created.code = QStringLiteral("P%1").arg(maxNum + 1, 4, 10, QLatin1Char('0'));
        }

        products.push_back(created);
        callback(ProductOperationResult { true, created, std::nullopt });
    }

    void updateProduct(
        const Product& product,
        const AuditContext& /* auditContext */,
        QObject* owner,
        OperateCallback callback) override
    {
        QPointer<QObject> ownerPtr(owner);
        if (ownerPtr.isNull() || !callback) {
            return;
        }

        lastUpdatedProduct = product;

        if (deferUpdateProduct) {
            pendingUpdateCallback_ = std::move(callback);
            pendingUpdateProduct_ = product;
            pendingUpdateOwner_ = ownerPtr;
            return;
        }

        if (nextUpdateError.has_value()) {
            const auto error = nextUpdateError;
            nextUpdateError.reset();
            callback(ProductOperationResult { false, std::nullopt, error });
            return;
        }

        auto found = std::find_if(products.begin(), products.end(),
            [id = product.id](const Product& item) {
                return item.id == id;
            });

        if (found == products.end()) {
            callback(ProductOperationResult {
                false,
                std::nullopt,
                AppError::repositoryFailure(QStringLiteral("产品不存在")) });
            return;
        }

        *found = product;
        callback(ProductOperationResult { true, product, std::nullopt });
    }

    void setProductActive(
        quint32 id,
        bool active,
        const AuditContext& /* auditContext */,
        QObject* owner,
        ActiveCallback callback) override
    {
        QPointer<QObject> ownerPtr(owner);
        if (ownerPtr.isNull() || !callback) {
            return;
        }

        lastActiveProductId = id;
        lastActiveValue = active;

        if (deferSetProductActive) {
            pendingActiveCallback_ = std::move(callback);
            pendingActiveProductId_ = id;
            pendingActiveValue_ = active;
            pendingActiveOwner_ = ownerPtr;
            return;
        }

        if (nextActiveError.has_value()) {
            const auto error = nextActiveError;
            nextActiveError.reset();
            callback(error);
            return;
        }

        auto found = std::find_if(products.begin(), products.end(),
            [id](const Product& product) {
                return product.id == id;
            });

        if (found == products.end()) {
            callback(AppError::repositoryFailure(QStringLiteral("产品不存在")));
            return;
        }

        found->active = active;
        callback(std::nullopt);
    }

private:
    // 一个挂起的 listProducts 请求
    struct PendingListOp {
        PageCallback callback;
        ProductFilter filter;
        PageRequest pageRequest;
        QPointer<QObject> owner;
    };

    void completePendingListProductsAt(int index, const ProductPageResult& result)
    {
        if (index < 0 || index >= pendingListOps_.size()) {
            return;
        }
        PendingListOp op = std::move(pendingListOps_[index]);
        pendingListOps_.removeAt(index);
        // owner 已在挂起期间销毁:丢弃回调,避免悬空
        if (op.owner.isNull() || !op.callback) {
            return;
        }
        op.callback(result);
    }

    void resetPendingCreate() noexcept
    {
        pendingCreateCallback_ = nullptr;
        pendingCreateProduct_.reset();
        pendingCreateOwner_.clear();
    }

    void resetPendingUpdate() noexcept
    {
        pendingUpdateCallback_ = nullptr;
        pendingUpdateProduct_.reset();
        pendingUpdateOwner_.clear();
    }

    void resetPendingActive() noexcept
    {
        pendingActiveCallback_ = nullptr;
        pendingActiveProductId_.reset();
        pendingActiveValue_.reset();
        pendingActiveOwner_.clear();
    }

    ProductPageResult buildListProductsSuccessResult(
        const ProductFilter& filter,
        const PageRequest& pageRequest) const
    {
        QVector<ProductListItemDto> filtered;
        for (const auto& product : products) {
            if (!matchesFilter(product, filter)) {
                continue;
            }
            filtered.push_back(toListItem(product));
        }

        PageResult<ProductListItemDto> page;
        page.total = filtered.size();
        page.page = pageRequest.page;
        page.pageSize = pageRequest.pageSize;

        const auto start = (pageRequest.page - 1) * pageRequest.pageSize;
        if (start < filtered.size()) {
            const auto end = qMin(start + pageRequest.pageSize, filtered.size());
            for (auto index = start; index < end; ++index) {
                page.items.push_back(filtered.at(index));
            }
        }

        return ProductPageResult { true, page, std::nullopt };
    }

    static bool matchesFilter(const Product& product, const ProductFilter& filter)
    {
        if (!filter.keyword.trimmed().isEmpty()) {
            const auto keyword = filter.keyword.trimmed();
            if (!product.code.contains(keyword, Qt::CaseInsensitive)
                && !product.name.contains(keyword, Qt::CaseInsensitive)
                && !product.specification.contains(keyword, Qt::CaseInsensitive)) {
                return false;
            }
        }

        if (filter.categoryId.has_value() && product.categoryId != filter.categoryId.value()) {
            return false;
        }

        if (filter.active.has_value() && product.active != filter.active.value()) {
            return false;
        }

        return true;
    }

    static ProductListItemDto toListItem(const Product& product)
    {
        return ProductListItemDto {
            product.id,
            product.code,
            product.name,
            product.categoryId,
            QString("默认分类"),
            product.unitId,
            QString("默认单位"),
            product.specification,
            product.safetyStock,
            product.active
        };
    }

    quint32 nextId_ { 1 };

    // listProducts 支持多个并行挂起请求
    QVector<PendingListOp> pendingListOps_;

    OperateCallback pendingCreateCallback_;
    std::optional<Product> pendingCreateProduct_;
    QPointer<QObject> pendingCreateOwner_;

    OperateCallback pendingUpdateCallback_;
    std::optional<Product> pendingUpdateProduct_;
    QPointer<QObject> pendingUpdateOwner_;

    ActiveCallback pendingActiveCallback_;
    std::optional<quint32> pendingActiveProductId_;
    std::optional<bool> pendingActiveValue_;
    QPointer<QObject> pendingActiveOwner_;
};