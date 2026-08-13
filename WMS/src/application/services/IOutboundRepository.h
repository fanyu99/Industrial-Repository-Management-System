#pragma once
#include "AppError.h"
#include "AuditContext.h"
#include "OutboundDto.h"
#include "ProductDto.h"
#include <QString>
#include <functional>
#include <optional>
class QObject;
// 出库操作结果
struct OutboundOperationResult {
    bool success { false };
    std::optional<OutboundOrder> order;
    std::optional<AppError> error;
};
// 出库分页结果
struct OutboundPageResult {
    bool success { false };
    PageResult<OutboundOrderListItemDto> page;
    std::optional<AppError> error;
};
// 出库接口
//
// 出库确认(confirmOrder)与入库确认最大的区别在于"库存扣减",业务规则约定如下:
//  1. 库存扣减必须使用条件更新:
//     UPDATE stock_balance SET quantity = quantity - :qty
//     WHERE product_id = :pid AND warehouse_id = :wid AND quantity >= :qty
//     并发扣减时靠 "quantity >= :qty" 守卫,防止库存变为负数。
//  2. affectedRows == 0 需要区分三种原因,不能一律视为数据库错误:
//     - 该产品在该仓库没有库存余额记录(产品不存在于库存)
//     - 库存不足(可用数量 < 出库明细数量)
//     - 订单状态不匹配(非草稿,被并发重复确认)
//     expectedAffectedRows 只作为事务守卫(结果不满足则回滚),
//     业务上的"库存不足"应映射为 InsufficientStock 业务错误,而非 DatabaseFailure。
//  3. 任一明细库存不足,整个出库事务回滚;MVP 不支持部分出库。
//  4. 出库明细数量不得超过该仓库可用库存。
//  5. 并发确认时应锁定订单状态行/库存行(如 SELECT ... FOR UPDATE),
//     避免两个请求同时读到草稿状态后各自扣减。
class IOutboundRepository {
public:
    using OperateCallback = std::function<void(OutboundOperationResult)>; // 出库操作回调
    using PageCallback = std::function<void(OutboundPageResult)>; // 出库分页回调
    virtual ~IOutboundRepository() = default;
    // 创建草稿订单(必须返回有效的订单OrderNo和订单ID)
    // 确保orderNo在INSERT前已经生成(Service层创建的orderNo允许为空,但是数据库层不允许为空!)
    virtual void createDraft(
        const OutboundOrder& order,
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
        const OutboundOrderFilter& filter,
        const PageRequest& request,
        QObject* owner,
        PageCallback callback)
        = 0;
    // 确认订单(出库:条件扣减库存,详见类注释中的出库库存规则)
    virtual void confirmOrder(
        quint32 id,
        const AuditContext& auditContext,
        QObject* owner,
        OperateCallback callback)
        = 0;
};
