#include "ProductTableModel.h"
ProductTableModel::ProductTableModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}
// 返回行数
int ProductTableModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return page_.items.size();
}

// 返回列数
int ProductTableModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return static_cast<int>(Column::CountColumn) + 1;
}
// 返回索引的数据
QVariant ProductTableModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return {};
    if (index.row() < 0 || index.row() >= page_.items.size())
        return {};
    const ProductListItemDto& product = page_.items[index.row()];
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (static_cast<Column>(index.column())) {
        case Column::CodeColumn:
            return product.code;
        case Column::NameColumn:
            return product.name;
        case Column::CategoryColumn:
            return product.categoryName;
        case Column::UnitColumn:
            return product.unitName;
        case Column::SpecificationColumn:
            return product.specification;
        case Column::SafetyStockColumn:
            return product.safetyStock;
        case Column::ActiveColumn:
            return product.active ? QStringLiteral("启用") : QStringLiteral("停用");
        default:
            return {};
        }
    } else
        return {};
}
// 设置列表头
QVariant ProductTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (static_cast<Column>(section)) {
        case Column::CodeColumn:
            return QStringLiteral("物品编码");
        case Column::NameColumn:
            return QStringLiteral("物品名称");
        case Column::CategoryColumn:
            return QStringLiteral("分类");
        case Column::UnitColumn:
            return QStringLiteral("单位");
        case Column::ActiveColumn:
            return QStringLiteral("状态");
        case Column::SafetyStockColumn:
            return QStringLiteral("安全库存");
        case Column::SpecificationColumn:
            return QStringLiteral("规格");
        default:
            return {};
        }
    } else
        return {};
}
// 清除数据
void ProductTableModel::clear()
{
    beginResetModel();
    this->page_ = {};
    endResetModel();
}
// 设置该页展示的产品
void ProductTableModel::setPage(const PageResult<ProductListItemDto>& page)
{
    beginResetModel();
    this->page_ = page;
    endResetModel();
}
// 获取指定行的产品信息
ProductListItemDto ProductTableModel::itemAt(int row) const
{
    if (row >= 0 && row < page_.items.size()) {
        return page_.items[row];
    }
    return {};
}
// 获取对应行的产品ID
quint32 ProductTableModel::productIdAt(int row) const
{
    if (row >= 0 && row < page_.items.size()) {
        return page_.items[row].id;
    }
    return 0;
}
// 获取总数
int ProductTableModel::total() const noexcept
{
    return page_.total;
}
// 获取当前页码
int ProductTableModel::page() const noexcept
{
    return page_.page;
}
// 获取每页大小
int ProductTableModel::pageSize() const noexcept
{
    return page_.pageSize;
}