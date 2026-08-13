#pragma once
#include <QDateTime>
#include <QString>
#include <QVector>
#include <optional>

// 出库订单状态(与入库订单状态一致)
enum class OutboundOrderStatus {
    Draft, // 草稿
    Confirmed, // 已确认
    Cancelled // 已取消
};
// 出库订单明细
struct OutboundOrderLine {
    quint32 id { 0 }; // 订单行id
    quint32 orderId { 0 }; // 出库订单id
    quint32 productId { 0 }; // 商品id
    int quantity { 0 }; // 数量(>0,确认时不得超过该仓库可用库存)
    double unitPrice { 0.0 }; // 单价(出库单价/成本)
};
// 出库订单头
struct OutboundOrder {
    quint32 id { 0 }; // 订单id
    QString orderNo; // 订单号(OUT-YYYYMMDD-序号)
    QString recipient; // 接收人/领用方(与入库的supplier语义不同)
    OutboundOrderStatus status { OutboundOrderStatus::Draft }; // 订单状态
    quint32 operatorId { 0 }; // 操作人id
    quint32 warehouseId { 0 }; // 仓库id
    QString remark; // 备注
    QVector<OutboundOrderLine> lines; // 订单行
    std::optional<QDateTime> confirmedAt; // 确认时间(确认订单时设置)
    QDateTime createdAt; // 创建时间
    QDateTime updatedAt; // 更新时间
};
