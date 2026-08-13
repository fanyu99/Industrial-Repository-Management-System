#pragma once
#include "AuditContext.h"
#include "IOutboundRepository.h"
#include "OutboundRequests.h"
#include "Permission.h"
#include "SessionManager.h"
#include <QObject>
#include <QString>
// 创建出库订单请求转换为出库订单结果
struct RequestToOutboundOrderResult {
    std::optional<OutboundOrder> order;
    std::optional<AppError> error;
};
// 出库服务
class OutboundService : public QObject {
    Q_OBJECT
public:
    explicit OutboundService(
        IOutboundRepository& repository,
        SessionManager& sessionManager,
        QObject* parent = nullptr);
    ~OutboundService() = default;
    using OperateCallback = IOutboundRepository::OperateCallback;
    using PageCallback = IOutboundRepository::PageCallback;
    // 获取当前的用户
    std::optional<AuthenticatedUser> currentUser() const noexcept;
    // 构建审计上下文
    [[nodiscard]] AuditContext buildAuditContext() const noexcept;
    // 检查是否有指定权限
    [[nodiscard]] bool hasPermission(Permission permission) const noexcept;
    // 校验是否有权限
    [[nodiscard]] std::optional<AppError> authorize(Permission permission) const noexcept;
    // 校验订单是否合法(用于创建订单之后)
    [[nodiscard]] static std::optional<AppError> validateCreateOutboundOrder(const OutboundOrder& order) noexcept;
    // 校验确认订单是否合法(用于确认订单之后)
    [[nodiscard]] static std::optional<AppError> validateConfirmOutboundOrder(const OutboundOrder& order) noexcept;
    // 校验分页请求是否合法
    [[nodiscard]] std::optional<AppError> validatePageRequest(const PageRequest& request) const noexcept;
    // 校验创建出库订单请求是否合法
    [[nodiscard]] std::optional<AppError> validateCreateRequest(
        const CreateOutboundOrderRequest& request) const noexcept;
    // 校验订单过滤器
    [[nodiscard]] std::optional<AppError> validateFilter(const OutboundOrderFilter& filter) const noexcept;
    // 列出出库订单
    void listOrders(
        const OutboundOrderFilter& filter,
        const PageRequest& request,
        QObject* owner,
        PageCallback callback);
    // 创建草稿订单
    void createDraft(
        const CreateOutboundOrderRequest& request,
        QObject* owner,
        OperateCallback callback);
    // 确认订单
    void confirmOrder(
        quint32 id,
        QObject* owner,
        OperateCallback callback);

private:
    RequestToOutboundOrderResult requestToOrder(const CreateOutboundOrderRequest& request) const noexcept;
    IOutboundRepository& repository_;
    SessionManager& sessionManager_;
};