// 产品表格模型 ProductTableModel
// 用于与ProductService进行交互,并在UI中展示产品信息
#pragma once
#include "ProductDto.h"
#include <QAbstractTableModel>
#include <QModelIndex>
#include <QObject>
#include <QString>
#include <QVariant>
#include <QVector>
// 设置当前页的状态
enum class PageState {
    Idle, // 空闲
    Loading, // 加载中
    Ready, // 就绪(有数据)
    Empty, // 空(无数据)
    Error, // 错误
};
// ProductTableModel
class ProductTableModel : public QAbstractTableModel {
    Q_OBJECT
public:
    // 列枚举
    enum class Column {
        CodeColumn = 0,
        NameColumn,
        CategoryColumn,
        UnitColumn,
        SpecificationColumn,
        SafetyStockColumn,
        ActiveColumn,
        CountColumn
    };
    explicit ProductTableModel(QObject* parent = nullptr);
    ~ProductTableModel() = default;
    // 重写rowCount,columnCount,data,headerData
    int rowCount(const QModelIndex& parent = QModelIndex()) const override; // 返回行数
    int columnCount(const QModelIndex& parent = QModelIndex()) const override; // 返回列数
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override; // 返回数据
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override; // 返回列的表头显示
    void clear();
    void setPage(const PageResult<ProductListItemDto>& page); // 设置该页展示的产品
    ProductListItemDto itemAt(int row) const; // 获取指定行的产品信息
    quint32 productIdAt(int row) const; // 获取对应行的产品ID
    int total() const noexcept;
    int page() const noexcept;
    int pageSize() const noexcept;

private:
    PageResult<ProductListItemDto> page_; // 当前页,存储分页信息
};