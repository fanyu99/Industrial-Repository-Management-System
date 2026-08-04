// 入库单请求
#pragma once
#include <QString>
#include <QVector>
#include <optional>
#include "InboundOrder.h"
#include "InboundDto.h"
// 入库订单行创建请求
struct CreateInboundOrderLineRequest {
    quint32 productId { 0 };
    int quantity { 0 };
    double unitPrice { 0.0 };
};
// 入库订单创建请求
struct CreateInboundOrderRequest {
    QString supplier; // 供应商
    quint32 warehouseId { 0 }; // 仓库id
    QString remark; // 备注
QVector<CreateInboundOrderLineRequest> lines; // 订单行请求
};
