#pragma once

#include "IOutboundRepository.h"

#include <QDateTime>
#include <QPointer>
#include <QVector>
#include <algorithm>
#include <optional>
#include <utility>

// 用于 OutboundService 单元测试的内存仓库,不连接真实数据库。
//
// 关键语义(由 OutboundService 的回调校验推导而来,务必保持一致):
//   - createDraft:Service 传入的订单 id==0、orderNo 为空、订单行 orderId==0。
//     本仓库必须为其分配 id、生成非空 orderNo、回填订单行 orderId 与订单 id 一致、
//     保持 status=Draft,并写入内存,否则 Service 的 validateCreateOutboundOrder 会失败。
//   - confirmOrder:Service 直接调用(不预先查询,以避免竟态)。本仓库需原子地
//     按 id 查找订单,仅允许 Draft 状态被确认,将状态迁移为 Confirmed 并设置 confirmedAt,
//     返回确认后的订单,否则 Service 的 validateConfirmOutboundOrder 会失败。
//     与入库不同,出库确认还需模拟库存扣减逻辑:
//     - 条件更新:仅当 stock_balance.quantity >= 出库明细数量时扣减成功
//     - affectedRows==0 需要区分三种原因:
//       a) 该产品在该仓库没有库存余额记录(无库存记录)
//       b) 库存不足(可用数量 < 出库明细数量)
//       c) 订单状态不匹配(非草稿)
//     - a)与 b)业务上等同"库存无法满足出库",都映射为 InsufficientStock(仅错误消息区分);
//       不能映射为 OutboundOrderNotFound(订单存在,语义不符)
//     - 任一明细库存不足,整个出库事务回滚;MVP 不支持部分出库
//     - 同一订单允许同一产品多行明细,校验与扣减按行执行(模拟条件更新),
//       多行合计超过可用库存时整体回滚,不允许库存变成负数
//   - findById/findByOrderNo:Service 当前未使用,按 findByCode 的约定实现
//     (未找到时返回 success=true、order=nullopt、error=nullopt)。
//
// 异步测试能力(与 FakeInboundRepository 一致):
//   - 通过 deferXxx 标志可以把 listOrders/createDraft/confirmOrder 挂起,
//     稍后用 completePendingXxx 显式完成,模拟真实仓储的异步时序。
//   - 挂起期间会记录 owner 的 QPointer。当 owner 在延迟期间被销毁时,
//     completePendingXxx 不会触发回调(避免悬空回调 / 访问已销毁对象),
//     与真实 MySqlOutboundRepository 中 PendingRequest.owner 的保护行为保持一致。
//   - listOrders 支持多个并行挂起请求(队列),可按下标完成,用于测试
//     "latest-wins"(旧查询结果不覆盖新查询)。
//   - 错误注入(nextXxxError)在立即完成与延迟完成路径中均生效:
//     completePendingXxxSuccess() 会先消费 nextXxxError,若已设置则按错误完成,
//     避免"测试配置了错误却仍走成功路径"的假象。
class FakeOutboundRepository : public IOutboundRepository {
public:
    QVector<OutboundOrder> orders;
    // 库存模拟: key = "productId_warehouseId", value = 可用数量
    QHash<QString, int> stockBalances;
    std::optional<AppError> nextListError;
    std::optional<AppError> nextFindError;
    std::optional<AppError> nextCreateError;
    std::optional<AppError> nextConfirmError;

    std::optional<OutboundOrderFilter> lastListFilter;
    std::optional<PageRequest> lastListPageRequest;
    std::optional<OutboundOrder> lastCreateDraftOrder;
    std::optional<quint32> lastConfirmOrderId;
    std::optional<quint32> lastConfirmOperatorId;

    bool deferListOrders { false };
    bool deferCreateDraft { false };
    bool deferConfirmOrder { false };
    bool deferFindById { false };
    bool deferFindByOrderNo { false };

