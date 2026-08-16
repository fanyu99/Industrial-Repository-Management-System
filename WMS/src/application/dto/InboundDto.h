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
    QString orderNo;// INB-YYYYMMDD-序号
    QString supplier;
    InboundOrderStatus status { InboundOrderStatus::Draft };

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
// 入库订单筛选器
struct InboundOrderFilter {
    QString keyword; // 关键词
    std::optional<InboundOrderStatus> status; // 状态选择
    std::optional<quint32> warehouseId; // 仓库id
    std::optional<QString> orderNo; // 订单号
    std::optional<QString> supplier; // 供应商
    std::optional<QString> operatorName; // 操作人
    std::optional<QString> warehouseName; // 仓库名称
};
// 入库订单行详情
struct InboundOrderDetailLineDto {
    quint32 productId { 0 };
    QString productCode;
    QString productName;
    int quantity { 0 };
    double unitPrice { 0.0 };
    double subtotal { 0.0 };

};
// 入库订单详情
struct InboundOrderDetailDto {
    
    quint32 id { 0 };
    QString orderNo;
    QString supplier;
    InboundOrderStatus status { InboundOrderStatus::Draft };

    quint32 operatorId { 0 };
    QString operatorName;

    quint32 warehouseId { 0 };
    QString warehouseName;

    QString remark;
    int lineCount { 0 }; // 订单行数量
    int totalQuantity { 0 }; // 总数量
    double totalAmount { 0.0 }; // 总金额

    QDateTime createdAt;
    QDateTime updatedAt;
    std::optional<QDateTime> confirmedAt;

    QVector<InboundOrderDetailLineDto> detailLines;

    
 };
