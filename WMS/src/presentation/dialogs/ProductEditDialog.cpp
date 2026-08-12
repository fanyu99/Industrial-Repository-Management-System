#include "ProductEditDialog.h"
#include <QPushButton>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QComboBox>
ProductEditDialog::ProductEditDialog(ProductEditMode mode, QWidget* parent)
    : editMode_ { mode }
    , QDialog(parent)
{
    this->setWindowTitle(editMode_ == ProductEditMode::Create ? "创建产品" : "编辑产品");
    // 主布局
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    // 表单布局
    QFormLayout* formLayout = new QFormLayout();

    // 1. 产品编码
    codeEdit_ = new QLineEdit(this);
    codeEdit_->setPlaceholderText(QStringLiteral("请输入产品编码"));
    formLayout->addRow(QStringLiteral("产品编码:"), codeEdit_);

    // 2. 产品名称
    nameEdit_ = new QLineEdit(this);
    nameEdit_->setPlaceholderText(QStringLiteral("请输入产品名称"));
    formLayout->addRow(QStringLiteral("产品名称:"), nameEdit_);

    // 3. 产品分类
    categoryNameComboBox_ = new QComboBox(this);
    formLayout->addRow(QStringLiteral("产品分类:"), categoryNameComboBox_);

    // 4. 产品单位
    unitNameComboBox_ = new QComboBox(this);
    formLayout->addRow(QStringLiteral("产品单位:"), unitNameComboBox_);

    // 5. 产品规格
    specificationEdit_ = new QTextEdit(this);
    specificationEdit_->setPlaceholderText(QStringLiteral("请输入产品规格"));
    formLayout->addRow(QStringLiteral("产品规格:"), specificationEdit_);

    // 6. 安全库存
    safetyStockSpin_ = new QSpinBox(this);
    formLayout->addRow(QStringLiteral("安全库存:"), safetyStockSpin_);

    // 7. 产品状态
    activeCheckBox_ = new QCheckBox(QStringLiteral("启用产品"));
    formLayout->addRow(QStringLiteral("产品状态:"), activeCheckBox_);

    // 8. 分类/单位仅激活复选框
    masterdataActiveOnlyCheckBox_ = new QCheckBox(QStringLiteral("仅激活的单位/分类"));
    masterdataActiveOnlyCheckBox_->setChecked(true);
    formLayout->addRow(QStringLiteral("分类/单位仅激活:"), masterdataActiveOnlyCheckBox_);

    mainLayout->addLayout(formLayout);

    // 8. 确认/取消按钮
    sureButtonBox_ = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(sureButtonBox_);
    // 连接信号槽
    connect(sureButtonBox_, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(sureButtonBox_, &QDialogButtonBox::rejected, this, &QDialog::reject);
    // 连接分类/单位仅激活复选框信号槽
    connect(masterdataActiveOnlyCheckBox_, &QCheckBox::toggled, this, &ProductEditDialog::masterdataActiveOnlyChanged);

    // 设置当前的状态
    setMode(editMode_);
}
// 获取当前编辑的产品id
quint32 ProductEditDialog::editingProductId() const noexcept
{
    return editingProductId_;
}
// 获取当前产品的状态
bool ProductEditDialog::isActive() const noexcept
{
    return activeCheckBox_->isChecked();
}
// 是否仅激活的单位/分类显示
bool ProductEditDialog::isMasterdataActiveOnly() const noexcept
{
    return masterdataActiveOnlyCheckBox_->isChecked();
}
// 设置产品信息
void ProductEditDialog::setProduct(const ProductListItemDto& product)
{
    this->editingProductId_ = product.id;
    pendingCategoryId_ = product.categoryId; // 设置待回显分类ID
    pendingUnitId_ = product.unitId; // 设置待回显单位ID
    if (!codeEdit_ || !nameEdit_ || !specificationEdit_ || !safetyStockSpin_ || !activeCheckBox_)
        return;
    this->editingProductId_ = product.id; // 设置当前的编辑的产品id
    codeEdit_->setText(product.code);
    nameEdit_->setText(product.name);
    specificationEdit_->setPlainText(product.specification);
    safetyStockSpin_->setValue(product.safetyStock);
    activeCheckBox_->setChecked(product.active);

    // TODO: 设置产品分类和单位(从后续服务中加载)
    const int categoryIndex = categoryNameComboBox_->findData(product.categoryId);
    if (categoryIndex >= 0) {
        categoryNameComboBox_->setCurrentIndex(categoryIndex);
    }
    const int unitIndex = unitNameComboBox_->findData(product.unitId);
    if (unitIndex >= 0) {
        unitNameComboBox_->setCurrentIndex(unitIndex);
    }
}
// 校验输入
bool ProductEditDialog::validateInput(QString& errorMessage) const noexcept
{
    if (!errorMessage.isEmpty()) {
        errorMessage.clear();
    }
    if (!codeEdit_) {
        errorMessage = QStringLiteral("产品编码框不可用");
        return false;
    }
    if (!nameEdit_) {
        errorMessage = QStringLiteral("产品名称框不可用");
        return false;
    }
    if (!specificationEdit_) {
        errorMessage = QStringLiteral("产品规格框不可用");
        return false;
    }
    if (!safetyStockSpin_) {
        errorMessage = QStringLiteral("安全库存框不可用");
        return false;
    }
    if (!activeCheckBox_) {
        errorMessage = QStringLiteral("产品状态框不可用");
        return false;
    }
    if (!categoryNameComboBox_) {
        errorMessage = QStringLiteral("产品分类框不可用");
        return false;
    }
    if (!unitNameComboBox_) {
        errorMessage = QStringLiteral("产品单位框不可用");
        return false;
    }
    if (codeEdit_->text().trimmed().isEmpty()) {
        errorMessage = QStringLiteral("产品编码不能为空!");
        return false;
    }
    if (nameEdit_->text().trimmed().isEmpty()) {
        errorMessage = QStringLiteral("产品名称不能为空!");
        return false;
    }
    if (categoryNameComboBox_->count() == 0 || categoryNameComboBox_->currentData().isNull()) {
        errorMessage = QStringLiteral("请选择产品分类!");
        return false;
    }
    if (unitNameComboBox_->count() == 0 || unitNameComboBox_->currentData().isNull()) {
        errorMessage = QStringLiteral("请选择产品单位!");
        return false;
    }
    if (safetyStockSpin_->value() < 0) {
        errorMessage = QStringLiteral("安全库存不能小于0!");
        return false;
    }
    return true;
}
// 创建创建产品请求
CreateProductRequest ProductEditDialog::createRequest() const noexcept
{
    if (!codeEdit_ || !nameEdit_ || !specificationEdit_ || !safetyStockSpin_ || !activeCheckBox_ || !categoryNameComboBox_ || !unitNameComboBox_)
        return {};
    CreateProductRequest request;
    request.code = codeEdit_->text().trimmed();
    request.name = nameEdit_->text().trimmed();
    request.categoryId = categoryNameComboBox_->currentData().toUInt();
    request.unitId = unitNameComboBox_->currentData().toUInt();
    request.specification = specificationEdit_->toPlainText().trimmed();
    request.safetyStock = safetyStockSpin_->value();
    request.active = activeCheckBox_->isChecked();
    return request;
}
// 创建更新产品请求
UpdateProductRequest ProductEditDialog::updateRequest() const noexcept
{
    if (!codeEdit_ || !nameEdit_ || !specificationEdit_ || !safetyStockSpin_ || !activeCheckBox_ || !categoryNameComboBox_ || !unitNameComboBox_)
        return {};
    UpdateProductRequest request;
    request.id = editingProductId_;
    request.code = codeEdit_->text().trimmed();
    request.name = nameEdit_->text().trimmed();
    request.categoryId = categoryNameComboBox_->currentData().toUInt();
    request.unitId = unitNameComboBox_->currentData().toUInt();
    request.specification = specificationEdit_->toPlainText().trimmed();
    request.safetyStock = safetyStockSpin_->value();
    request.active = activeCheckBox_->isChecked();
    return request;
}
// 设置编辑模式
void ProductEditDialog::setMode(ProductEditMode mode)
{
    editMode_ = mode;
    switch (mode) {
        // 创建产品
    case ProductEditMode::Create: {
        setWindowTitle(QStringLiteral("创建产品"));
        if (codeEdit_)
            codeEdit_->setEnabled(true); // 创建产品时,可以修改产品编码
        if (nameEdit_)
            nameEdit_->setEnabled(true);
        if (specificationEdit_)
            specificationEdit_->setEnabled(true);
        if (safetyStockSpin_)
            safetyStockSpin_->setEnabled(true);
        if (activeCheckBox_)
            activeCheckBox_->setEnabled(true);
        if (masterdataActiveOnlyCheckBox_)
            masterdataActiveOnlyCheckBox_->setEnabled(true);
        if (categoryNameComboBox_)
            categoryNameComboBox_->setEnabled(true);
        if (unitNameComboBox_)
            unitNameComboBox_->setEnabled(true);
        break;
    }
        // 编辑产品
    case ProductEditMode::Edit:
        setWindowTitle(QStringLiteral("编辑产品"));
        if (codeEdit_)
            codeEdit_->setEnabled(false); // 编辑产品时,产品编码不能修改
        if (nameEdit_)
            nameEdit_->setEnabled(true);
        if (specificationEdit_)
            specificationEdit_->setEnabled(true);
        if (safetyStockSpin_)
            safetyStockSpin_->setEnabled(true);
        if (activeCheckBox_)
            activeCheckBox_->setEnabled(true);
        if (masterdataActiveOnlyCheckBox_)
            masterdataActiveOnlyCheckBox_->setEnabled(true);
        if (categoryNameComboBox_)
            categoryNameComboBox_->setEnabled(true);
        if (unitNameComboBox_)
            unitNameComboBox_->setEnabled(true);
        break;
        // 编辑产品状态
    case ProductEditMode::EditState:
        setWindowTitle(QStringLiteral("编辑产品状态"));
        if (codeEdit_)
            codeEdit_->setEnabled(false);
        if (nameEdit_)
            nameEdit_->setEnabled(false);
        if (specificationEdit_)
            specificationEdit_->setEnabled(false);
        if (safetyStockSpin_)
            safetyStockSpin_->setEnabled(false);
        if (activeCheckBox_)
            activeCheckBox_->setEnabled(true);
        if (masterdataActiveOnlyCheckBox_)
            masterdataActiveOnlyCheckBox_->setEnabled(false);
        if (categoryNameComboBox_)
            categoryNameComboBox_->setEnabled(false);
        if (unitNameComboBox_)
            unitNameComboBox_->setEnabled(false);
        break;
    default:
        break;
    }
}
// ComboBox添加分类
bool ProductEditDialog::addCategory(const QString& categoryName, quint32 categoryId, QString& errorMessage)
{
    if (!errorMessage.trimmed().isEmpty())
        errorMessage.clear();
    if (!categoryNameComboBox_) {
        errorMessage = QStringLiteral("分类选择框不存在");
        return false;
    }
    if (categoryName.trimmed().isEmpty()) {
        errorMessage = QStringLiteral("分类名称不能为空");
        return false;
    }
    if (categoryId == 0) {
        errorMessage = QStringLiteral("分类ID不合法");
        return false;
    }
    const int index = categoryNameComboBox_->count();
    categoryNameComboBox_->addItem(categoryName, categoryId);
    if (categoryId == pendingCategoryId_) {
        categoryNameComboBox_->setCurrentIndex(index);
    }
    errorMessage.clear();
    return true;
}
// ComboBox添加单位
bool ProductEditDialog::addUnit(const QString& unitName, quint32 unitId, QString& errorMessage)
{
    if (!unitNameComboBox_) {
        errorMessage = QStringLiteral("单位选择框不存在");
        return false;
    }
    if (unitName.trimmed().isEmpty()) {
        errorMessage = QStringLiteral("单位名称不能为空");
        return false;
    }
    if (unitId == 0) {
        errorMessage = QStringLiteral("单位ID不合法");
        return false;
    }
    const int index = unitNameComboBox_->count();
    unitNameComboBox_->addItem(unitName, unitId);
    if (unitId == pendingUnitId_) {
        unitNameComboBox_->setCurrentIndex(index);
    }
    errorMessage.clear();
    return true;
}
// 设置加载中(分类/单位禁用)
void ProductEditDialog::setOptionsLoading(bool loading)
{
    if (categoryNameComboBox_)
        categoryNameComboBox_->setEnabled(!loading);
    if (unitNameComboBox_)
        unitNameComboBox_->setEnabled(!loading);
    // 加载完成后才能确认
    if (sureButtonBox_) {
        if (auto* okButton = sureButtonBox_->button(QDialogButtonBox::Ok)) {
            okButton->setEnabled(!loading);
        }
    }
}
// 开始重新加载基础数据,返回序列号
quint64 ProductEditDialog::beginMasterDataReload()
{
    ++masterDataReloadSeq_;// 序列号++
    clearMasterDataOptions(); // 清空选项
    setOptionsLoading(true); // 设置加载
    return masterDataReloadSeq_; 
}

// 是否是最新的重新加载的基础数据
bool ProductEditDialog::isCurrentMasterDataReload(quint64 reloadId) const noexcept
{
    return reloadId == masterDataReloadSeq_;
}
// 完成重新加载
void ProductEditDialog::finishMasterDataReload(quint64 reloadId, bool success)
{
    // 以最新的请求为准
    if (!isCurrentMasterDataReload(reloadId))
        return;
    if (categoryNameComboBox_) {
        categoryNameComboBox_->setEnabled(success);
    }
    if (unitNameComboBox_) {
        unitNameComboBox_->setEnabled(success);
    }
    if (sureButtonBox_) {
        if (auto* okButton = sureButtonBox_->button(QDialogButtonBox::Ok)) {
            okButton->setEnabled(success);
        }
    }
}
// 清除基础数据选项
void ProductEditDialog::clearMasterDataOptions()
{
    if (categoryNameComboBox_)
        categoryNameComboBox_->clear();

    if (unitNameComboBox_)
        unitNameComboBox_->clear();
}
