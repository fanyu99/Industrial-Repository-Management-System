#include "InboundService.h"
#include "IInboundRepository.h"
#include <QPointer>
#include <optional>
#include <utility>
InboundService::InboundService(IInboundRepository& repository, SessionManager& sessionManager,
    QObject* parent)
    : repository_(repository)
    , sessionManager_(sessionManager)
    , QObject(parent)
{
}
// 获取当前的用户
std::optional<AuthenticatedUser> InboundService::currentUser() const noexcept
{
    return sessionManager_.currentUser();
}
// 校验权限
bool InboundService::hasPermission(
    Permission permission) const noexcept
{
    return sessionManager_.hasPermission(permission);
}
// 校验是否有指定权限
std::optional<AppError> InboundService::authorize(Permission permission) const noexcept
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
std::optional<AppError> InboundService::validateCreateInboundOrder(const InboundOrder& order) noexcept
{
    if (order.id == 0) {
        return AppError {
            AppErrorCategory::Validation,
            AppErrorCode::InvalidInboundOrder,
            QStringLiteral("订单ID无效")
        };
    }
    if (order.status != InboundOrderStatus::Draft) {
        return AppError {
            AppErrorCategory::Validation,
            AppErrorCode::InvalidInboundOrder,
            QStringLiteral("订单非草稿状态")
        };
    }
    if (order.orderNo.trimmed().isEmpty()) {
        return AppError {
            AppErrorCategory::Validation,
            AppErrorCode::InvalidInboundOrder,
            QStringLiteral("订单号不能为空")
        };
    }
    if (order.supplier.trimmed().isEmpty()) {
        return AppError {
            AppErrorCategory::Validation,
            AppErrorCode::InvalidInboundOrder,
            QStringLiteral("供应商不能为空")
        };
    }
    if (order.operatorId == 0) {
        return AppError {
            AppErrorCategory::Validation,
            AppErrorCode::InvalidInboundOrder,
            QStringLiteral("操作员ID无效")
        };
    }
    if (order.warehouseId == 0) {
        return AppError {
            AppErrorCategory::Validation,
            AppErrorCode::InvalidInboundOrder,
            QStringLiteral("仓库ID无效")
        };
    }
    if (order.lines.isEmpty()) {
        return AppError {
            AppErrorCategory::Validation,
            AppErrorCode::InvalidInboundOrder,
            QStringLiteral("订单不能为空")
        };
    }
    for (const auto& line : order.lines) {
        if (line.quantity <= 0 || line.unitPrice < 0 || line.productId == 0) {
            return AppError {
                AppErrorCategory::Validation,
                AppErrorCode::InvalidInboundOrder,
                QStringLiteral("订单行数据不合法")
            };
        }
        if (line.orderId != order.id) {
            return AppError {
                AppErrorCategory::Validation,
                AppErrorCode::InvalidInboundOrder,
                QStringLiteral("订单行订单ID与订单ID不一致")
            };
        }
    }
    return std::nullopt;
}
// 校验创建入库订单请求是否合法
std::optional<AppError> InboundService::validateCreateRequest(const CreateInboundOrderRequest& request) const noexcept
{
    if (request.supplier.trimmed().isEmpty()) {
        return AppError {
            AppErrorCategory::Validation,
            AppErrorCode::InvalidInboundOrder,
            QStringLiteral("供应商不能为空")
        };
    }
    if (request.warehouseId == 0) {
        return AppError {
            AppErrorCategory::Validation,
            AppErrorCode::InvalidInboundOrder,
            QStringLiteral("仓库ID无效")
        };
    }
    if (request.lines.isEmpty()) {
        return AppError {
            AppErrorCategory::Validation,
            AppErrorCode::InvalidInboundOrder,
            QStringLiteral("订单不能为空")
        };
    }
    for (const auto& line : request.lines) {
        if (line.quantity <= 0 || line.unitPrice < 0 || line.productId == 0) {
            return AppError {
                AppErrorCategory::Validation,
                AppErrorCode::InvalidInboundOrder,
                QStringLiteral("订单(%1号产品)数据不合法").arg(line.productId)
            };
        }
    }
    return std::nullopt;
}
// 校验订单过滤器
std::optional<AppError> InboundService::validateFilter(const InboundOrderFilter& filter) const noexcept
{
    if (filter.warehouseId.has_value() && filter.warehouseId.value() == 0) {
        return AppError {
            AppErrorCategory::Validation,
            AppErrorCode::InvalidInboundOrder,
            QStringLiteral("仓库ID无效")
        };
    }
    return std::nullopt;
}
// 校验分页请求是否合法
std::optional<AppError> InboundService::validatePageRequest(const PageRequest& request) const noexcept
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
// 列出入库订单
void InboundService::listOrders(
    const InboundOrderFilter& filter,
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
    if (auto error1 = authorize(Permission::ViewInboundOrders); error1.has_value()) {
        callback(
            InboundPageResult {
                false,
                PageResult<InboundOrderListItemDto> {},
                error1 });
        return;
    }
    // 校验分页请求
    if (auto error2 = validatePageRequest(request); error2.has_value()) {
        callback(InboundPageResult {
            false,
            PageResult<InboundOrderListItemDto> {},
            error2 });
        return;
    }
    // 校验过滤器
    if (auto error3 = validateFilter(filter); error3.has_value()) {
        callback(InboundPageResult {
            false,
            PageResult<InboundOrderListItemDto> {},
            error3 });
        return;
    }
    // 查找并回调
    repository_.listOrders(filter, request, ownerPtr.data(), [ownerPtr, callback = std::move(callback)](const InboundPageResult& result) {
        if (ownerPtr.isNull() || !callback)
            return;
        if (!result.success) {
            callback(InboundPageResult {
                false,
                PageResult<InboundOrderListItemDto> {},
                result.error.has_value() ? result.error : AppError { AppErrorCategory::Database, AppErrorCode::RepositoryFailure, QStringLiteral("订单查找时出现未知错误") } });
            return;
        }
        if (result.error.has_value()) {
            callback(InboundPageResult { false, PageResult<InboundOrderListItemDto> {}, result.error });
            return;
        }
        // 找到订单
        callback(result);
    });
}
// 由订单请求转换为入库订单
RequestToOrderResult InboundService::requestToOrder(const CreateInboundOrderRequest& request) const noexcept
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
    InboundOrder order;
    order.id = 0;
    order.orderNo.clear();
    order.warehouseId = request.warehouseId;
    order.supplier = request.supplier;
    order.status = InboundOrderStatus::Draft;
    order.operatorId = user->id;
    order.remark = request.remark;
    for (const auto& line : request.lines) {
        InboundOrderLine Inboundline;
        Inboundline.id = 0;
        Inboundline.orderId = 0;
        Inboundline.productId = line.productId;
        Inboundline.quantity = line.quantity;
        Inboundline.unitPrice = line.unitPrice;
        order.lines.push_back(Inboundline);
    }
    return { std::make_optional(order), std::nullopt };
}
// 创建草稿订单
void InboundService::createDraft(
    const CreateInboundOrderRequest& request,
    QObject* owner,
    OperateCallback callback)
{
    // 确认参数
    QPointer<QObject> ownerPtr(owner);
    if (ownerPtr.isNull() || !callback) {
        return;
    }
    // 校验权限
    if (auto error1 = authorize(Permission::CreateInboundOrders); error1.has_value()) {
        callback(InboundOperationResult {
            false,
            std::nullopt,
            error1 });
        return;
    }
    // 组装订单
    const auto requestToOrderResult = requestToOrder(request);
    if (requestToOrderResult.error.has_value()) {
        callback(InboundOperationResult {
            false,
            std::nullopt,
            requestToOrderResult.error });
        return;
    }
    if (!requestToOrderResult.order.has_value()) {
        callback(InboundOperationResult {
            false,
            std::nullopt,
            AppError {
                AppErrorCategory::Validation,
                AppErrorCode::InvalidInboundOrder,
                QStringLiteral("订单组装未返回有效数据") } });
        return;
    }
    // 创建草稿订单(是否存在由repository判断)
    const InboundOrder order = requestToOrderResult.order.value();
    repository_.createDraft(order, ownerPtr.data(), [ownerPtr, callback](const InboundOperationResult& result) {
        if (ownerPtr.isNull() || !callback)
            return;
        if (!result.success) {
            callback(InboundOperationResult {
                false,
                std::nullopt,
                result.error.has_value() ? result.error : AppError { AppErrorCategory::Database, AppErrorCode::RepositoryFailure, QStringLiteral("订单创建时出现未知错误") } });
            return;
        }
        if (result.error.has_value()) {
            callback(InboundOperationResult {
                false,
                std::nullopt,
                result.error });
            return;
        }
        if (!result.order.has_value()) {
            callback(InboundOperationResult {
                false,
                std::nullopt,
                AppError {
                    AppErrorCategory::Validation,
                    AppErrorCode::InvalidInboundOrder,
                    QStringLiteral("订单创建后未返回有效数据") } });
            return;
        }
        // 对创建好的订单进行确认
        const InboundOrder createdOrder = result.order.value();
        if (auto error = validateCreateInboundOrder(createdOrder); error.has_value()) {
            callback(InboundOperationResult {
                false,
                std::nullopt,
                error });
            return;
        }
        callback(InboundOperationResult {
            true,
            createdOrder,
            std::nullopt });
    });
}
// 校验确认订单是否合法(用于确认订单之后)
std::optional<AppError> InboundService::validateConfirmInboundOrder(const InboundOrder& order) noexcept
{
    if (order.id == 0) {
        return AppError {
            AppErrorCategory::Validation,
            AppErrorCode::InvalidInboundOrder,
            QStringLiteral("订单ID无效")
        };
    }
    if (order.orderNo.trimmed().isEmpty()) {
        return AppError {
            AppErrorCategory::Validation,
            AppErrorCode::InvalidInboundOrder,
            QStringLiteral("订单号无效")
        };
    }
    if (order.warehouseId == 0) {
        return AppError {
            AppErrorCategory::Validation,
            AppErrorCode::InvalidInboundOrder,
            QStringLiteral("仓库ID无效")
        };
    }
    if (order.operatorId == 0) {
        return AppError {
            AppErrorCategory::Validation,
            AppErrorCode::InvalidInboundOrder,
            QStringLiteral("操作人ID无效")
        };
    }
    if (order.supplier.trimmed().isEmpty()) {
        return AppError {
            AppErrorCategory::Validation,
            AppErrorCode::InvalidInboundOrder,
            QStringLiteral("供应商无效")
        };
    }
    if (order.status != InboundOrderStatus::Confirmed) {
        return AppError {
            AppErrorCategory::Validation,
            AppErrorCode::InvalidInboundOrder,
            QStringLiteral("订单状态无效")
        };
    }
    if (!order.confirmedAt.has_value()) {
        return AppError {
            AppErrorCategory::Validation,
            AppErrorCode::InvalidInboundOrder,
            QStringLiteral("确认时间无效")
        };
    }
    if(order.lines.isEmpty())
    {
        return AppError {
            AppErrorCategory::Validation,
            AppErrorCode::InvalidInboundOrder,
            QStringLiteral("订单不能为空")
        };
    }
    for (const auto& line : order.lines) {
        if (line.quantity <= 0 || line.unitPrice < 0 || line.productId == 0) {
            return AppError {
                AppErrorCategory::Validation,
                AppErrorCode::InvalidInboundOrder,
                QStringLiteral("订单行数据不合法")
            };
        }
        if (line.orderId != order.id) {
            return AppError {
                AppErrorCategory::Validation,
                AppErrorCode::InvalidInboundOrder,
                QStringLiteral("订单行订单ID与订单ID不一致")
            };
        }
    }
    return std::nullopt;
}
// 确认订单
void InboundService::confirmOrder(
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
    if (auto error1 = authorize(Permission::ConfirmInboundOrders); error1.has_value()) {
        callback(InboundOperationResult {
            false,
            std::nullopt,
            error1 });
        return;
    }
    if (id == 0) {
        callback(InboundOperationResult {
            false,
            std::nullopt,
            AppError {
                AppErrorCategory::Validation,
                AppErrorCode::InvalidInput,
                QStringLiteral("订单ID无效") } });
        return;
    }
    const auto user = currentUser();
    if (!user.has_value() || user->id == 0) {
        callback(InboundOperationResult {
            false,
            std::nullopt,
            AppError {
                AppErrorCategory::Auth,
                AppErrorCode::NotAuthenticated,
                QStringLiteral("当前用户未认证") } });
        return;
    }
    const quint32 operatorId = user->id;
    // 直接确认(取消查询时带来的竟态问题)
    repository_.confirmOrder(id, operatorId, ownerPtr.data(), [ownerPtr, callback](const InboundOperationResult& result) {
        if (ownerPtr.isNull() || !callback)
            return;
        if (!result.success) {
            callback(InboundOperationResult {
                false,
                std::nullopt,
                result.error.has_value() ? result.error : AppError { AppErrorCategory::Database, AppErrorCode::RepositoryFailure, QStringLiteral("确认订单时出现未知错误") } });
            return;
        }
        if (result.error.has_value()) {
            callback(InboundOperationResult {
                false,
                std::nullopt,
                result.error });
            return;
        }
        if (!result.order.has_value()) {
            callback(InboundOperationResult {
                false,
                std::nullopt,
                AppError {
                    AppErrorCategory::Validation,
                    AppErrorCode::InvalidInboundOrder,
                    QStringLiteral("确认订单后未返回有效数据") } });
            return;
        }
        // 校验确认后的订单是否合法
        if (auto error = validateConfirmInboundOrder(result.order.value()); error.has_value()) {
            callback(InboundOperationResult {
                false,
                std::nullopt,
                error });
            return;
        }
        // 确认成功的回调
        callback(result);
    });
}