#pragma once

#include "IInboundRepository.h"

#include <QDateTime>
#include <QPointer>
#include <QVector>
#include <algorithm>
#include <optional>
#include <utility>

// 用于 InboundService 单元测试的内存仓库,不连接真实数据库。
//
// 关键语义(由 InboundService 的回调校验推导而来,务必保持一致):
//   - createDraft:Service 传入的订单 id==0、orderNo 为空、订单行 orderId==0。
//     本仓库必须为其分配 id、生成非空 orderNo、回填订单行 orderId 与订单 id 一致、
//     保持 status=Draft,并写入内存,否则 Service 的 validateCreateInboundOrder 会失败。
//   - confirmOrder:Service 直接调用(不预先查询,以避免竟态)。本仓库需原子地
//     按 id 查找订单,仅允许 Draft 状态被确认,将状态迁移为 Confirmed 并设置 confirmedAt,
//     返回确认后的订单,否则 Service 的 validateConfirmInboundOrder 会失败。
//   - findById/findByOrderNo:Service 当前未使用,按 findByCode 的约定实现
//     (未找到时返回 success=true、order=nullopt、error=nullopt)。
//
// 异步测试能力(与 FakeProductRepository 基本一致,并做了一处改进):
//   - 通过 deferXxx 标志可以把 listOrders/createDraft/confirmOrder 挂起,
//     稍后用 completePendingXxx 显式完成,模拟真实仓储的异步时序。
//   - 挂起期间会记录 owner 的 QPointer。当 owner 在延迟期间被销毁时,
//     completePendingXxx 不会触发回调(避免悬空回调 / 访问已销毁对象),
//     与真实 MySqlInboundRepository 中 PendingRequest.owner 的保护行为保持一致。
//   - listOrders 支持多个并行挂起请求(队列),可按下标完成,用于测试
//     "latest-wins"(旧查询结果不覆盖新查询)。
//   - 错误注入(nextXxxError)在立即完成与延迟完成路径中均生效:
//     completePendingXxxSuccess() 会先消费 nextXxxError,若已设置则按错误完成,
//     避免"测试配置了错误却仍走成功路径"的假象(FakeProductRepository 未做此处理)。
class FakeInboundRepository : public IInboundRepository {
public:
    QVector<InboundOrder> orders;
    std::optional<AppError> nextListError;
    std::optional<AppError> nextFindError;
    std::optional<AppError> nextCreateError;
    std::optional<AppError> nextConfirmError;

    std::optional<InboundOrderFilter> lastListFilter;
    std::optional<PageRequest> lastListPageRequest;
    std::optional<InboundOrder> lastCreateDraftOrder;
    std::optional<quint32> lastConfirmOrderId;
    std::optional<quint32> lastConfirmOperatorId;

    bool deferListOrders { false };
    bool deferCreateDraft { false };
    bool deferConfirmOrder { false };

    // ===== 挂起状态查询 =====

    [[nodiscard]] bool hasPendingListOrders() const noexcept
    {
        return !pendingListOps_.isEmpty();
    }

    // 当前挂起的 listOrders 请求数量(支持多个并行挂起)
    [[nodiscard]] int pendingListCount() const noexcept
    {
        return pendingListOps_.size();
    }

    [[nodiscard]] bool hasPendingCreateDraft() const noexcept
    {
        return static_cast<bool>(pendingCreateCallback_);
    }

    [[nodiscard]] bool hasPendingConfirmOrder() const noexcept
    {
        return static_cast<bool>(pendingConfirmCallback_);
    }

    // ===== listOrders 完成 =====
    // index 默认为 0(最早挂起的请求),支持多挂起时按下标完成。

    void completePendingListOrdersSuccess(int index = 0)
    {
        if (index < 0 || index >= pendingListOps_.size()) {
            return;
        }
        const auto& op = pendingListOps_.at(index);
        // owner 已在挂起期间销毁:丢弃回调,避免悬空(不消费 nextListError)
        if (op.owner.isNull()) {
            pendingListOps_.removeAt(index);
            return;
        }
        // 错误注入优先:与立即完成路径一致,先消费 nextListError,
        // 避免"配置了错误却走了成功路径"的假象
        if (nextListError.has_value()) {
            const auto error = nextListError;
            nextListError.reset();
            completePendingListOrdersAt(index, InboundPageResult { false, {}, error });
            return;
        }
        completePendingListOrdersAt(index,
            buildListOrdersSuccessResult(op.filter, op.pageRequest));
    }

