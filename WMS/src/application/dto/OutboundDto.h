// OutboundOrder.h 出库订单
/*数据库表结构:
1. outbound_orders:
id, order_no, recipient, status, operator_id, warehouse_id,
confirmed_at, remark, created_at, updated_at

2. outbound_details:
id, order_id, product_id, quantity, unit_price
*/
#pragma once
#include <QDateTime>
#include <QString>
#include <QVector>
#include <optional>
#include "OutboundOrder.h"
// 出库订单DTO
struct OutboundOrderListItemDto {
    quint32 id { 0 };
    QString orderNo; // OUT-YYYYMMDD-序号
    QString recipient; // 接收人/领用方
    OutboundOrderStatus status { OutboundOrderStatus::Draft };

    quint32 operatorId { 0 };
    QString operatorName;

    quint32 warehouseId { 0 };
    QString warehouseName;

    int lineCount { 0 }; // 订单行数量
    int totalQuantity { 0 }; // 总数量

    QDateTime createdAt;
    QDateTime updatedAt;
    std::optional<QDateTime> confirmedAt;
};
// 出库订单筛选器
struct OutboundOrderFilter {
    QString keyword; // 关键词
    std::optional<OutboundOrderStatus> status; // 状态选择
    std::optional<quint32> warehouseId; // 仓库id
    std::optional<QString> orderNo; // 订单号
    std::optional<QString> recipient; // 接收人
    std::optional<QString> operatorName; // 操作人
    std::optional<QString> warehouseName; // 仓库名称
};
