#pragma once
#include "AppError.h"
#include "AuditContext.h"
#include "InboundDto.h"
#include "ProductDto.h"
#include <QString>
#include <functional>
#include <optional>
class QObject;
// 入库操作结果
struct InboundOperationResult {
    bool success { false };
    std::optional<InboundOrder> order;
    std::optional<AppError> error;
};
// 入库分页结果
struct InboundPageResult {
    bool success { false };
    PageResult<InboundOrderListItemDto> page;
    std::optional<AppError> error;
};
// 订单详情查找结果
struct InboundOrderDetailResult {
    bool success { false };
    std::optional<InboundOrderDetailDto> orderDetail;
    std::optional<AppError> error;

};
// 入库接口
class IInboundRepository {
public:
    using OperateCallback = std::function<void(InboundOperationResult)>; // 入库操作回调
    using PageCallback = std::function<void(InboundPageResult)>; // 入库分页回调
    using DetailCallback = std::function<void(InboundOrderDetailResult)>; // 订单详情回调
    virtual ~IInboundRepository() = default;
    // 创建草稿订单(必须返回有效的订单OrderNo和订单ID)
    // 确保orderNo在INSERT前已经生成(Service层创建的orderNo允许为空,但是数据库层不允许为空!)
    virtual void createDraft(
        const InboundOrder& order,
        const AuditContext& auditContext,
        QObject* owner,
        OperateCallback callback)
        = 0;
    // 根据id查询订单
    virtual void findById(
        quint32 id,
        QObject* owner,
        OperateCallback callback)
        = 0;
    // 根据编号查询订单
    virtual void findByOrderNo(
        const QString& orderNo,
        QObject* owner,
        OperateCallback callback)
        = 0;
    // 分页查询订单
    virtual void listOrders(
        const InboundOrderFilter& filter,
        const PageRequest& request,
        QObject* owner,
        PageCallback callback)
        = 0;
    // 确认订单
    virtual void confirmOrder(
        quint32 id,
        const AuditContext& auditContext,
        QObject* owner,
        OperateCallback callback)
        = 0;
    // 获取订单详情
    virtual void getOrderDetail(
        quint32 id,
        QObject* owner,
        DetailCallback callback)
        = 0;
};