    // ===== 库存辅助方法 =====

    static QString stockKey(quint32 productId, quint32 warehouseId)
    {
        return QStringLiteral("%1_%2").arg(productId).arg(warehouseId);
    }

    void setStock(quint32 productId, quint32 warehouseId, int quantity)
    {
        stockBalances[stockKey(productId, warehouseId)] = quantity;
    }

    int getStock(quint32 productId, quint32 warehouseId) const
    {
        return stockBalances.value(stockKey(productId, warehouseId), 0);
    }

    // ===== 挂起状态查询 =====

    [[nodiscard]] bool hasPendingListOrders() const noexcept
    {
        return !pendingListOps_.isEmpty();
    }

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

    [[nodiscard]] bool hasPendingFindById() const noexcept
    {
        return static_cast<bool>(pendingFindByIdCallback_);
    }

    [[nodiscard]] bool hasPendingFindByOrderNo() const noexcept
    {
        return static_cast<bool>(pendingFindByOrderNoCallback_);
    }

    // ===== listOrders 完成 =====

    void completePendingListOrdersSuccess(int index = 0)
    {
        if (index < 0 || index >= pendingListOps_.size()) {
            return;
        }
        const auto& op = pendingListOps_.at(index);
        if (op.owner.isNull()) {
            pendingListOps_.removeAt(index);
            return;
        }
        if (nextListError.has_value()) {
            const auto error = nextListError;
            nextListError.reset();
            completePendingListOrdersAt(index, OutboundPageResult { false, {}, error });
            return;
        }
        completePendingListOrdersAt(index,
            buildListOrdersSuccessResult(op.filter, op.pageRequest));
    }

    void completePendingListOrders(const OutboundPageResult& result, int index = 0)
    {
        completePendingListOrdersAt(index, result);
    }

    void completePendingListOrdersError(const AppError& error, int index = 0)
    {
        completePendingListOrdersAt(index, OutboundPageResult { false, {}, error });
    }

    // ===== createDraft 完成 =====

    void completePendingCreateDraft(const OutboundOperationResult& result)
    {
        if (!pendingCreateCallback_) {
            return;
        }
        if (pendingCreateOwner_.isNull()) {
            resetPendingCreate();
            return;
        }
        auto callback = std::move(pendingCreateCallback_);
        resetPendingCreate();
        callback(result);
    }

    void completePendingCreateDraftSuccess()
    {
        if (!pendingCreateCallback_) {
            return;
        }
        if (pendingCreateOwner_.isNull()) {
            resetPendingCreate();
            return;
        }
        if (nextCreateError.has_value()) {
            const auto error = nextCreateError;
            nextCreateError.reset();
            auto callback = std::move(pendingCreateCallback_);
            resetPendingCreate();
            callback(OutboundOperationResult { false, std::nullopt, error });
            return;
        }
        OutboundOrder order = pendingCreateOrder_.value_or(OutboundOrder {});
        auto callback = std::move(pendingCreateCallback_);
        resetPendingCreate();
        callback(performCreateDraft(order));
    }

    void completePendingCreateDraftError(const AppError& error)
    {
        completePendingCreateDraft(OutboundOperationResult { false, std::nullopt, error });
    }

    // ===== confirmOrder 完成 =====

    void completePendingConfirmOrder(const OutboundOperationResult& result)
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

