// 出库单请求
#pragma once
#include <QString>
#include <QVector>
#include <optional>
#include "OutboundOrder.h"
#include "OutboundDto.h"
// 出库订单行创建请求
struct CreateOutboundOrderLineRequest {
    quint32 productId { 0 };
    int quantity { 0 };
    double unitPrice { 0.0 };
};
// 出库订单创建请求
struct CreateOutboundOrderRequest {
    QString recipient; // 接收人/领用方
    quint32 warehouseId { 0 }; // 仓库id
    QString remark; // 备注
    QVector<CreateOutboundOrderLineRequest> lines; // 订单行请求
};
