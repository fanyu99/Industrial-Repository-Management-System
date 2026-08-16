#include "OutboundTableModel.h"

OutboundTableModel::OutboundTableModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

// 返回行数
int OutboundTableModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return page_.items.size();
}

// 返回列数
int OutboundTableModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return static_cast<int>(Column::CountColumn);
}

// 返回索引的数据
QVariant OutboundTableModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return {};
    if (index.row() < 0 || index.row() >= page_.items.size())
        return {};
    const OutboundOrderListItemDto& order = page_.items[index.row()];
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (static_cast<Column>(index.column())) {
        case Column::OrderNoColumn:
            return order.orderNo;
        case Column::RecipientColumn:
            return order.recipient;
        case Column::StatusColumn:
            switch (order.status) {
            case OutboundOrderStatus::Draft:
                return QStringLiteral("待处理");
            case OutboundOrderStatus::Confirmed:
                return QStringLiteral("已确认");
            case OutboundOrderStatus::Cancelled:
                return QStringLiteral("已取消");
            default:
                return {};
            }
        case Column::OperatorNameColumn:
            return order.operatorName;
        case Column::WarehouseNameColumn:
            return order.warehouseName;
        case Column::LineCountColumn:
            return order.lineCount;
        case Column::TotalQuantityColumn:
            return order.totalQuantity;
        case Column::CreateAtColumn:
            return order.createdAt.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
        case Column::UpdateAtColumn:
            return order.updatedAt.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
        case Column::ConfirmAtColumn:
            if (order.confirmedAt.has_value() && order.status == OutboundOrderStatus::Confirmed)
                return order.confirmedAt.value().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
            else
                return {};
        default:
            return {};
        }
    }
    return {};
}
// 设置展示的出库订单信息
void OutboundTableModel::setPage(const PageResult<OutboundOrderListItemDto>& page)
{
    beginResetModel();
    this->page_ = page;
    endResetModel();
}
// 返回表头数据
QVariant OutboundTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (static_cast<Column>(section)) {
        case Column::OrderNoColumn:
            return QStringLiteral("出库订单编号");
        case Column::RecipientColumn:
            return QStringLiteral("接收人");
        case Column::StatusColumn:
            return QStringLiteral("状态");
        case Column::OperatorNameColumn:
            return QStringLiteral("操作人");
        case Column::WarehouseNameColumn:
            return QStringLiteral("仓库");
        case Column::LineCountColumn:
            return QStringLiteral("订单明细数");
        case Column::TotalQuantityColumn:
            return QStringLiteral("总数量");
        case Column::CreateAtColumn:
            return QStringLiteral("创建时间");
        case Column::UpdateAtColumn:
            return QStringLiteral("更新时间");
        case Column::ConfirmAtColumn:
            return QStringLiteral("确认时间");
        default:
            return {};
        }
    }
    return {};
}
// 清除数据
void OutboundTableModel::clear()
{
    beginResetModel();
    page_ = {};
    endResetModel();
}
// 获取指定行的出库订单
OutboundOrderListItemDto OutboundTableModel::itemAt(int row) const
{
    if (row >= 0 && row < page_.items.size())
        return page_.items[row];
    return {};
}
// 获取对应行的出库订单ID
quint32 OutboundTableModel::orderIdAt(int row) const
{
    if (row >= 0 && row < page_.items.size())
        return page_.items[row].id;
    return 0;
}
// 获取总数
int OutboundTableModel::total() const noexcept
{
    return page_.total;
}
// 获取对应行的出库订单操作人ID
quint32 OutboundTableModel::operatorIdAt(int row) const
{
    if (row >= 0 && row < page_.items.size())
        return page_.items[row].operatorId;
    return 0;
}
// 获取对应行的出库订单仓库ID
quint32 OutboundTableModel::warehouseIdAt(int row) const
{
    if (row >= 0 && row < page_.items.size())
        return page_.items[row].warehouseId;
    return 0;
}
// 获取当前页码
int OutboundTableModel::page() const noexcept
{
    return page_.page;
}
// 获取每页的大小
int OutboundTableModel::pageSize() const noexcept
{
    return page_.pageSize;
}
// 获取总页数
int OutboundTableModel::totalPages() const noexcept
{
    return (total() + pageSize() - 1) / pageSize();
}
