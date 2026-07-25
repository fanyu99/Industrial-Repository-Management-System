#pragma once

#include "IProductRepository.h"

#include <QPointer>
#include <QVector>
#include <algorithm>
#include <utility>

// 用于 ProductService 单元测试的内存仓库,不连接真实数据库。
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

    [[nodiscard]] bool hasPendingListProducts() const noexcept
    {
        return static_cast<bool>(pendingListCallback_);
    }

    void completePendingListProductsSuccess()
    {
        if (!pendingListCallback_ || !pendingListFilter_.has_value() || !pendingListPageRequest_.has_value()) {
            return;
        }

        completePendingListProducts(buildListProductsSuccessResult(
            pendingListFilter_.value(),
            pendingListPageRequest_.value()));
    }

    void completePendingListProducts(const ProductPageResult& result)
    {
        if (!pendingListCallback_) {
            return;
        }

        auto callback = std::move(pendingListCallback_);
        pendingListCallback_ = nullptr;
        pendingListFilter_.reset();
        pendingListPageRequest_.reset();
        callback(result);
    }

    void addProduct(const Product& product)
    {
        products.push_back(product);
        nextId_ = std::max(nextId_, product.id + 1);
    }

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
        pendingListCallback_ = nullptr;
        pendingListFilter_.reset();
        pendingListPageRequest_.reset();
        nextId_ = 1;
    }

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
            pendingListCallback_ = std::move(callback);
            pendingListFilter_ = filter;
            pendingListPageRequest_ = pageRequest;
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
        QObject* owner,
        OperateCallback callback) override
    {
        QPointer<QObject> ownerPtr(owner);
        if (ownerPtr.isNull() || !callback) {
            return;
        }

        lastCreatedProduct = product;

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

        products.push_back(created);
        callback(ProductOperationResult { true, created, std::nullopt });
    }

    void updateProduct(
        const Product& product,
        QObject* owner,
        OperateCallback callback) override
    {
        QPointer<QObject> ownerPtr(owner);
        if (ownerPtr.isNull() || !callback) {
            return;
        }

        lastUpdatedProduct = product;

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
        QObject* owner,
        ActiveCallback callback) override
    {
        QPointer<QObject> ownerPtr(owner);
        if (ownerPtr.isNull() || !callback) {
            return;
        }

        lastActiveProductId = id;
        lastActiveValue = active;

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
            QString::number(product.categoryId),
            QString::number(product.unitId),
            product.specification,
            product.safetyStock,
            product.active
        };
    }

    quint32 nextId_ { 1 };
    PageCallback pendingListCallback_;
    std::optional<ProductFilter> pendingListFilter_;
    std::optional<PageRequest> pendingListPageRequest_;
};
