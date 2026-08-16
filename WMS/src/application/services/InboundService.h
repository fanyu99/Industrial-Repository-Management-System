#pragma once
#include "AuditContext.h"
#include "IInboundRepository.h"
#include "InboundRequests.h"
#include "Permission.h"
#include "SessionManager.h"
#include <QObject>
#include <QString>
// 创建入库订单请求转换为入库订单结果
struct RequestToOrderResult {
    std::optional<InboundOrder> order;
    std::optional<AppError> error;
};
// 入库服务
class InboundService : public QObject {
    Q_OBJECT
public:
    explicit InboundService(
        IInboundRepository& repository,
        SessionManager& sessionManager,
        QObject* parent = nullptr);
    ~InboundService() = default;
    using OperateCallback = IInboundRepository::OperateCallback;
    using PageCallback = IInboundRepository::PageCallback;
    using DetailCallback = IInboundRepository::DetailCallback;
    // 获取当前的用户
    std::optional<AuthenticatedUser> currentUser() const noexcept;
    // 构建审计上下文
    [[nodiscard]] AuditContext buildAuditContext() const noexcept;
    // 检查是否有指定权限
    [[nodiscard]] bool hasPermission(Permission permission) const noexcept;
    // 校验是否有权限
    [[nodiscard]] std::optional<AppError> authorize(Permission permission) const noexcept;
    // 校验订单是否合法(用于创建订单之后)
    [[nodiscard]] static std::optional<AppError> validateCreateInboundOrder(const InboundOrder& order) noexcept;
    // 校验确认订单是否合法(用于确认订单之后)
    [[nodiscard]] static std::optional<AppError> validateConfirmInboundOrder(const InboundOrder& order) noexcept;
    // 校验分页请求是否合法
    [[nodiscard]] std::optional<AppError> validatePageRequest(const PageRequest& request) const noexcept;
    // 校验创建入库订单请求是否合法
    [[nodiscard]] std::optional<AppError> validateCreateRequest(
        const CreateInboundOrderRequest& request) const noexcept;
    // 校验订单过滤器
    [[nodiscard]] std::optional<AppError> validateFilter(const InboundOrderFilter& filter) const noexcept;
    // 列出入库订单
    void listOrders(
        const InboundOrderFilter& filter,
        const PageRequest& request,
        QObject* owner,
        PageCallback callback);
    // 创建草稿订单
    void createDraft(
        const CreateInboundOrderRequest& request,
        QObject* owner,
        OperateCallback callback);
    // 确认订单
    void confirmOrder(
        quint32 id,
        QObject* owner,
        OperateCallback callback);
    // 获取订单详情
    void getOrderDetail(
        quint32 id,
        QObject* owner,
        DetailCallback callback);
    

private:
    RequestToOrderResult requestToOrder(const CreateInboundOrderRequest& request) const noexcept;
    IInboundRepository& repository_;
    SessionManager& sessionManager_;
};