    void completePendingConfirmOrderSuccess()
    {
        if (!pendingConfirmCallback_) {
            return;
        }
        if (pendingConfirmOwner_.isNull()) {
            resetPendingConfirm();
            return;
        }
        if (nextConfirmError.has_value()) {
            const auto error = nextConfirmError;
            nextConfirmError.reset();
            auto callback = std::move(pendingConfirmCallback_);
            resetPendingConfirm();
            callback(OutboundOperationResult { false, std::nullopt, error });
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
        completePendingConfirmOrder(OutboundOperationResult { false, std::nullopt, error });
    }

    // ===== findById 完成 =====

    void completePendingFindById(const OutboundOperationResult& result)
    {
        if (!pendingFindByIdCallback_) {
            return;
        }
        if (pendingFindByIdOwner_.isNull()) {
            resetPendingFindById();
            return;
        }
        auto callback = std::move(pendingFindByIdCallback_);
        resetPendingFindById();
        callback(result);
    }

    void completePendingFindByIdSuccess()
    {
        if (!pendingFindByIdCallback_) {
            return;
        }
        if (pendingFindByIdOwner_.isNull()) {
            resetPendingFindById();
            return;
        }
        if (nextFindError.has_value()) {
            const auto error = nextFindError;
            nextFindError.reset();
            auto callback = std::move(pendingFindByIdCallback_);
            resetPendingFindById();
            callback(OutboundOperationResult { false, std::nullopt, error });
            return;
        }
        const quint32 id = pendingFindByIdId_;
        auto callback = std::move(pendingFindByIdCallback_);
        resetPendingFindById();
        callback(performFindById(id));
    }

    void completePendingFindByIdError(const AppError& error)
    {
        completePendingFindById(OutboundOperationResult { false, std::nullopt, error });
    }

    // ===== findByOrderNo 完成 =====

    void completePendingFindByOrderNo(const OutboundOperationResult& result)
    {
        if (!pendingFindByOrderNoCallback_) {
            return;
        }
        if (pendingFindByOrderNoOwner_.isNull()) {
            resetPendingFindByOrderNo();
            return;
        }
        auto callback = std::move(pendingFindByOrderNoCallback_);
        resetPendingFindByOrderNo();
        callback(result);
    }

    void completePendingFindByOrderNoSuccess()
    {
        if (!pendingFindByOrderNoCallback_) {
            return;
        }
        if (pendingFindByOrderNoOwner_.isNull()) {
            resetPendingFindByOrderNo();
            return;
        }
        if (nextFindError.has_value()) {
            const auto error = nextFindError;
            nextFindError.reset();
            auto callback = std::move(pendingFindByOrderNoCallback_);
            resetPendingFindByOrderNo();
            callback(OutboundOperationResult { false, std::nullopt, error });
            return;
        }
        const QString orderNo = pendingFindByOrderNoOrderNo_;
        auto callback = std::move(pendingFindByOrderNoCallback_);
        resetPendingFindByOrderNo();
        callback(performFindByOrderNo(orderNo));
    }

    void completePendingFindByOrderNoError(const AppError& error)
    {
        completePendingFindByOrderNo(OutboundOperationResult { false, std::nullopt, error });
    }

    // ===== 辅助 =====

    void addOrder(const OutboundOrder& order)
    {
        orders.push_back(order);
        nextId_ = std::max(nextId_, order.id + 1);
        for (const auto& line : order.lines) {
            nextLineId_ = std::max(nextLineId_, line.id + 1);
        }
    }

    void clear()
    {
        orders.clear();
        stockBalances.clear();
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
        deferFindById = false;
        deferFindByOrderNo = false;

        pendingListOps_.clear();
        resetPendingCreate();
        resetPendingConfirm();
        resetPendingFindById();
        resetPendingFindByOrderNo();
        nextId_ = 1;
        nextLineId_ = 1;
    }

    // ===== IOutboundRepository 实现 =====

    void listOrders(
        const OutboundOrderFilter& filter,
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
            callback(OutboundPageResult { false, {}, error });
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

        if (deferFindById) {
            pendingFindByIdCallback_ = std::move(callback);
            pendingFindByIdId_ = id;
            pendingFindByIdOwner_ = ownerPtr;
            return;
        }

        if (nextFindError.has_value()) {
            const auto error = nextFindError;
            nextFindError.reset();
            callback(OutboundOperationResult { false, std::nullopt, error });
            return;
        }

        callback(performFindById(id));
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

        if (deferFindByOrderNo) {
            pendingFindByOrderNoCallback_ = std::move(callback);
            pendingFindByOrderNoOrderNo_ = orderNo;
            pendingFindByOrderNoOwner_ = ownerPtr;
            return;
        }

        if (nextFindError.has_value()) {
            const auto error = nextFindError;
            nextFindError.reset();
            callback(OutboundOperationResult { false, std::nullopt, error });
            return;
        }

        callback(performFindByOrderNo(orderNo));
    }

    void createDraft(
        const OutboundOrder& order,
        const AuditContext& /* auditContext */,
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
            callback(OutboundOperationResult { false, std::nullopt, error });
            return;
        }

        callback(performCreateDraft(order));
    }

