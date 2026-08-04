// InboundOrder.h 入库订单
/*数据库表结构:
1. inbound_orders:
id, order_no, supplier, status, operator_id, warehouse_id,
confirmed_at, remark, created_at, updated_at

2. inbound_details:
id, order_id, product_id, quantity, unit_price
*/
#pragma once
#include <QDateTime>
#include <QString>
#include <QVector>
#include <optional>
#include "InboundOrder.h"
// 入库订单DTO
struct InboundOrderListItemDto {
    quint32 id { 0 };
    QString orderNo;
    QString supplier;
    InboundOrderStatus status { InboundOrderStatus::Draft };
    quint32 operatorId { 0 };
    quint32 warehouseId { 0 };
};
// 入库订单筛选器
struct InboundOrderFilter {
    QString keyword; // 关键词
    std::optional<InboundOrderStatus> status; // 状态选择
    std::optional<quint32> warehouseId; // 仓库id
};