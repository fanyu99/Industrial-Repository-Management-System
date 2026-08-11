#include "InboundEditDialog.h"
#include <QPushButton>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QWidget>
InboundEditDialog::InboundEditDialog(InboundEditMode editMode, QWidget* parent)
    : editMode_(editMode)
    , QDialog(parent)
{
    this->setWindowTitle(QStringLiteral("创建入库订单"));
    // 主布局
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    // 表单布局
    QFormLayout* formLayout = new QFormLayout();
    // 订单编号
    orderNoEdit_ = new QLineEdit(this);
    formLayout->addRow(QStringLiteral("入库订单编号"), orderNoEdit_);
    orderNoEdit_->setReadOnly(true); // 只读(创建时自动生成)
    // 1.供应商
    supplierEdit_ = new QLineEdit(this);
    supplierEdit_->setPlaceholderText(QStringLiteral("请输入供应商"));
    formLayout->addRow(QStringLiteral("供应商"), supplierEdit_);
    // 2.操作人
    operatorNameEdit_ = new QLineEdit(this);
    operatorNameEdit_->setReadOnly(true); // 只读
    formLayout->addRow(QStringLiteral("操作人"), operatorNameEdit_);
    // 3.仓库
    warehouseComboBox_ = new QComboBox(this);
    formLayout->addRow(QStringLiteral("仓库:"), warehouseComboBox_);
    // 4.仅显示活跃仓库复选框
    masterdataActiveOnlyCheckBox_ = new QCheckBox(QStringLiteral("仅显示活跃仓库"));
    masterdataActiveOnlyCheckBox_->setChecked(true);
    formLayout->addRow(QStringLiteral("仅显示活跃仓库"), masterdataActiveOnlyCheckBox_);
    // 5.明细数
    lineCountEdit_ = new QLineEdit(QStringLiteral("0"));
    lineCountEdit_->setReadOnly(true); // 自动计算
    formLayout->addRow(QStringLiteral("明细数"), lineCountEdit_);
    // 6.总数量
    totalQuantityEdit_ = new QLineEdit(QStringLiteral("0"));
    totalQuantityEdit_->setReadOnly(true); // 自动计算
    formLayout->addRow(QStringLiteral("总数量"), totalQuantityEdit_);
    // 7.确认按钮框
    sureButtonBox_ = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(sureButtonBox_);
    // 连接信号槽
    connect(sureButtonBox_, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(sureButtonBox_, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(masterdataActiveOnlyCheckBox_,&QCheckBox::toggled,this,&InboundEditDialog::masterdataActiveOnlyChanged);
    setMode(editMode_);
}
// 设置订单信息
void InboundEditDialog::setInboundOrder(const InboundOrderListItemDto& order)
{
    pendingWarehouseId_ = order.warehouseId;

    if (orderNoEdit_)
        orderNoEdit_->setText(order.orderNo);
    if (supplierEdit_)
        supplierEdit_->setText(order.supplier);
    if (operatorNameEdit_)
        operatorNameEdit_->setText(order.operatorName);
    if (warehouseComboBox_) {
        const int idx = warehouseComboBox_->findData(order.warehouseId);
        if (idx >= 0)
            warehouseComboBox_->setCurrentIndex(idx);
    }
    if (lineCountEdit_)
        lineCountEdit_->setText(QString::number(order.lineCount));
    if (totalQuantityEdit_)
        totalQuantityEdit_->setText(QString::number(order.totalQuantity));
}
// 设置编辑模式
void InboundEditDialog::setMode(InboundEditMode mode)
{
    editMode_ = mode;
    switch (editMode_) {
    case InboundEditMode::Create:
        if (supplierEdit_)
            supplierEdit_->setEnabled(true);
        if (operatorNameEdit_)
            operatorNameEdit_->setEnabled(true);
        if (warehouseComboBox_)
            warehouseComboBox_->setEnabled(true);
        if (masterdataActiveOnlyCheckBox_)
            masterdataActiveOnlyCheckBox_->setEnabled(true);
        break;
    default:
        break;
    }
}
// 添加仓库
bool InboundEditDialog::addWarehouse(const QString& warehouseName, quint32 warehouseId, QString& errorMessage)
{
    if (!warehouseComboBox_) {
        errorMessage = QStringLiteral("仓库选择框不存在");
        return false;
    }
    if (warehouseName.trimmed().isEmpty()) {
        errorMessage = QStringLiteral("仓库名称不能为空");
        return false;
    }
    if (warehouseId == 0) {
        errorMessage = QStringLiteral("仓库ID不合法");
        return false;
    }
    const int index = warehouseComboBox_->count();
    warehouseComboBox_->addItem(warehouseName, warehouseId);
    if (pendingWarehouseId_ == warehouseId)
        warehouseComboBox_->setCurrentIndex(index);
    errorMessage.clear();
    return true;
}
// 校验输入
bool InboundEditDialog::validateInput(QString& errorMessage) const noexcept
{
    if (!errorMessage.isEmpty()) {
        errorMessage.clear();
    }
    if (!orderNoEdit_) {
        errorMessage = QStringLiteral("订单编号不可用!");
        return false;
    }
    if (!supplierEdit_) {
        errorMessage = QStringLiteral("供应商不可用!");
        return false;
    }
    if (!operatorNameEdit_) {
        errorMessage = QStringLiteral("操作人不可用!");
        return false;
    }
    if (!warehouseComboBox_) {
        errorMessage = QStringLiteral("仓库选择框不存在!");
        return false;
    }
    if (!lineCountEdit_) {
        errorMessage = QStringLiteral("明细数不可用!");
        return false;
    }
    if (!totalQuantityEdit_) {
        errorMessage = QStringLiteral("总数量不可用!");
        return false;
    }
    if (supplierEdit_->text().trimmed().isEmpty()) {
        errorMessage = QStringLiteral("供应商不能为空!");
        return false;
    }
    if (operatorNameEdit_->text().trimmed().isEmpty()) {
        errorMessage = QStringLiteral("操作人不能为空!");
        return false;
    }
    if (warehouseComboBox_->count() == 0 || warehouseComboBox_->currentData().isNull()) {
        errorMessage = QStringLiteral("请选择仓库!");
        return false;
    }
    if (lineCountEdit_->text().toInt() <= 0) {
        errorMessage = QStringLiteral("明细数必须大于0!");
        return false;
    }
    if (totalQuantityEdit_->text().toInt() <= 0) {
        errorMessage = QStringLiteral("总数量必须大于0!");
        return false;
    }
    return true;
}
// 是否仅显示活跃仓库
bool InboundEditDialog::isMasterdataActiveOnly() const noexcept
{
    return masterdataActiveOnlyCheckBox_->isChecked();
}
// 设置加载中
void InboundEditDialog::setOptionsLoading(bool loading)
{
    if (warehouseComboBox_) {
        warehouseComboBox_->setEnabled(!loading);
    }
    if (sureButtonBox_) {
        if (auto* okButton = sureButtonBox_->button(QDialogButtonBox::Ok)) {
            okButton->setEnabled(!loading);
        }
    }
}
// 开始重新加载仓库服务数据
quint64 InboundEditDialog::beginWarehouseReload()
{
    ++warehouseReloadId_;
    clearWarehouseOptions();
    setOptionsLoading(true);
    return warehouseReloadId_;
}

// 是否是当前重新加载的仓库服务数据
bool InboundEditDialog::isCurrentWarehouseReload(quint64 reloadId) const noexcept
{
    return reloadId == warehouseReloadId_;
}

// 完成重新加载仓库服务数据
void InboundEditDialog::finishWarehouseReload(quint64 reloadId, bool success)
{
    if (!isCurrentWarehouseReload(reloadId))
        return;
    if (warehouseComboBox_) {
        warehouseComboBox_->setEnabled(success);
    }
    if(sureButtonBox_)
        if(auto* okButton = sureButtonBox_->button(QDialogButtonBox::Ok))
            okButton->setEnabled(success);
}
// 清除仓库服务数据
void InboundEditDialog::clearWarehouseOptions()
{
    if (!warehouseComboBox_)
        return;
    warehouseComboBox_->clear();
}