    void confirmOrder(
        quint32 id,
        const AuditContext& auditContext,
        QObject* owner,
        OperateCallback callback) override
    {
        QPointer<QObject> ownerPtr(owner);
        if (ownerPtr.isNull() || !callback) {
            return;
        }

        lastConfirmOrderId = id;
        lastConfirmOperatorId = auditContext.operatorId;

        if (deferConfirmOrder) {
            pendingConfirmCallback_ = std::move(callback);
            pendingConfirmId_ = id;
            pendingConfirmOperatorId_ = auditContext.operatorId;
            pendingConfirmOwner_ = ownerPtr;
            return;
        }

        if (nextConfirmError.has_value()) {
            const auto error = nextConfirmError;
            nextConfirmError.reset();
            callback(OutboundOperationResult { false, std::nullopt, error });
            return;
        }

        callback(performConfirm(id, auditContext.operatorId));
    }

private:
    struct PendingListOp {
        PageCallback callback;
        OutboundOrderFilter filter;
        PageRequest pageRequest;
        QPointer<QObject> owner;
    };

    void completePendingListOrdersAt(int index, const OutboundPageResult& result)
    {
        if (index < 0 || index >= pendingListOps_.size()) {
            return;
        }
        PendingListOp op = std::move(pendingListOps_[index]);
        pendingListOps_.removeAt(index);
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

    OutboundPageResult buildListOrdersSuccessResult(
        const OutboundOrderFilter& filter,
        const PageRequest& pageRequest) const
    {
        QVector<OutboundOrderListItemDto> filtered;
        for (const auto& order : orders) {
            if (!matchesFilter(order, filter)) {
                continue;
            }
            filtered.push_back(toListItem(order));
        }

        PageResult<OutboundOrderListItemDto> page;
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

        return OutboundPageResult { true, page, std::nullopt };
    }

    static bool matchesFilter(const OutboundOrder& order, const OutboundOrderFilter& filter)
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
                && !order.recipient.contains(keyword, Qt::CaseInsensitive)
                && !order.remark.contains(keyword, Qt::CaseInsensitive)) {
                return false;
            }
        }

        return true;
    }

    static OutboundOrderListItemDto toListItem(const OutboundOrder& order)
    {
        OutboundOrderListItemDto dto;
        dto.id = order.id;
        dto.orderNo = order.orderNo;
        dto.recipient = order.recipient;
        dto.status = order.status;
        dto.operatorId = order.operatorId;
        dto.operatorName = QString();
        dto.warehouseId = order.warehouseId;
        dto.warehouseName = QString();
        dto.lineCount = order.lines.size();
        dto.totalQuantity = 0;
        for (const auto& line : order.lines) {
            dto.totalQuantity += line.quantity;
        }
        dto.createdAt = order.createdAt;
        dto.updatedAt = order.updatedAt;
        dto.confirmedAt = order.confirmedAt;
        return dto;
    }

    QString generateOrderNo() const
    {
        const auto now = QDateTime::currentDateTime();
        const QString todayPrefix = QStringLiteral("OUT-%1-").arg(
            now.toString(QStringLiteral("yyyyMMdd")));

        int maxSeq = 0;
        for (const auto& order : orders) {
            if (order.orderNo.startsWith(todayPrefix)) {
                const int lastDash = order.orderNo.lastIndexOf('-');
                if (lastDash >= 0) {
                    bool ok = false;
                    const int seq = order.orderNo.mid(lastDash + 1).toInt(&ok);
                    if (ok && seq > maxSeq) {
                        maxSeq = seq;
                    }
                }
            }
        }

        return QStringLiteral("%1%2").arg(todayPrefix).arg(maxSeq + 1, 6, 10, QChar('0'));
    }

    OutboundOperationResult performCreateDraft(const OutboundOrder& order)
    {
        if (!order.orderNo.trimmed().isEmpty()) {
            const auto duplicated = std::find_if(orders.cbegin(), orders.cend(),
                [&order](const OutboundOrder& existing) {
                    return existing.orderNo == order.orderNo;
                });
            if (duplicated != orders.cend()) {
                return OutboundOperationResult {
                    false,
                    std::nullopt,
                    AppError {
                        AppErrorCategory::Database,
                        AppErrorCode::DuplicateOutboundOrder,
                        QStringLiteral("出库订单号已存在") }
                };
            }
        }

        OutboundOrder created = order;
        if (created.id == 0) {
            created.id = nextId_++;
        } else {
            nextId_ = std::max(nextId_, created.id + 1);
        }
        if (created.orderNo.trimmed().isEmpty()) {
            created.orderNo = generateOrderNo();
        }
        created.status = OutboundOrderStatus::Draft;
        const auto now = QDateTime::currentDateTime();
        created.createdAt = now;
        created.updatedAt = now;
        for (auto& line : created.lines) {
            if (line.id == 0) {
                line.id = nextLineId_++;
            } else {
                nextLineId_ = std::max(nextLineId_, line.id + 1);
            }
            line.orderId = created.id;
        }

        orders.push_back(created);
        return OutboundOperationResult { true, created, std::nullopt };
    }

    // confirmOrder 的实际迁移逻辑:按 id 查找、校验状态、Draft -> Confirmed。
    // 与入库不同,出库确认还需模拟库存扣减:
    //   - 条件扣减:仅当库存 >= 出库数量时成功
    //   - 按行使用运行快照校验+扣减(模拟条件更新),任一明细不足 → 整个事务回滚
    //     (不落任何库存/状态),同一产品多行明细合计超过库存也不会变成负数
    //   - a)无库存记录 / b)库存不足 → 均映射为 InsufficientStock(仅消息区分)
    //   - c)订单状态不匹配 → InvalidOutboundOrder
    OutboundOperationResult performConfirm(quint32 id, quint32 operatorId)
    {
        auto found = std::find_if(orders.begin(), orders.end(),
            [id](const OutboundOrder& order) {
                return order.id == id;
            });

        if (found == orders.end()) {
            return OutboundOperationResult {
                false,
                std::nullopt,
                AppError {
                    AppErrorCategory::Database,
                    AppErrorCode::OutboundOrderNotFound,
                    QStringLiteral("出库订单不存在") }
            };
        }

        if (found->status != OutboundOrderStatus::Draft) {
            return OutboundOperationResult {
                false,
                std::nullopt,
                AppError {
                    AppErrorCategory::Database,
                    AppErrorCode::InvalidOutboundOrder,
                    QStringLiteral("订单状态不允许确认") }
            };
        }

        // 库存扣减:按行模拟条件更新(UPDATE ... WHERE quantity >= :qty)
        // 使用运行快照逐行校验+扣减,任一明细不足则整体回滚(不落任何库存/状态),
        // 同一产品多行明细时也不会让库存变成负数
        QHash<QString, int> remaining = stockBalances; // 运行中的可用库存快照
        for (const auto& line : found->lines) {
            const QString key = stockKey(line.productId, found->warehouseId);
            const int avail = remaining.value(key, 0);
            if (!stockBalances.contains(key)) {
                // 含义 a):该产品在该仓库没有库存余额记录(等同库存为0,不能出库)
                return OutboundOperationResult {
                    false,
                    std::nullopt,
                    AppError {
                        AppErrorCategory::Validation,
                        AppErrorCode::InsufficientStock,
                        QStringLiteral("产品(%1)在仓库(%2)中没有库存记录,无法出库")
                            .arg(line.productId)
                            .arg(found->warehouseId) }
                };
            }
            if (avail < line.quantity) {
                // 含义 b):库存不足
                return OutboundOperationResult {
                    false,
                    std::nullopt,
                    AppError {
                        AppErrorCategory::Validation,
                        AppErrorCode::InsufficientStock,
                        QStringLiteral("产品(%1)库存不足:需要%2,可用%3")
                            .arg(line.productId)
                            .arg(line.quantity)
                            .arg(avail) }
                };
            }
            remaining[key] = avail - line.quantity; // 模拟该行条件更新成功后的余额
        }

        // 全部明细可出,执行扣减
        for (const auto& line : found->lines) {
            const QString key = stockKey(line.productId, found->warehouseId);
            stockBalances[key] = stockBalances.value(key, 0) - line.quantity;
        }

        found->status = OutboundOrderStatus::Confirmed;
        const auto now = QDateTime::currentDateTime();
        found->confirmedAt = now;
        found->updatedAt = now;
        found->operatorId = operatorId;

        return OutboundOperationResult { true, *found, std::nullopt };
    }

    OutboundOperationResult performFindById(quint32 id) const
    {
        const auto found = std::find_if(orders.cbegin(), orders.cend(),
            [id](const OutboundOrder& order) {
                return order.id == id;
            });

        if (found == orders.cend()) {
            return OutboundOperationResult { true, std::nullopt, std::nullopt };
        }

        return OutboundOperationResult { true, *found, std::nullopt };
    }

    OutboundOperationResult performFindByOrderNo(const QString& orderNo) const
    {
        const auto normalizedOrderNo = orderNo.trimmed();
        const auto found = std::find_if(orders.cbegin(), orders.cend(),
            [&normalizedOrderNo](const OutboundOrder& order) {
                return order.orderNo == normalizedOrderNo;
            });

        if (found == orders.cend()) {
            return OutboundOperationResult { true, std::nullopt, std::nullopt };
        }

        return OutboundOperationResult { true, *found, std::nullopt };
    }

    // 挂起状态
    QVector<PendingListOp> pendingListOps_;

    OperateCallback pendingCreateCallback_;
    std::optional<OutboundOrder> pendingCreateOrder_;
    QPointer<QObject> pendingCreateOwner_;

    OperateCallback pendingConfirmCallback_;
    quint32 pendingConfirmId_ { 0 };
    quint32 pendingConfirmOperatorId_ { 0 };
    QPointer<QObject> pendingConfirmOwner_;

    OperateCallback pendingFindByIdCallback_;
    quint32 pendingFindByIdId_ { 0 };
    QPointer<QObject> pendingFindByIdOwner_;

    OperateCallback pendingFindByOrderNoCallback_;
    QString pendingFindByOrderNoOrderNo_;
    QPointer<QObject> pendingFindByOrderNoOwner_;

    void resetPendingFindById() noexcept
    {
        pendingFindByIdCallback_ = nullptr;
        pendingFindByIdId_ = 0;
        pendingFindByIdOwner_.clear();
    }

    void resetPendingFindByOrderNo() noexcept
    {
        pendingFindByOrderNoCallback_ = nullptr;
        pendingFindByOrderNoOrderNo_.clear();
        pendingFindByOrderNoOwner_.clear();
    }

    quint32 nextId_ { 1 };
    quint32 nextLineId_ { 1 };
};