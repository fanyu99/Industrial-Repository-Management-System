#pragma once
#include <QDateTime>
#include <QString>
#include <QVector>
#include <optional>
// 入库订单状态
enum class InboundOrderStatus {
    Draft, // 草稿
    Confirmed, // 已确认
    Cancelled // 已取消
};
// 入库订单明细
struct InboundOrderLine {
    quint32 id { 0 }; // id 订单行id
    quint32 orderId { 0 }; // 入库订单id
    quint32 productId { 0 }; // 商品id
    int quantity { 0 }; // 数量
    double unitPrice { 0.0 }; // 单价
};
// 入库订单头
struct InboundOrder {
    quint32 id { 0 }; // 订单id
    QString orderNo; // 订单号
    QString supplier; // 供应商
    InboundOrderStatus status { InboundOrderStatus::Draft }; // 订单状态
    quint32 operatorId { 0 }; // 操作人id
    quint32 warehouseId { 0 }; // 仓库id
    QString remark; // 备注
    QVector<InboundOrderLine> lines; // 订单行
    std::optional<QDateTime> confirmedAt; // 确认时间(确认订单时设置)
    QDateTime createdAt; // 创建时间
    QDateTime updatedAt; // 更新时间
    
};
