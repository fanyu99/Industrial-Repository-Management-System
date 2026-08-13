#include "InboundEditDialog.h"
#include "qabstractitemview.h"
#include "qboxlayout.h"
#include "qheaderview.h"
#include "qpushbutton.h"
#include "qtablewidget.h"
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>
InboundEditDialog::InboundEditDialog(InboundEditMode editMode, QWidget* parent)
    : editMode_(editMode)
    , QDialog(parent)
{
    this->setWindowTitle(QStringLiteral("创建入库订单"));
    this->resize(600, 500);
    this->setMinimumSize(500, 400);
    // 主布局
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    // 表单布局
    QFormLayout* formLayout = new QFormLayout();
    // 订单编号
    orderNoEdit_ = new QLineEdit(this);
    orderNoEdit_->setPlaceholderText(QStringLiteral("自动生成:INB-YYYYMMDD-######"));
    orderNoEdit_->setReadOnly(true); // 只读(创建时自动生成)
    formLayout->addRow(QStringLiteral("入库订单编号"), orderNoEdit_);
    // 1.供应商
    supplierEdit_ = new QLineEdit(this);
    supplierEdit_->setPlaceholderText(QStringLiteral("请输入供应商"));
    formLayout->addRow(QStringLiteral("供应商"), supplierEdit_);
    // 2.操作人
    operatorNameEdit_ = new QLineEdit(this);
    operatorNameEdit_->setPlaceholderText(QStringLiteral("当前用户"));
    operatorNameEdit_->setReadOnly(true); // 只读
    formLayout->addRow(QStringLiteral("操作人"), operatorNameEdit_);
    // 3.仓库
    warehouseComboBox_ = new QComboBox(this);
    formLayout->addRow(QStringLiteral("仓库:"), warehouseComboBox_);
    // 4.仅显示活跃仓库复选框
    masterdataActiveOnlyCheckBox_ = new QCheckBox(QStringLiteral("仅显示活跃仓库"), this);
    masterdataActiveOnlyCheckBox_->setChecked(true);
    formLayout->addRow(QStringLiteral("仅显示活跃仓库"), masterdataActiveOnlyCheckBox_);
    // 5.明细数
    lineCountEdit_ = new QLineEdit(QStringLiteral("0"), this);
    lineCountEdit_->setReadOnly(true); // 自动计算
    formLayout->addRow(QStringLiteral("明细数"), lineCountEdit_);
    // 6.总数量
    totalQuantityEdit_ = new QLineEdit(QStringLiteral("0"), this);
    totalQuantityEdit_->setReadOnly(true); // 自动计算
    formLayout->addRow(QStringLiteral("总数量"), totalQuantityEdit_);
    // 7.备注
    remarkEdit_ = new QTextEdit(this);
    formLayout->addRow(QStringLiteral("备注"), remarkEdit_);
    // 8.添加行/删除行按钮布局
    QHBoxLayout* lineBtnLayout = new QHBoxLayout();
    addLineBtn_ = new QPushButton(QStringLiteral("添加行"), this);
    removeLineBtn_ = new QPushButton(QStringLiteral("删除行"), this);
    lineBtnLayout->addWidget(addLineBtn_);
    lineBtnLayout->addWidget(removeLineBtn_);

    // 9.入库订单明细表
    linesTable_ = new QTableWidget(this);
    linesTable_->setColumnCount(static_cast<int>(LineColumn::CountColumn));
    linesTable_->setHorizontalHeaderLabels({
        QStringLiteral("产品ID"),
        QStringLiteral("数量"),
        QStringLiteral("单价"),
    });
    linesTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    linesTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    linesTable_->horizontalHeader()->setStretchLastSection(true);
    linesTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // 10.确认按钮框
    sureButtonBox_ = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);

    // 添加布局
    mainLayout->addLayout(formLayout);
    mainLayout->addLayout(lineBtnLayout);
    mainLayout->addWidget(linesTable_);
    mainLayout->addWidget(sureButtonBox_);

    // 连接信号槽
    connect(sureButtonBox_, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(sureButtonBox_, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(masterdataActiveOnlyCheckBox_, &QCheckBox::toggled, this, &InboundEditDialog::masterdataActiveOnlyChanged);
    connect(linesTable_, &QTableWidget::itemChanged, this, &InboundEditDialog::updateLineSummary);
    connect(addLineBtn_, &QPushButton::clicked, this, &InboundEditDialog::addEmptyLine);
    connect(removeLineBtn_, &QPushButton::clicked, this, &InboundEditDialog::removeSelectedLine);
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
        if (linesTable_)
            linesTable_->setEnabled(true);
        if (addLineBtn_)
            addLineBtn_->setEnabled(true);
        if (removeLineBtn_)
            removeLineBtn_->setEnabled(false);
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
    if (!linesTable_ || linesTable_->rowCount() == 0) {
        errorMessage = QStringLiteral("请至少添加一行订单行!");
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
    for (int r = 0; r < linesTable_->rowCount(); ++r) {
        bool productIdOk = false;
        bool quantityOk = false;
        bool unitPriceOk = false;
        const quint32 productId = linesTable_->item(r, static_cast<int>(LineColumn::ProductId)) ? linesTable_->item(r, static_cast<int>(LineColumn::ProductId))->text().toUInt(&productIdOk) : 0;
        const int quantity = linesTable_->item(r, static_cast<int>(LineColumn::Quantity)) ? linesTable_->item(r, static_cast<int>(LineColumn::Quantity))->text().toInt(&quantityOk) : 0;
        const double unitPrice = linesTable_->item(r, static_cast<int>(LineColumn::UnitPrice)) ? linesTable_->item(r, static_cast<int>(LineColumn::UnitPrice))->text().toDouble(&unitPriceOk) : 0.0;
        if (!productIdOk || productId == 0) {
            errorMessage = QStringLiteral("第%1行商品ID不合法!").arg(r + 1);
            return false;
        }
        if (!quantityOk || quantity <= 0) {
            errorMessage = QStringLiteral("第%1行数量不合法!").arg(r + 1);
            return false;
        }
        if (!unitPriceOk || unitPrice < 0.0) {
            errorMessage = QStringLiteral("第%1行单价不合法!").arg(r + 1);
            return false;
        }
    }
    return true;
}
// 是否仅显示活跃仓库
bool InboundEditDialog::isMasterdataActiveOnly() const noexcept
{
    if (!masterdataActiveOnlyCheckBox_)
        return false;
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
    if (!warehouseComboBox_)
        return 0;
    ++warehouseReloadId_;
    clearWarehouseOptions();
    setOptionsLoading(true);
    return warehouseReloadId_;
}
// 设置操作人
void InboundEditDialog::setOperatorName(const QString& operatorName)
{
    if (operatorNameEdit_)
        operatorNameEdit_->setText(QStringLiteral("当前用户:%1").arg(operatorName));
}
// 添加订单行
void InboundEditDialog::addEmptyLine()
{
    if (!linesTable_)
        return;
    const int row = linesTable_->rowCount();
    linesTable_->insertRow(row);
    linesTable_->setItem(row, 0, new QTableWidgetItem());
    linesTable_->setItem(row, 1, new QTableWidgetItem(QStringLiteral("1")));
    linesTable_->setItem(row, 2, new QTableWidgetItem(QStringLiteral("0.00")));
    updateLineSummary(); // 更新数据
}
// 删除选中行
void InboundEditDialog::removeSelectedLine()
{
    if (!linesTable_)
        return;
    const auto ranges = linesTable_->selectedRanges(); // 获取选中的行
    if (ranges.isEmpty())
        return;
    linesTable_->removeRow(ranges.first().topRow()); // 删除选中行
    updateLineSummary(); // 更新数据
}
// 更新订单表数据
void InboundEditDialog::updateLineSummary()
{
    if (!linesTable_ || !lineCountEdit_ || !totalQuantityEdit_)
        return;
    int totalQuantity = 0;
    for (int r = 0; r < linesTable_->rowCount(); ++r) {
        const auto* quantityItem = linesTable_->item(r, static_cast<int>(LineColumn::Quantity));
        bool ok = false;
        const int quantity = quantityItem ? quantityItem->text().toInt(&ok) : 0;
        if (ok && quantity > 0)
            totalQuantity += quantity;
    }
    // 设置明细数和总数量
    lineCountEdit_->setText(QString::number(linesTable_->rowCount()));
    totalQuantityEdit_->setText(QString::number(totalQuantity));
}
// 获取创建入库单请求(未校验相关数据)
CreateInboundOrderRequest InboundEditDialog::createRequest() const noexcept
{
    CreateInboundOrderRequest request;
    if (supplierEdit_)
        request.supplier = supplierEdit_->text().trimmed();
    if (warehouseComboBox_)
        request.warehouseId = warehouseComboBox_->currentData().toUInt();
    if (remarkEdit_)
        request.remark = remarkEdit_->toPlainText().trimmed();
    if (!linesTable_)
        return {};
    for (int r = 0; r < linesTable_->rowCount(); ++r) {
        const quint32 productId = linesTable_->item(r, static_cast<int>(LineColumn::ProductId)) ? linesTable_->item(r, static_cast<int>(LineColumn::ProductId))->text().toUInt() : 0;
        const int quantity = linesTable_->item(r, static_cast<int>(LineColumn::Quantity)) ? linesTable_->item(r, static_cast<int>(LineColumn::Quantity))->text().toInt() : 0;
        const double unitPrice = linesTable_->item(r, static_cast<int>(LineColumn::UnitPrice)) ? linesTable_->item(r, static_cast<int>(LineColumn::UnitPrice))->text().toDouble() : 0.0;
        CreateInboundOrderLineRequest lineRequest;
        lineRequest.productId = productId;
        lineRequest.quantity = quantity;
        lineRequest.unitPrice = unitPrice;
        request.lines.append(lineRequest);
    }
    return request;
}

// 仓库数据动态刷新:
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
    if (sureButtonBox_)
        if (auto* okButton = sureButtonBox_->button(QDialogButtonBox::Ok))
            okButton->setEnabled(success);
}
// 清除仓库服务数据
void InboundEditDialog::clearWarehouseOptions()
{
    if (!warehouseComboBox_)
        return;
    warehouseComboBox_->clear();
}