    void completePendingListOrders(const InboundPageResult& result, int index = 0)
    {
        completePendingListOrdersAt(index, result);
    }

    void completePendingListOrdersError(const AppError& error, int index = 0)
    {
        completePendingListOrdersAt(index, InboundPageResult { false, {}, error });
    }

    // ===== createDraft 完成 =====

    void completePendingCreateDraft(const InboundOperationResult& result)
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

    // 以成功完成:同时把订单写入内存仓库(与立即完成路径一致)
    void completePendingCreateDraftSuccess()
    {
        if (!pendingCreateCallback_) {
            return;
        }
        if (pendingCreateOwner_.isNull()) {
            resetPendingCreate();
            return;
        }
        // 错误注入优先:与立即完成路径一致,先消费 nextCreateError,
        // 避免"配置了错误却走了成功路径"的假象
        if (nextCreateError.has_value()) {
            const auto error = nextCreateError;
            nextCreateError.reset();
            auto callback = std::move(pendingCreateCallback_);
            resetPendingCreate();
            callback(InboundOperationResult { false, std::nullopt, error });
            return;
        }
        InboundOrder order = pendingCreateOrder_.value_or(InboundOrder {});
        auto callback = std::move(pendingCreateCallback_);
        resetPendingCreate();
        callback(performCreateDraft(order));
    }

    void completePendingCreateDraftError(const AppError& error)
    {
        completePendingCreateDraft(InboundOperationResult { false, std::nullopt, error });
    }

    // ===== confirmOrder 完成 =====

    void completePendingConfirmOrder(const InboundOperationResult& result)
    {
        if (!pendingConfirmCallback_) {
            return;
        }
        if (pendingConfirmOwner_.isNull()) {
            resetPendingConfirm();
            return;
        }
        auto callback = std::move(pendingConfirmCallback_);
        resetPendingConfirm();
        callback(result);
    }

    // 以成功完成:同时把状态变更应用到内存仓库
    void completePendingConfirmOrderSuccess()
    {
        if (!pendingConfirmCallback_) {
            return;
        }
        if (pendingConfirmOwner_.isNull()) {
            resetPendingConfirm();
            return;
        }
        // 错误注入优先:与立即完成路径一致,先消费 nextConfirmError,
        // 避免"配置了错误却走了成功路径"的假象
        if (nextConfirmError.has_value()) {
            const auto error = nextConfirmError;
            nextConfirmError.reset();
            auto callback = std::move(pendingConfirmCallback_);
            resetPendingConfirm();
            callback(InboundOperationResult { false, std::nullopt, error });
            return;
        }
        const quint32 id = pendingConfirmId_;
        const quint32 operatorId = pendingConfirmOperatorId_;
        auto callback = std::move(pendingConfirmCallback_);
        resetPendingConfirm();
        callback(performConfirm(id, operatorId));
    }

    void completePendingConfirmOrderError(const AppError& error)
    {
        completePendingConfirmOrder(InboundOperationResult { false, std::nullopt, error });
    }

    // ===== 辅助 =====

    void addOrder(const InboundOrder& order)
    {
        orders.push_back(order);
        nextId_ = std::max(nextId_, order.id + 1);
        for (const auto& line : order.lines) {
            nextLineId_ = std::max(nextLineId_, line.id + 1);
        }
    }

    // 重置全部状态(不会触发任何挂起回调,直接丢弃)
    void clear()
    {
        orders.clear();
        lastListFilter.reset();
        lastListPageRequest.reset();
        lastCreateDraftOrder.reset();
        lastConfirmOrderId.reset();
        lastConfirmOperatorId.reset();
        nextListError.reset();
        nextFindError.reset();
        nextCreateError.reset();
        nextConfirmError.reset();
        deferListOrders = false;
        deferCreateDraft = false;
        deferConfirmOrder = false;

        pendingListOps_.clear();
        resetPendingCreate();
        resetPendingConfirm();
        nextId_ = 1;
        nextLineId_ = 1;
    }

    // ===== IInboundRepository 实现 =====

    void listOrders(
        const InboundOrderFilter& filter,
        const PageRequest& pageRequest,
        QObject* owner,
        PageCallback callback) override
    {
        QPointer<QObject> ownerPtr(owner);
        if (ownerPtr.isNull() || !callback) {
            return;
        }

        lastListFilter = filter;
        lastListPageRequest = pageRequest;

        if (deferListOrders) {
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
            callback(InboundPageResult { false, {}, error });
            return;
        }

        callback(buildListOrdersSuccessResult(filter, pageRequest));
    }

