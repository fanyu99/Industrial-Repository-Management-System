#include "ProductPage.h"
#include <QAbstractItemModel>
#include <QEventLoop>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QStringLiteral>
#include <QVBoxLayout>
#include <QWidget>
ProductPage::ProductPage(ProductService* ps, MasterDataService* masterDataService, ProductTableModel* tableModel, QWidget* parent)
    : QWidget(parent)
    , productService_ { ps }
    , masterDataService_ { masterDataService }
    , tableModel_ { tableModel }
{
    setWindowTitle(QStringLiteral("产品页"));
    if (productService_) {
        currentUser_ = productService_->currentUser();
    }
    this->setObjectName(QStringLiteral("ProductPage"));
    // 创建表格视图
    tableView_ = new QTableView(this);
    tableView_->setObjectName(QStringLiteral("ProductPage_tableView"));
    tableView_->setModel(tableModel_);
    tableView_->setSelectionBehavior(QAbstractItemView::SelectRows); // 整行选
    tableView_->setSelectionMode(QAbstractItemView::SingleSelection); // 单选
    selectionModel_ = tableView_->selectionModel();
    // 总布局
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    // 按钮布局
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    createBtn = new QPushButton(QStringLiteral("创建产品"), this);
    editBtn = new QPushButton(QStringLiteral("编辑产品"), this);
    setActiveBtn = new QPushButton(QStringLiteral("设置产品状态(启用/禁用)"), this);
    reloadBtn = new QPushButton(QStringLiteral("重新加载"), this);
    createBtn->setObjectName(QStringLiteral("ProductPage_createBtn"));
    editBtn->setObjectName(QStringLiteral("ProductPage_editBtn"));
    setActiveBtn->setObjectName(QStringLiteral("ProductPage_setActiveBtn"));
    reloadBtn->setObjectName(QStringLiteral("ProductPage_reloadBtn"));
    // 布局添加按钮
    buttonLayout->addWidget(createBtn);
    buttonLayout->addWidget(editBtn);
    buttonLayout->addWidget(setActiveBtn);
    buttonLayout->addWidget(reloadBtn);
    mainLayout->addLayout(buttonLayout);
    // 添加表格视图到布局
    mainLayout->addWidget(tableView_);
    // 连接信号槽
    connect(createBtn, &QPushButton::clicked, this, &ProductPage::onCreateClicked);
    connect(editBtn, &QPushButton::clicked, this, &ProductPage::onEditClicked);
    connect(setActiveBtn, &QPushButton::clicked, this, &ProductPage::onSetActiveClicked);
    connect(reloadBtn, &QPushButton::clicked, this, &ProductPage::reloadCurrentPage);
    if (selectionModel_)
        connect(selectionModel_, &QItemSelectionModel::selectionChanged, this, [this]() { updateActions(); }); // 根据选中修改按钮状态
    updateActions();
}
// 将错误映射为标题
QString ProductPage::errorToTitle(const AppError& error) const
{
    switch (error.category) {
    case AppErrorCategory::Validation:
        return QStringLiteral("验证错误");
    case AppErrorCategory::Auth:
        return QStringLiteral("认证错误");
    case AppErrorCategory::Permission:
        return QStringLiteral("权限不足");
    case AppErrorCategory::Database: {
        if (error.code == AppErrorCode::RepositoryFailure)
            return QStringLiteral("数据库(repository)错误");
        if (error.code == AppErrorCode::DatabaseFailure)
            return QStringLiteral("数据库(Database)错误");
        return QStringLiteral("数据库错误");
    }
    case AppErrorCategory::System:
        return QStringLiteral("系统错误");
    default:
        return QStringLiteral("未知错误");
    }
}
// 显示操作错误信息
void ProductPage::showOperationError(const AppError& error)
{
    if (!error.errorMessage.isEmpty())
        QMessageBox::warning(this, errorToTitle(error), error.errorMessage);
}
// 显示错误消息
void ProductPage::showErrorMessage(const QString& message)
{
    if (!message.isEmpty())
        QMessageBox::warning(this, QStringLiteral("错误"), message);
}
// 显示信息消息
void ProductPage::showInformationMessage(const QString& message)
{
    if (!message.isEmpty())
        QMessageBox::information(this, QStringLiteral("信息"), message);
}
// 设置当前页的状态,并显示消息
void ProductPage::setPageState(PageState state)
{
    currentPageState_ = state;
    updateActions(); // 更新按钮状态
}
// 重新加载当前页面(以最新的请求为主)
void ProductPage::reloadCurrentPage()
{

    const auto requestSeq = ++listRequestSeq_; // 当前的请求序列号
    if (!productService_) {
        setPageState(PageState::Error);
        showErrorMessage(QStringLiteral("产品服务不可用"));
        return;
    }
    if (!tableModel_) {
        setPageState(PageState::Error);
        showErrorMessage(QStringLiteral("表格模型不可用"));
        return;
    }
    setPageState(PageState::Loading);
    // 采用重新查询方式刷新页面数据
    // 使用ProductService来进行获取产品列表(刷新当前页面)
    productService_->listProducts(
        currentFilter_,
        currentRequest_,
        this,
        [this, requestSeq](const ProductPageResult& result) {
            // 检查是否是最新的请求
            if (requestSeq != listRequestSeq_) {
                return;
            }
            // 如果失败,更改状态并显示错误
            if (!result.success) {
                setPageState(PageState::Error);
                if (result.error.has_value())
                    showOperationError(result.error.value());
                else
                    showErrorMessage(QStringLiteral("未知错误!"));
                return;
            }
            // 设置页面
            tableModel_->setPage(result.page);
            if (result.page.items.isEmpty()) {
                setPageState(PageState::Empty);
                return;
            } else {
                setPageState(PageState::Ready);
            }
        });
}
// 获取当前选中的产品DTO
std::optional<ProductListItemDto> ProductPage::selectedProductDto() const
{
    if (!tableView_) {
        return std::nullopt;
    }
    if (!tableModel_) {
        return std::nullopt;
    }
    if (!selectionModel_) {
        return std::nullopt;
    }
    const QModelIndexList selectedRows = selectionModel_->selectedRows();
    if (selectedRows.isEmpty()) {
        return std::nullopt;
    }
    const int row = selectedRows.first().row();
    const ProductListItemDto product = tableModel_->itemAt(row);
    if (product.id == 0)
        return std::nullopt;
    return product;
}
// 将分类/单位服务加载到产品编辑对话框
void ProductPage::loadProductDialogOptions(ProductEditDialog& dialog, bool activeOnly)
{
    if (!masterDataService_) {
        showErrorMessage(QStringLiteral("基础数据服务不可用!"));
        return;
    }
    QEventLoop eventloop; // 等待两个加载完成后再退出
    int readyCount = 0;
    auto Ready = [&]() {
        if(++readyCount==2)eventloop.quit(); };
    masterDataService_->listCategories(&dialog, activeOnly, [&](const CategoryListResult& result) {
        if (result.success && result.categories.has_value()) {
            for (const auto& category : result.categories.value()) {
                QString error;
                dialog.addCategory(category.name, category.id, error);
            }
        }

        Ready();
    });
    masterDataService_->listUnits(&dialog, activeOnly, [&](const UnitListResult& result) {
        if (result.success && result.units.has_value()) {
            for (const auto& unit : result.units.value()) {
                QString error;
                dialog.addUnit(unit.name, unit.id, error);
            }
        }

        Ready();
    });

    eventloop.exec(); // 等待分类/单位加载加载完成
}
// 槽函数

