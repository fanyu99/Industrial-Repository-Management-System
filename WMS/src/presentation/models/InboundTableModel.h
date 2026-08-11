 // 入库订单页
#pragma once
#include "InboundDto.h"
#include "ProductDto.h"
#include <QVariant>
#include <QModelIndex>
#include <QObject>
#include <QString>
#include <QAbstractTableModel>
#include <QPushButton>
#include <QWidget>
class InboundTableModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum class Column {
        OrderNoColumn, // 订单号
        SupplierColumn, // 供应商
        StatusColumn, // 状态
        OperatorNameColumn, // 操作人名
        WarehouseNameColumn, // 仓库名
        LineCountColumn, // 订单明细数
        TotalQuantityColumn, // 总数量
        CreateAtColumn, // 创建时间
        UpdateAtColumn, // 更新时间
        ConfirmAtColumn, // 确认时间
        CountColumn,  // 当前列数
    };
    explicit InboundTableModel(QObject* parent = nullptr);
    ~InboundTableModel() = default;
    // 返回行数
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    // 返回列数
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    // 获取数据
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    // 获取表头数据
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    void clear(); // 清空当前页的入库订单信息
    void setPage(const PageResult<InboundOrderListItemDto>& page); // 设置该页展示的入库订单
    InboundOrderListItemDto itemAt(int row) const; // 获取指定行的入库订单信息
    quint32 orderIdAt(int row) const; // 获取对应行的入库订单ID
    quint32 operatorIdAt(int row) const; // 获取对应行的入库订单操作人ID
    quint32 warehouseIdAt(int row) const; // 获取对应行的入库订单仓库ID
    int total() const noexcept;
    int page() const noexcept;
    int pageSize() const noexcept;

private:
    PageResult<InboundOrderListItemDto> page_; // 当前页,入库订单
};