    void findById(
        quint32 id,
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
            callback(InboundOperationResult { false, std::nullopt, error });
            return;
        }

        const auto found = std::find_if(orders.cbegin(), orders.cend(),
            [id](const InboundOrder& order) {
                return order.id == id;
            });

        if (found == orders.cend()) {
            callback(InboundOperationResult { true, std::nullopt, std::nullopt });
            return;
        }

        callback(InboundOperationResult { true, *found, std::nullopt });
    }

    void findByOrderNo(
        const QString& orderNo,
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
            callback(InboundOperationResult { false, std::nullopt, error });
            return;
        }

        const auto normalizedOrderNo = orderNo.trimmed();
        const auto found = std::find_if(orders.cbegin(), orders.cend(),
            [&normalizedOrderNo](const InboundOrder& order) {
                return order.orderNo == normalizedOrderNo;
            });

        if (found == orders.cend()) {
            callback(InboundOperationResult { true, std::nullopt, std::nullopt });
            return;
        }

        callback(InboundOperationResult { true, *found, std::nullopt });
    }

    void createDraft(
        const InboundOrder& order,
        QObject* owner,
        OperateCallback callback) override
    {
        QPointer<QObject> ownerPtr(owner);
        if (ownerPtr.isNull() || !callback) {
            return;
        }

        lastCreateDraftOrder = order;

        if (deferCreateDraft) {
            pendingCreateCallback_ = std::move(callback);
            pendingCreateOrder_ = order;
            pendingCreateOwner_ = ownerPtr;
            return;
        }

        if (nextCreateError.has_value()) {
            const auto error = nextCreateError;
            nextCreateError.reset();
            callback(InboundOperationResult { false, std::nullopt, error });
            return;
        }

        callback(performCreateDraft(order));
    }

    void confirmOrder(
        quint32 id,
        quint32 operatorId,
        QObject* owner,
        OperateCallback callback) override
    {
        QPointer<QObject> ownerPtr(owner);
        if (ownerPtr.isNull() || !callback) {
            return;
        }

        lastConfirmOrderId = id;
        lastConfirmOperatorId = operatorId;

        if (deferConfirmOrder) {
            pendingConfirmCallback_ = std::move(callback);
            pendingConfirmId_ = id;
            pendingConfirmOperatorId_ = operatorId;
            pendingConfirmOwner_ = ownerPtr;
            return;
        }

        if (nextConfirmError.has_value()) {
            const auto error = nextConfirmError;
            nextConfirmError.reset();
            callback(InboundOperationResult { false, std::nullopt, error });
            return;
        }

        callback(performConfirm(id, operatorId));
    }