// 创建产品
void ProductPage::onCreateClicked()
{
    // 校验Dialog是否可用
    if (!productService_) {
        showErrorMessage(QStringLiteral("产品服务不可用!"));
        return;
    }
    if (!tableModel_) {
        showErrorMessage(QStringLiteral("表格模型不可用!"));
        return;
    }
    // 校验权限
    if (!productService_->hasPermission(Permission::CreateProducts)) {
        showOperationError(AppError::permissionDenied());
        return;
    }
    ProductEditDialog dialog(ProductEditMode::Create, this);

    // 从分类/单位服务中加载
    loadProductDialogOptions(dialog, dialog.isMasterdataActiveOnly());
    if (dialog.exec() != QDialog::Accepted)
        return;
    // 校验输入
    QString errorMessage;
    if (!dialog.validateInput(errorMessage)) {
        showErrorMessage(errorMessage);
        return;
    }
    const CreateProductRequest request = dialog.createRequest(); // 创建产品请求
    // 创建产品
    Product product;
    product.id = 0;
    product.code = request.code;
    product.name = request.name;
    product.categoryId = request.categoryId;
    product.unitId = request.unitId;
    product.specification = request.specification;
    product.safetyStock = request.safetyStock;
    product.active = request.active;
    // 创建
    setPageState(PageState::Loading);
    productService_->createProduct(
        product,
        this,
        // 回调函数,刷新页面
        [this](const ProductOperationResult& result) {
            if (!result.success) {
                setPageState(PageState::Error);
                if (result.error.has_value())
                    showOperationError(result.error.value());
                else
                    showErrorMessage(QStringLiteral("未知错误!"));
                return;
            }
            showInformationMessage(QStringLiteral("创建产品成功!"));
            reloadCurrentPage(); // 重新加载
        });
}
// 更新产品
void ProductPage::onEditClicked()
{
    // 校验Dialog是否可用
    if (!productService_) {
        showErrorMessage(QStringLiteral("产品服务不可用!"));
        return;
    }
    if (!tableModel_) {
        showErrorMessage(QStringLiteral("表格模型不可用!"));
        return;
    }
    // 校验权限
    if (!productService_->hasPermission(Permission::EditProducts)) {
        showOperationError(AppError::permissionDenied());
        return;
    }
    // 校验当前的选中
    if (!selectionModel_) {
        showErrorMessage(QStringLiteral("表格模型不可用!"));
        return;
    }
    if (!tableView_) {
        showErrorMessage(QStringLiteral("表格视图不可用!"));
        return;
    }
    std::optional<ProductListItemDto> productDto = selectedProductDto();
    if (!productDto.has_value()) {
        showInformationMessage(QStringLiteral("请选择要编辑的产品!"));
        return;
    }
    // 设置产品信息

    ProductEditDialog dialog(ProductEditMode::Edit, this);

    // 加载分类/单位
    loadProductDialogOptions(dialog, dialog.isMasterdataActiveOnly());
    dialog.setProduct(productDto.value());
    if (dialog.exec() != QDialog::Accepted)
        return;

    // 校验输入
    QString errorMessage;
    if (!dialog.validateInput(errorMessage)) {
        showErrorMessage(errorMessage);
        return;
    }
    const UpdateProductRequest request = dialog.updateRequest(); // 更新产品请求
    // 更新产品
    Product product;
    product.id = request.id;
    product.code = request.code;
    product.name = request.name;
    product.categoryId = request.categoryId;
    product.unitId = request.unitId;
    product.specification = request.specification;
    product.safetyStock = request.safetyStock;
    product.active = request.active;
    // 更新
    setPageState(PageState::Loading);
    productService_->updateProduct(
        product,
        this,
        // 回调函数,刷新页面
        [this](const ProductOperationResult& result) {
            if (!result.success) {
                setPageState(PageState::Error);
                if (result.error.has_value())
                    showOperationError(result.error.value());
                else
                    showErrorMessage(QStringLiteral("未知错误!"));
                return;
            }
            showInformationMessage(QStringLiteral("更新产品成功!"));
            reloadCurrentPage(); // 重新加载
        });
}
// 设置产品状态
void ProductPage::onSetActiveClicked()
{
    if (!productService_) {
        showErrorMessage(QStringLiteral("产品服务不可用!"));
        return;
    }
    if (!tableModel_) {
        showErrorMessage(QStringLiteral("表格模型不可用!"));
        return;
    }
    if (!productService_->hasPermission(Permission::DisableProducts)) {
        showOperationError(AppError::permissionDenied());
        return;
    }
    if (!tableView_) {
        showErrorMessage(QStringLiteral("表格视图不可用!"));
        return;
    }
    if (!selectionModel_) {
        showErrorMessage(QStringLiteral("选择模型不可用!"));
        return;
    }
    std::optional<ProductListItemDto> productDto = selectedProductDto();
    if (!productDto.has_value()) {
        showInformationMessage(QStringLiteral("请选择要设置状态的产品!"));
        return;
    }
    // 使用弹窗进行设置状态
    const bool oppositeActive = !productDto->active;
    const auto answer = QMessageBox::question(this,
        QStringLiteral("确认操作"),
        oppositeActive ? QStringLiteral("确认启用产品吗?") : QStringLiteral("确认禁用产品吗?"));
    if (answer != QMessageBox::Yes) {
        return;
    }
    // 设置产品状态
    setPageState(PageState::Loading);
    productService_->setProductActive(
        productDto->id,
        oppositeActive,
        this,
        // 回调函数,刷新页面
        [this, oppositeActive](std::optional<AppError> error) {
            if (error.has_value()) {
                setPageState(PageState::Error);
                showOperationError(error.value());
                return;
            }
            showInformationMessage(oppositeActive ? QStringLiteral("启用产品成功!") : QStringLiteral("禁用产品成功!"));
            reloadCurrentPage(); // 重新加载
        });
}
// 更新操作按钮的状态
void ProductPage::updateActions()
{
    bool loading = (currentPageState_ == PageState::Loading);
    bool hasSelection = selectedProductDto().has_value();
    bool canCreate = productService_ && productService_->hasPermission(Permission::CreateProducts);
    bool canEdit = productService_ && productService_->hasPermission(Permission::EditProducts);
    bool canDisable = productService_ && productService_->hasPermission(Permission::DisableProducts);
    // 设置按钮的状态
    createBtn->setEnabled(!loading && canCreate);
    reloadBtn->setEnabled(!loading);
    editBtn->setEnabled(!loading && hasSelection && canEdit && currentPageState_ == PageState::Ready);
    setActiveBtn->setEnabled(!loading && hasSelection && canDisable && currentPageState_ == PageState::Ready);
}