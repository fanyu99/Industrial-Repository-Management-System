#include "OutboundService.h"
#include "IOutboundRepository.h"
#include <QPointer>
#include <optional>
#include <utility>
OutboundService::OutboundService(IOutboundRepository& repository, SessionManager& sessionManager,
    QObject* parent)
    : repository_(repository)
    , sessionManager_(sessionManager)
    , QObject(parent)
{
}
AuditContext OutboundService::buildAuditContext() const noexcept
{
    const auto user = sessionManager_.currentUser();
    if (!user.has_value()) {
        return {};
    }
    return { user->userName, user->id };
}

// 获取当前的用户
std::optional<AuthenticatedUser> OutboundService::currentUser() const noexcept
{
    return sessionManager_.currentUser();
}
// 校验权限
bool OutboundService::hasPermission(
    Permission permission) const noexcept
{
    return sessionManager_.hasPermission(permission);
}
// 校验是否有指定权限
std::optional<AppError> OutboundService::authorize(Permission permission) const noexcept
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
// 校验创建后的订单是否合法(用于创建订单之后)
std::optional<AppError> OutboundService::validateCreateOutboundOrder(const OutboundOrder& order) noexcept
{
    if (order.id == 0) {
        return AppError {
            AppErrorCategory::Validation,
            AppErrorCode::InvalidOutboundOrder,
            QStringLiteral("订单ID无效")
        };
    }
    if (order.status != OutboundOrderStatus::Draft) {
        return AppError {
            AppErrorCategory::Validation,
            AppErrorCode::InvalidOutboundOrder,
            QStringLiteral("订单非草稿状态")
        };
    }
    if (order.orderNo.trimmed().isEmpty()) {
        return AppError {
            AppErrorCategory::Validation,
            AppErrorCode::InvalidOutboundOrder,
            QStringLiteral("订单号不能为空")
        };
    }
    if (order.recipient.trimmed().isEmpty()) {
        return AppError {
            AppErrorCategory::Validation,
            AppErrorCode::InvalidOutboundOrder,
            QStringLiteral("接收人不能为空")
        };
    }
    if (order.operatorId == 0) {
        return AppError {
            AppErrorCategory::Validation,
            AppErrorCode::InvalidOutboundOrder,
            QStringLiteral("操作员ID无效")
        };
    }
    if (order.warehouseId == 0) {
        return AppError {
            AppErrorCategory::Validation,
            AppErrorCode::InvalidOutboundOrder,
            QStringLiteral("仓库ID无效")
        };
    }
    if (order.lines.isEmpty()) {
        return AppError {
            AppErrorCategory::Validation,
            AppErrorCode::InvalidOutboundOrder,
            QStringLiteral("订单不能为空")
        };
    }
    for (const auto& line : order.lines) {
        if (line.quantity <= 0 || line.unitPrice < 0 || line.productId == 0) {
            return AppError {
                AppErrorCategory::Validation,
                AppErrorCode::InvalidOutboundOrder,
                QStringLiteral("订单行数据不合法")
            };
        }
        if (line.orderId != order.id) {
            return AppError {
                AppErrorCategory::Validation,
                AppErrorCode::InvalidOutboundOrder,
                QStringLiteral("订单行订单ID与订单ID不一致")
            };
        }
    }
    return std::nullopt;
}
// 校验创建出库订单请求是否合法
std::optional<AppError> OutboundService::validateCreateRequest(const CreateOutboundOrderRequest& request) const noexcept
{
    if (request.recipient.trimmed().isEmpty()) {
        return AppError {
            AppErrorCategory::Validation,
            AppErrorCode::InvalidOutboundOrder,
            QStringLiteral("接收人不能为空")
        };
    }
    if (request.warehouseId == 0) {
        return AppError {
            AppErrorCategory::Validation,
            AppErrorCode::InvalidOutboundOrder,
            QStringLiteral("仓库ID无效")
        };
    }
    if (request.lines.isEmpty()) {
        return AppError {
            AppErrorCategory::Validation,
            AppErrorCode::InvalidOutboundOrder,
            QStringLiteral("订单不能为空")
        };
    }
    for (const auto& line : request.lines) {
        if (line.quantity <= 0 || line.unitPrice < 0 || line.productId == 0) {
            return AppError {
                AppErrorCategory::Validation,
                AppErrorCode::InvalidOutboundOrder,
                QStringLiteral("订单(%1号产品)数据不合法").arg(line.productId)
            };
        }
    }
    return std::nullopt;
}
// 校验订单过滤器
std::optional<AppError> OutboundService::validateFilter(const OutboundOrderFilter& filter) const noexcept
{
    if (filter.warehouseId.has_value() && filter.warehouseId.value() == 0) {
        return AppError {
            AppErrorCategory::Validation,
            AppErrorCode::InvalidOutboundOrder,
            QStringLiteral("仓库ID无效")
        };
    }
    return std::nullopt;
}
// 校验分页请求是否合法
std::optional<AppError> OutboundService::validatePageRequest(const PageRequest& request) const noexcept
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
// 列出出库订单
void OutboundService::listOrders(
    const OutboundOrderFilter& filter,
    const PageRequest& request,
    QObject* owner,
    PageCallback callback)
{
    // 校验参数
    QPointer<QObject> ownerPtr(owner);
    if (ownerPtr.isNull() || !callback) {
        return;
    }
    // 校验权限
    if (auto error1 = authorize(Permission::ViewOutboundOrders); error1.has_value()) {
        callback(
            OutboundPageResult {
                false,
                PageResult<OutboundOrderListItemDto> {},
                error1 });
        return;
    }
    // 校验分页请求
    if (auto error2 = validatePageRequest(request); error2.has_value()) {
        callback(OutboundPageResult {
            false,
            PageResult<OutboundOrderListItemDto> {},
            error2 });
        return;
    }
    // 校验过滤器
    if (auto error3 = validateFilter(filter); error3.has_value()) {
        callback(OutboundPageResult {
            false,
            PageResult<OutboundOrderListItemDto> {},
            error3 });
        return;
    }
    // 查找并回调
    repository_.listOrders(filter, request, ownerPtr.data(), [ownerPtr, callback = std::move(callback)](const OutboundPageResult& result) {
        if (ownerPtr.isNull() || !callback)
            return;
        if (!result.success) {
            callback(OutboundPageResult {
                false,
                PageResult<OutboundOrderListItemDto> {},
                result.error.has_value() ? result.error : AppError { AppErrorCategory::Database, AppErrorCode::RepositoryFailure, QStringLiteral("订单查找时出现未知错误") } });
            return;
        }
        if (result.error.has_value()) {
            callback(OutboundPageResult { false, PageResult<OutboundOrderListItemDto> {}, result.error });
            return;
        }
        // 找到订单
        callback(result);
    });
}
// 由订单请求转换为出库订单
RequestToOutboundOrderResult OutboundService::requestToOrder(const CreateOutboundOrderRequest& request) const noexcept
{
    if (auto error = validateCreateRequest(request); error.has_value()) {
        return { std::nullopt, error };
    }
    // 获取用户
    const auto user = currentUser();
    if (!user.has_value()) {
        return { std::nullopt, AppError { AppErrorCategory::Auth, AppErrorCode::NotAuthenticated, QStringLiteral("当前用户未认证") } };
    }
    if (user->id == 0) {
        return { std::nullopt, AppError { AppErrorCategory::Auth, AppErrorCode::NotAuthenticated, QStringLiteral("当前用户无效") } };
    }
    OutboundOrder order;
    order.id = 0;
    order.orderNo.clear();
    order.warehouseId = request.warehouseId;
    order.recipient = request.recipient;
    order.status = OutboundOrderStatus::Draft;
    order.operatorId = user->id;
    order.remark = request.remark;
    for (const auto& line : request.lines) {
        OutboundOrderLine outboundLine;
        outboundLine.id = 0;
        outboundLine.orderId = 0;
        outboundLine.productId = line.productId;
        outboundLine.quantity = line.quantity;
        outboundLine.unitPrice = line.unitPrice;
        order.lines.push_back(outboundLine);
    }
    return { std::make_optional(order), std::nullopt };
}
// 创建草稿订单
void OutboundService::createDraft(
    const CreateOutboundOrderRequest& request,
    QObject* owner,
    OperateCallback callback)
{
    // 确认参数
    QPointer<QObject> ownerPtr(owner);
    if (ownerPtr.isNull() || !callback) {
        return;
    }
    // 校验权限
    if (auto error1 = authorize(Permission::CreateOutboundOrders); error1.has_value()) {
        callback(OutboundOperationResult {
            false,
            std::nullopt,
            error1 });
        return;
    }
    // 组装订单
    const auto requestToOrderResult = requestToOrder(request);
    if (requestToOrderResult.error.has_value()) {
        callback(OutboundOperationResult {
            false,
            std::nullopt,
            requestToOrderResult.error });
        return;
    }
    if (!requestToOrderResult.order.has_value()) {
        callback(OutboundOperationResult {
            false,
            std::nullopt,
            AppError {
                AppErrorCategory::Validation,
                AppErrorCode::InvalidOutboundOrder,
                QStringLiteral("订单组装未返回有效数据") } });
        return;
    }
    // 创建草稿订单(是否存在由repository判断)
    const OutboundOrder order = requestToOrderResult.order.value();
    const auto auditCtx = buildAuditContext();
    repository_.createDraft(order, auditCtx, ownerPtr.data(), [ownerPtr, callback](const OutboundOperationResult& result) {
        if (ownerPtr.isNull() || !callback)
            return;
        if (!result.success) {
            callback(OutboundOperationResult {
                false,
                std::nullopt,
                result.error.has_value() ? result.error : AppError { AppErrorCategory::Database, AppErrorCode::RepositoryFailure, QStringLiteral("订单创建时出现未知错误") } });
            return;
        }
        if (result.error.has_value()) {
            callback(OutboundOperationResult {
                false,
                std::nullopt,
                result.error });
            return;
        }
        if (!result.order.has_value()) {
            callback(OutboundOperationResult {
                false,
                std::nullopt,
                AppError {
                    AppErrorCategory::Validation,
                    AppErrorCode::InvalidOutboundOrder,
                    QStringLiteral("订单创建后未返回有效数据") } });
            return;
        }
        // 对创建好的订单进行确认
        const OutboundOrder createdOrder = result.order.value();
        if (auto error = validateCreateOutboundOrder(createdOrder); error.has_value()) {
            callback(OutboundOperationResult {
                false,
                std::nullopt,
                error });
            return;
        }
        callback(OutboundOperationResult {
            true,
            createdOrder,
            std::nullopt });
    });
}
// 校验确认订单是否合法(用于确认订单之后)
std::optional<AppError> OutboundService::validateConfirmOutboundOrder(const OutboundOrder& order) noexcept
{
    if (order.id == 0) {
        return AppError {
            AppErrorCategory::Validation,
            AppErrorCode::InvalidOutboundOrder,
            QStringLiteral("订单ID无效")
        };
    }
    if (order.orderNo.trimmed().isEmpty()) {
        return AppError {
            AppErrorCategory::Validation,
            AppErrorCode::InvalidOutboundOrder,
            QStringLiteral("订单号无效")
        };
    }
    if (order.warehouseId == 0) {
        return AppError {
            AppErrorCategory::Validation,
            AppErrorCode::InvalidOutboundOrder,
            QStringLiteral("仓库ID无效")
        };
    }
    if (order.operatorId == 0) {
        return AppError {
            AppErrorCategory::Validation,
            AppErrorCode::InvalidOutboundOrder,
            QStringLiteral("操作人ID无效")
        };
    }
    if (order.recipient.trimmed().isEmpty()) {
        return AppError {
            AppErrorCategory::Validation,
            AppErrorCode::InvalidOutboundOrder,
            QStringLiteral("接收人无效")
        };
    }
    if (order.status != OutboundOrderStatus::Confirmed) {
        return AppError {
            AppErrorCategory::Validation,
            AppErrorCode::InvalidOutboundOrder,
            QStringLiteral("订单状态无效")
        };
    }
    if (!order.confirmedAt.has_value()) {
        return AppError {
            AppErrorCategory::Validation,
            AppErrorCode::InvalidOutboundOrder,
            QStringLiteral("确认时间无效")
        };
    }
    if (order.lines.isEmpty()) {
        return AppError {
            AppErrorCategory::Validation,
            AppErrorCode::InvalidOutboundOrder,
            QStringLiteral("订单不能为空")
        };
    }
    for (const auto& line : order.lines) {
        if (line.quantity <= 0 || line.unitPrice < 0 || line.productId == 0) {
            return AppError {
                AppErrorCategory::Validation,
                AppErrorCode::InvalidOutboundOrder,
                QStringLiteral("订单行数据不合法")
            };
        }
        if (line.orderId != order.id) {
            return AppError {
                AppErrorCategory::Validation,
                AppErrorCode::InvalidOutboundOrder,
                QStringLiteral("订单行订单ID与订单ID不一致")
            };
        }
    }
    return std::nullopt;
}
// 确认订单
void OutboundService::confirmOrder(
    quint32 id,
    QObject* owner,
    OperateCallback callback)
{
    // 校验参数
    QPointer<QObject> ownerPtr(owner);
    if (ownerPtr.isNull() || !callback) {
        return;
    }
    // 校验权限
    if (auto error1 = authorize(Permission::ConfirmOutboundOrders); error1.has_value()) {
        callback(OutboundOperationResult {
            false,
            std::nullopt,
            error1 });
        return;
    }
    if (id == 0) {
        callback(OutboundOperationResult {
            false,
            std::nullopt,
            AppError {
                AppErrorCategory::Validation,
                AppErrorCode::InvalidInput,
                QStringLiteral("订单ID无效") } });
        return;
    }
    const auto auditCtx = buildAuditContext();
    if (auditCtx.operatorId == 0) {
        callback(OutboundOperationResult {
            false,
            std::nullopt,
            AppError {
                AppErrorCategory::Auth,
                AppErrorCode::NotAuthenticated,
                QStringLiteral("当前用户未认证") } });
        return;
    }
    // 直接确认(取消查询时带来的竟态问题)
    repository_.confirmOrder(id, auditCtx, ownerPtr.data(), [ownerPtr, callback](const OutboundOperationResult& result) {
        if (ownerPtr.isNull() || !callback)
            return;
        if (!result.success) {
            callback(OutboundOperationResult {
                false,
                std::nullopt,
                result.error.has_value() ? result.error : AppError { AppErrorCategory::Database, AppErrorCode::RepositoryFailure, QStringLiteral("确认订单时出现未知错误") } });
            return;
        }
        if (result.error.has_value()) {
            callback(OutboundOperationResult {
                false,
                std::nullopt,
                result.error });
            return;
        }
        if (!result.order.has_value()) {
            callback(OutboundOperationResult {
                false,
                std::nullopt,
                AppError {
                    AppErrorCategory::Validation,
                    AppErrorCode::InvalidOutboundOrder,
                    QStringLiteral("确认订单后未返回有效数据") } });
            return;
        }
        // 校验确认后的订单是否合法
        if (auto error = validateConfirmOutboundOrder(result.order.value()); error.has_value()) {
            callback(OutboundOperationResult {
                false,
                std::nullopt,
                error });
            return;
        }
        callback(OutboundOperationResult {
            true,
            result.order.value(),
            std::nullopt });
    });
}
// 获取订单详情
void OutboundService::getOrderDetail(
    quint32 id,
    QObject* owner,
    DetailCallback callback)
{
    QPointer<QObject> ownerPtr(owner);
    if (ownerPtr.isNull() || !callback) {
        return;
    }
    if (auto error1 = authorize(Permission::ViewOutboundOrders); error1.has_value()) {
        callback(OutboundOrderDetailResult {
            false,
            std::nullopt,
            error1 });
        return;
    }
    if (id == 0) {
        callback(OutboundOrderDetailResult {
            false,
            std::nullopt,
            AppError {
                AppErrorCategory::Validation,
                AppErrorCode::InvalidInput,
                QStringLiteral("订单ID无效") } });
        return;
    }
    repository_.getOrderDetail(id, ownerPtr.data(), [ownerPtr, callback = std::move(callback)](const OutboundOrderDetailResult& result) {
        if (ownerPtr.isNull() || !callback)
            return;
        if (!result.success) {
            callback(OutboundOrderDetailResult {
                false,
                std::nullopt,
                result.error.has_value() ? result.error : AppError { AppErrorCategory::Database, AppErrorCode::RepositoryFailure, QStringLiteral("获取订单详情时出现未知错误") } });
            return;
        }
        if (result.error.has_value()) {
            callback(OutboundOrderDetailResult {
                false,
                std::nullopt,
                result.error });
            return;
        }
        if (!result.orderDetail.has_value()) {
            callback(OutboundOrderDetailResult {
                false,
                std::nullopt,
                AppError {
                    AppErrorCategory::Validation,
                    AppErrorCode::InvalidOutboundOrder,
                    QStringLiteral("获取订单详情后未返回有效数据") } });
            return;
        }
        const auto& orderDetail = result.orderDetail.value();
        if (orderDetail.id == 0 || orderDetail.operatorId == 0 || orderDetail.orderNo.trimmed().isEmpty() || orderDetail.recipient.trimmed().isEmpty() || orderDetail.warehouseId == 0 || orderDetail.operatorName.trimmed().isEmpty() || orderDetail.warehouseName.trimmed().isEmpty() || orderDetail.lineCount == 0 || orderDetail.totalQuantity == 0 || orderDetail.totalAmount < 0.0 || orderDetail.detailLines.isEmpty() || !orderDetail.createdAt.isValid() || !orderDetail.updatedAt.isValid() || (orderDetail.confirmedAt.has_value() && !orderDetail.confirmedAt->isValid())) {
            callback(OutboundOrderDetailResult {
                false,
                std::nullopt,
                AppError {
                    AppErrorCategory::Validation,
                    AppErrorCode::InvalidOutboundOrder,
                    QStringLiteral("订单详情数据无效") } });
            return;
        }
        if ((orderDetail.status == OutboundOrderStatus::Confirmed && !orderDetail.confirmedAt.has_value()) || (orderDetail.status == OutboundOrderStatus::Draft && orderDetail.confirmedAt.has_value())) {
            callback(OutboundOrderDetailResult {
                false,
                std::nullopt,
                AppError {
                    AppErrorCategory::Validation,
                    AppErrorCode::InvalidOutboundOrder,
                    QStringLiteral("订单状态无效") } });
            return;
        }
        int lineCount = 0;
        int totalQuantity = 0;
        double totalAmount = 0.0;
        for (const auto& line : orderDetail.detailLines) {
            ++lineCount;
            totalQuantity += line.quantity;
            totalAmount += line.subtotal;
            if (line.productId == 0 || line.productCode.trimmed().isEmpty() || line.productName.trimmed().isEmpty() || line.quantity <= 0 || line.unitPrice < 0.0 || line.subtotal < 0.0 || !qFuzzyCompare(line.subtotal + 1.0, line.unitPrice * line.quantity + 1.0)) {
                callback(OutboundOrderDetailResult {
                    false,
                    std::nullopt,
                    AppError {
                        AppErrorCategory::Validation,
                        AppErrorCode::InvalidOutboundOrder,
                        QStringLiteral("订单详情数据无效") } });
                return;
            }
        }
        if (lineCount != orderDetail.lineCount || totalQuantity != orderDetail.totalQuantity || !qFuzzyCompare(totalAmount + 1.0, orderDetail.totalAmount + 1.0)) {
            callback(OutboundOrderDetailResult {
                false,
                std::nullopt,
                AppError {
                    AppErrorCategory::Validation,
                    AppErrorCode::InvalidOutboundOrder,
                    QStringLiteral("订单详情汇总数据无效") } });
            return;
        }
        callback(result);
    });
}