private:
    // 一个挂起的 listOrders 请求
    struct PendingListOp {
        PageCallback callback;
        InboundOrderFilter filter;
        PageRequest pageRequest;
        QPointer<QObject> owner;
    };

    void completePendingListOrdersAt(int index, const InboundPageResult& result)
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
        pendingCreateOrder_.reset();
        pendingCreateOwner_.clear();
    }

    void resetPendingConfirm() noexcept
    {
        pendingConfirmCallback_ = nullptr;
        pendingConfirmId_ = 0;
        pendingConfirmOperatorId_ = 0;
        pendingConfirmOwner_.clear();
    }

    InboundPageResult buildListOrdersSuccessResult(
        const InboundOrderFilter& filter,
        const PageRequest& pageRequest) const
    {
        QVector<InboundOrderListItemDto> filtered;
        for (const auto& order : orders) {
            if (!matchesFilter(order, filter)) {
                continue;
            }
            filtered.push_back(toListItem(order));
        }

        PageResult<InboundOrderListItemDto> page;
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

        return InboundPageResult { true, page, std::nullopt };
    }

    static bool matchesFilter(const InboundOrder& order, const InboundOrderFilter& filter)
    {
        if (filter.status.has_value() && order.status != filter.status.value()) {
            return false;
        }

        if (filter.warehouseId.has_value() && order.warehouseId != filter.warehouseId.value()) {
            return false;
        }

        if (!filter.keyword.trimmed().isEmpty()) {
            const auto keyword = filter.keyword.trimmed();
            if (!order.orderNo.contains(keyword, Qt::CaseInsensitive)
                && !order.supplier.contains(keyword, Qt::CaseInsensitive)) {
                return false;
            }
        }

        return true;
    }

    static InboundOrderListItemDto toListItem(const InboundOrder& order)
    {
        return InboundOrderListItemDto {
            order.id,
                order.orderNo,
                order.supplier,
                order.status,
                order.operatorId,
                QString(),
                order.warehouseId,
                QString()
        };
    }

    // 生成订单号(基于自增 id,保证唯一且非空,供 Service 传入空 orderNo 时使用)
    QString generateOrderNo(quint32 id) const
    {
        return QStringLiteral("IN%1").arg(static_cast<qulonglong>(id), 6, 10, QChar('0'));
    }

    // createDraft 的实际持久化逻辑:分配 id、生成 orderNo、回填订单行 orderId、写入内存仓库。
    // 立即完成路径与 completePendingCreateDraftSuccess 共用此逻辑。
    InboundOperationResult performCreateDraft(const InboundOrder& order)
    {
        // 订单号重复校验(Service 传入的 orderNo 通常为空,由本仓库生成;
        // 仅当调用方显式指定已存在的 orderNo 时才判定重复)
        if (!order.orderNo.trimmed().isEmpty()) {
            const auto duplicated = std::find_if(orders.cbegin(), orders.cend(),
                [&order](const InboundOrder& existing) {
                    return existing.orderNo == order.orderNo;
                });
            if (duplicated != orders.cend()) {
                return InboundOperationResult {
                    false,
                    std::nullopt,
                    AppError {
                        AppErrorCategory::Database,
                        AppErrorCode::DuplicateInboundOrder,
                        QStringLiteral("入库订单号已存在") }
                };
            }
        }

        InboundOrder created = order;
        if (created.id == 0) {
            created.id = nextId_++;
        } else {
            nextId_ = std::max(nextId_, created.id + 1);
        }
        // 确保orderNo在INSERT前已经生成(Service 允许为空,数据库层不允许为空)
        if (created.orderNo.trimmed().isEmpty()) {
            created.orderNo = generateOrderNo(created.id);
        }
        created.status = InboundOrderStatus::Draft;
        const auto now = QDateTime::currentDateTime();
        created.createdAt = now;
        created.updatedAt = now;
        // 回填订单行:分配行 id,并使 orderId 与订单 id 一致(供 Service 校验)
        for (auto& line : created.lines) {
            if (line.id == 0) {
                line.id = nextLineId_++;
            } else {
                nextLineId_ = std::max(nextLineId_, line.id + 1);
            }
            line.orderId = created.id;
        }

        orders.push_back(created);
        return InboundOperationResult { true, created, std::nullopt };
    }

    // confirmOrder 的实际迁移逻辑:按 id 查找、校验状态、Draft -> Confirmed。
    // 立即完成路径与 completePendingConfirmOrderSuccess 共用此逻辑。
    InboundOperationResult performConfirm(quint32 id, quint32 operatorId)
    {
        auto found = std::find_if(orders.begin(), orders.end(),
            [id](const InboundOrder& order) {
                return order.id == id;
            });

        if (found == orders.end()) {
            return InboundOperationResult {
                false,
                std::nullopt,
                AppError {
                    AppErrorCategory::Database,
                    AppErrorCode::InboundOrderNotFound,
                    QStringLiteral("入库订单不存在") }
            };
        }

        if (found->status != InboundOrderStatus::Draft) {
            return InboundOperationResult {
                false,
                std::nullopt,
                AppError {
                    AppErrorCategory::Database,
                    AppErrorCode::InvalidInboundOrder,
                    QStringLiteral("订单状态不允许确认") }
            };
        }

        found->status = InboundOrderStatus::Confirmed;
        const auto now = QDateTime::currentDateTime();
        found->confirmedAt = now;
        found->updatedAt = now;
        // operatorId 保留为订单创建者;传入的 operatorId 仅供真实仓储做审计,
        // 此处已记录到 lastConfirmOperatorId 供测试断言。
        (void)operatorId;

        return InboundOperationResult { true, *found, std::nullopt };
    }

    quint32 nextId_ { 1 };
    quint32 nextLineId_ { 1 };

    // listOrders 支持多个并行挂起请求
    QVector<PendingListOp> pendingListOps_;

    OperateCallback pendingCreateCallback_;
    std::optional<InboundOrder> pendingCreateOrder_;
    QPointer<QObject> pendingCreateOwner_;

    OperateCallback pendingConfirmCallback_;
    quint32 pendingConfirmId_ { 0 };
    quint32 pendingConfirmOperatorId_ { 0 };
    QPointer<QObject> pendingConfirmOwner_;
};
