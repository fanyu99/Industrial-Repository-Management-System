// ProductEditDialog 产品编辑对话框
#pragma once
#include "ProductDto.h"
#include "ProductRequests.h"
#include <QAbstractItemView>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QString>
#include <QTextEdit>
#include <QVBoxLayout>
// 编辑模式
enum class ProductEditMode {
    Create, // 创建产品
    Edit, // 编辑产品
    EditState // 编辑产品状态
};
class ProductEditDialog : public QDialog {
    Q_OBJECT
public:
    ProductEditDialog(ProductEditMode mode = ProductEditMode::Create, QWidget* parent = nullptr);
    ~ProductEditDialog() = default;
    quint32 editingProductId() const noexcept; // 获取当前编辑的产品ID
    bool isActive() const noexcept; // 获取当前产品的状态
    bool isMasterdataActiveOnly() const noexcept; // 获取分类/单位仅激活复选框状态
    void setProduct(const ProductListItemDto& product); // 设置编辑的产品信息
    bool validateInput(QString& errorMessage) const noexcept; // 校验输入
    CreateProductRequest createRequest() const noexcept; // 创建创建产品请求
    UpdateProductRequest updateRequest() const noexcept; // 创建更新产品请求
    void setMode(ProductEditMode mode); // 设置编辑模式
    bool addCategory(const QString& categoryName, quint32 categoryId, QString& errorMessage); // ComboBox添加分类
    bool addUnit(const QString& unitName, quint32 unitId, QString& errorMessage); // ComboBox添加单位
    // 设置加载中
    void setOptionsLoading(bool loading);
    // 信号
signals:
    // 分类/单位仅激活复选框状态改变信号
    void masterdataActiveOnlyChanged(bool activeOnly);

public:
    quint64 beginMasterDataReload(); // 开始重新加载基础数据
    bool isCurrentMasterDataReload(quint64 reloadId) const noexcept; // 是否是当前重新加载的基础数据
    void finishMasterDataReload(quint64 reloadId, bool success); // 完成重新加载基础数据
    void clearMasterDataOptions(); // 清除基础数据选项
private:
    QLineEdit* codeEdit_; // 产品编码编辑
    QLineEdit* nameEdit_; // 产品名称编辑
    QComboBox* categoryNameComboBox_; // 分类选择框(文本显示名称,Data保存id)
    QComboBox* unitNameComboBox_; // 单位选择框(文本显示名称,Data保存id)
    QTextEdit* specificationEdit_; // 产品规格编辑
    QSpinBox* safetyStockSpin_; // 安全库存编辑
    QCheckBox* activeCheckBox_; // 产品状态复选框
    QCheckBox* masterdataActiveOnlyCheckBox_; // 分类/单位仅激活复选框
    QDialogButtonBox* sureButtonBox_; // 确认/取消按钮框
    quint32 editingProductId_ { 0 }; // 编辑的产品ID
    ProductEditMode editMode_ { ProductEditMode::Create }; // 编辑模式(创建/编辑)
    quint32 pendingCategoryId_ { 0 }; // 待回显分类ID
    quint32 pendingUnitId_ { 0 }; // 待回显单位ID
    quint64 masterDataReloadSeq_ { 0 }; // 基础数据重新加载序列号
};