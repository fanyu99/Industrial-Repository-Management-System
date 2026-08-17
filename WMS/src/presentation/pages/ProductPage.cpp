#include "ProductPage.h"
#include <QAbstractItemModel>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QPointer>
#include <QStringLiteral>
#include <QVBoxLayout>
#include <QWidget>
#include <memory>
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
    emptyLabel_ = new QLabel(QStringLiteral("暂无产品"), this);
    emptyLabel_->setAlignment(Qt::AlignCenter);
    emptyLabel_->hide(); // 隐藏,仅页面空状态显示
    emptyLabel_->setObjectName(QStringLiteral("ProductPage_emptyLabel"));
    tableView_ = new QTableView(this);
    tableView_->setObjectName(QStringLiteral("ProductPage_tableView"));
    tableView_->setModel(tableModel_);
    tableView_->setSelectionBehavior(QAbstractItemView::SelectRows); // 整行选
    tableView_->setSelectionMode(QAbstractItemView::SingleSelection); // 单选
    tableView_->horizontalHeader()->setStretchLastSection(true);
    tableView_->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    tableView_->setColumnWidth(static_cast<int>(ProductTableModel::Column::CodeColumn), 100);
    tableView_->setColumnWidth(static_cast<int>(ProductTableModel::Column::NameColumn), 120);
    tableView_->setColumnWidth(static_cast<int>(ProductTableModel::Column::CategoryColumn), 80);
    tableView_->setColumnWidth(static_cast<int>(ProductTableModel::Column::UnitColumn), 60);
    tableView_->setColumnWidth(static_cast<int>(ProductTableModel::Column::SpecificationColumn), 150);
    tableView_->setColumnWidth(static_cast<int>(ProductTableModel::Column::SafetyStockColumn), 80);
    tableView_->setColumnWidth(static_cast<int>(ProductTableModel::Column::ActiveColumn), 60);
    selectionModel_ = tableView_->selectionModel();
    // 搜索栏
    keywordLineEdit_ = new QLineEdit(this);
    keywordLineEdit_->setPlaceholderText(QStringLiteral("请输入产品名称/编码/规格描述"));
    keywordLineEdit_->setObjectName(QStringLiteral("ProductPage_keywordLineEdit"));
    clearKeywordBtn_ = new QPushButton(QStringLiteral("\u2715"), this);
    clearKeywordBtn_->setObjectName(QStringLiteral("ProductPage_clearKeywordBtn"));
    clearKeywordBtn_->setFixedSize(24, 24);
    clearKeywordBtn_->setToolTip(QStringLiteral("清除关键词"));
    clearKeywordBtn_->setVisible(false);
    clearKeywordBtn_->setCursor(Qt::PointingHandCursor);
    categoryComboBox_ = new QComboBox(this);
    categoryComboBox_->setObjectName(QStringLiteral("ProductPage_categoryComboBox"));
    categoryComboBox_->addItem(QStringLiteral("全部"));
    categoryComboBox_->setPlaceholderText(QStringLiteral("请选择分类"));
    categoryComboBox_->setCurrentIndex(0);
    searchBtn_ = new QPushButton(QStringLiteral("搜索"), this);
    searchBtn_->setObjectName(QStringLiteral("ProductPage_searchBtn"));
    clearBtn_ = new QPushButton(QStringLiteral("清除条件"), this);
    clearBtn_->setObjectName(QStringLiteral("ProductPage_clearBtn"));
    activeOnlyCheckBox_ = new QCheckBox(QStringLiteral("仅显示启用"), this);
    activeOnlyCheckBox_->setObjectName(QStringLiteral("ProductPage_activeOnlyCheckBox"));
    activeOnlyCheckBox_->setToolTip(QStringLiteral("仅显示已启用的产品"));
    activeOnlyCheckBox_->setChecked(false); // 默认不勾选（显示所有产品）
    // 连接信号
    connect(searchBtn_, &QPushButton::clicked, this, &ProductPage::onSearchClicked);
    connect(keywordLineEdit_, &QLineEdit::returnPressed, this, &ProductPage::onSearchClicked);
    connect(clearBtn_, &QPushButton::clicked, this, &ProductPage::onClearClicked);
    connect(clearKeywordBtn_, &QPushButton::clicked, this, &ProductPage::onClearKeywordClicked);
    connect(keywordLineEdit_, &QLineEdit::textChanged, this, [this](const QString& text) {
        clearKeywordBtn_->setVisible(!text.trimmed().isEmpty());
    });

    // 搜索栏布局
    QHBoxLayout* searchLayout = new QHBoxLayout();
    QWidget* keywordContainer = new QWidget(this);
    keywordContainer->setObjectName(QStringLiteral("ProductPage_keywordContainer"));
    QHBoxLayout* keywordLayout = new QHBoxLayout(keywordContainer);
    keywordLayout->setContentsMargins(0, 0, 0, 0);
    keywordLayout->setSpacing(2);
    keywordLayout->addWidget(keywordLineEdit_);
    keywordLayout->addWidget(clearKeywordBtn_);
    searchLayout->addWidget(keywordContainer);
    searchLayout->addWidget(categoryComboBox_);
    searchLayout->addWidget(activeOnlyCheckBox_);
    searchLayout->addWidget(searchBtn_);
    searchLayout->addWidget(clearBtn_);
    searchLayout->setObjectName(QStringLiteral("ProductPage_searchLayout"));
    searchLayout->setStretch(0, 4);
    searchLayout->setStretch(1, 2);
    searchLayout->setStretch(2, 1);
    searchLayout->setStretch(3, 1);
    searchLayout->setStretch(4, 1);

    // 搜索栏
    QWidget* searchWidget = new QWidget(this);
    searchWidget->setLayout(searchLayout);
    searchWidget->setObjectName(QStringLiteral("ProductPage_searchWidget"));

    // 总布局
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // 添加搜索栏到布局
    mainLayout->addWidget(searchWidget);

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
    // 分页导航
    pageNavigator_ = new PageNavigator(this);
    pageNavigator_->setObjectName(QStringLiteral("ProductPage_pageNavigator"));
    mainLayout->addWidget(pageNavigator_);
    // 连接信号槽
    connect(createBtn, &QPushButton::clicked, this, &ProductPage::onCreateClicked);
    connect(editBtn, &QPushButton::clicked, this, &ProductPage::onEditClicked);
    connect(setActiveBtn, &QPushButton::clicked, this, &ProductPage::onSetActiveClicked);
    connect(reloadBtn, &QPushButton::clicked, this, &ProductPage::reloadCurrentPage);
    if (selectionModel_)
        connect(selectionModel_, &QItemSelectionModel::selectionChanged, this, [this]() { updateActions(); }); // 根据选中修改按钮状态
    // 转页后重新加载页面
    connect(pageNavigator_, &PageNavigator::pageChanged, this, [this](int page) {
        currentRequest_.page = page;
        reloadCurrentPage();
    });

    // 加载分类搜索选项
    loadCategorySearchOptions();

    reloadCurrentPage();
    updateActions();
}
// 设置筛选器
void ProductPage::setCurrentFilter(const ProductFilter& filter)
{
    currentFilter_ = filter;
    reloadCurrentPage();
}
// 获取当前分页请求参数
PageRequest ProductPage::currentPageRequest() const
{
    return currentRequest_;
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
void ProductPage::setPageState(ProductPageState state)
{
    currentPageState_ = state;
    if (pageNavigator_)
        pageNavigator_->setLoading(currentPageState_ == ProductPageState::Loading);
    if (emptyLabel_)
        emptyLabel_->setVisible(currentPageState_ == ProductPageState::Empty);
    updateActions(); // 更新按钮状态
}
// 重新加载当前页面(以最新的请求为主)
void ProductPage::reloadCurrentPage()
{

    const auto requestSeq = ++listRequestSeq_; // 当前的请求序列号
    if (!productService_) {
        setPageState(ProductPageState::Error);
        showErrorMessage(QStringLiteral("产品服务不可用"));
        return;
    }
    if (!tableModel_) {
        setPageState(ProductPageState::Error);
        showErrorMessage(QStringLiteral("表格模型不可用"));
        return;
    }
    setPageState(ProductPageState::Loading);

    loadCategorySearchOptions();

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
                setPageState(ProductPageState::Error);
                if (result.error.has_value())
                    showOperationError(result.error.value());
                else
                    showErrorMessage(QStringLiteral("未知错误!"));
                return;
            }
            // 设置页面
            tableModel_->setPage(result.page);
            if (result.page.items.isEmpty()) {
                setPageState(ProductPageState::Empty);
                pageNavigator_->updatePageInfo(tableModel_->page(), tableModel_->totalPages());
                return;
            }
            setPageState(ProductPageState::Ready);
            pageNavigator_->updatePageInfo(tableModel_->page(), tableModel_->totalPages());
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
        showErrorMessage(QStringLiteral("基础数据(分类/单位)服务不可用"));
        return;
    }
    // 设置加载中,禁用分类/单位,等待加载完成

    auto pending = std::make_shared<int>(2); // 待完成
    auto failed = std::make_shared<bool>(false); // 失败
    const quint64 reloadId = dialog.beginMasterDataReload(); // 序列号
    QPointer<ProductEditDialog> dialogPtr(&dialog);
    // 完成一个加载任务
    auto finishOne = [this, dialogPtr, pending, failed, reloadId]() {
        // 以最新的序列号为准
        if (!dialogPtr || !dialogPtr->isCurrentMasterDataReload(reloadId))
            return;
        --(*pending);
        if (*pending != 0)
            return;
        if (*failed) {
            dialogPtr->finishMasterDataReload(reloadId, false);
            showErrorMessage(QStringLiteral("基础数据(分类/单位)加载失败!请重试"));
            return;
        }
        dialogPtr->setOptionsLoading(false);
        dialogPtr->finishMasterDataReload(reloadId, true);
    };
    // 加载分类/单位
    masterDataService_->listCategories(
        &dialog,
        activeOnly,
        [this, dialogPtr, failed, finishOne](const CategoryListResult& result) {
            if (!dialogPtr)
                return;
            if (!result.success) {
                *failed = true;
                finishOne();
                return;
            }
            QString errorMessage;
            for (const auto& category : result.categories.value()) {
                if (!dialogPtr->addCategory(category.name, category.id, errorMessage)) {
                    *failed = true;
                    break;
                }
            }
            finishOne();
        });
    masterDataService_->listUnits(
        &dialog,
        activeOnly,
        [this, dialogPtr, failed, finishOne](const UnitListResult& result) {
            if (!dialogPtr)
                return;
            if (!result.success) {
                *failed = true;
                finishOne();
                return;
            }
            QString errorMessage;
            for (const auto& unit : result.units.value()) {
                if (!dialogPtr->addUnit(unit.name, unit.id, errorMessage)) {
                    *failed = true;
                    break;
                }
            }
            finishOne();
        });
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
    connect(&dialog, &ProductEditDialog::masterdataActiveOnlyChanged, &dialog, [this, &dialog](bool activeOnly) { loadProductDialogOptions(dialog, activeOnly); });
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
    setPageState(ProductPageState::Loading);
    productService_->createProduct(
        product,
        this,
        // 回调函数,刷新页面
        [this](const ProductOperationResult& result) {
            if (!result.success) {
                setPageState(ProductPageState::Error);
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
        showErrorMessage(QStringLiteral("选择模型不可用!"));
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

    dialog.setProduct(productDto.value()); // 先进行设置产品,使分类/单位能够正确回显
    connect(&dialog, &ProductEditDialog::masterdataActiveOnlyChanged, &dialog, [this, &dialog](bool activeOnly) { loadProductDialogOptions(dialog, activeOnly); }); // 信号槽: 激活状态改变,重新加载分类/单位
    // 加载分类/单位
    loadProductDialogOptions(dialog, dialog.isMasterdataActiveOnly());
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
    setPageState(ProductPageState::Loading);
    productService_->updateProduct(
        product,
        this,
        // 回调函数,刷新页面
        [this](const ProductOperationResult& result) {
            if (!result.success) {
                setPageState(ProductPageState::Error);
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
    setPageState(ProductPageState::Loading);
    productService_->setProductActive(
        productDto->id,
        oppositeActive,
        this,
        // 回调函数,刷新页面
        [this, oppositeActive](std::optional<AppError> error) {
            if (error.has_value()) {
                setPageState(ProductPageState::Error);
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
    const bool loading = currentPageState_ == ProductPageState::Loading || categoryOptionsLoading_;
    bool isError = (currentPageState_ == ProductPageState::Error);
    bool hasSelection = selectedProductDto().has_value();
    bool canCreate = productService_ && productService_->hasPermission(Permission::CreateProducts);
    bool canEdit = productService_ && productService_->hasPermission(Permission::EditProducts);
    bool canDisable = productService_ && productService_->hasPermission(Permission::DisableProducts);
    // 设置按钮的状态
    if(tableView_)tableView_->setEnabled(!loading && !isError);
    if(createBtn)createBtn->setEnabled(!loading && canCreate);
    if(reloadBtn)reloadBtn->setEnabled(!loading);
    if(editBtn)editBtn->setEnabled(!loading && hasSelection && canEdit && currentPageState_ == ProductPageState::Ready);
    if(setActiveBtn)setActiveBtn->setEnabled(!loading && hasSelection && canDisable && currentPageState_ == ProductPageState::Ready);

    // 设置搜索栏控件状态
    if (keywordLineEdit_)
        keywordLineEdit_->setEnabled(!loading);
    if (categoryComboBox_)
        categoryComboBox_->setEnabled(!loading);
    if (activeOnlyCheckBox_)
        activeOnlyCheckBox_->setEnabled(!loading);
    if (searchBtn_)
        searchBtn_->setEnabled(!loading);
    if (clearBtn_)
        clearBtn_->setEnabled(!loading);
}

// 从控件读取筛选器
ProductFilter ProductPage::readFilterFromControls() const
{
    ProductFilter filter;
    if (!keywordLineEdit_ || !categoryComboBox_)
        return filter;

    // 关键字（编号/名称/规格）
    filter.keyword = keywordLineEdit_->text().trimmed();

    // 分类
    const QVariant categoryData = categoryComboBox_->currentData();
    bool ok = false;
    const quint32 categoryId = categoryData.toUInt(&ok);
    if (ok && categoryId > 0)
        filter.categoryId = categoryId;

    // 激活状态（仅显示启用的产品）
    if (activeOnlyCheckBox_ && activeOnlyCheckBox_->isChecked()) {
        filter.active = true;
    } else {
        filter.active = std::nullopt; // 不筛选，显示所有状态的产品
    }

    return filter;
}

// 重置筛选器控件
void ProductPage::resetFilterControls()
{
    if (keywordLineEdit_)
        keywordLineEdit_->clear();
    if (categoryComboBox_)
        categoryComboBox_->setCurrentIndex(0);
    if (activeOnlyCheckBox_)
        activeOnlyCheckBox_->setChecked(false); // 重置为不勾选（显示所有产品）
}

// 搜索
void ProductPage::onSearchClicked()
{
    currentFilter_ = readFilterFromControls();
    currentRequest_.page = 1;
    reloadCurrentPage();
}

// 清除搜索
void ProductPage::onClearClicked()
{
    resetFilterControls();
    currentFilter_ = {};
    currentRequest_.page = 1;
    reloadCurrentPage();
}

// 清除关键词
void ProductPage::onClearKeywordClicked()
{
    if (keywordLineEdit_) {
        keywordLineEdit_->clear();
        keywordLineEdit_->setFocus();
    }
}

// 加载分类搜索选项
void ProductPage::loadCategorySearchOptions()
{
    if (!masterDataService_) {
        showErrorMessage(QStringLiteral("基础数据服务不可用"));
        return;
    }
    if (!categoryComboBox_)
        return;

    categoryOptionsLoading_ = true;
    updateActions();

    // 移除除了"全部"的所有选项
    while (categoryComboBox_->count() > 1) {
        categoryComboBox_->removeItem(1);
    }

    // 加载分类（包括停用的分类，因为搜索时可能需要查看停用分类的产品）
    masterDataService_->listCategories(
        this,
        false,
        [this](const CategoryListResult& result) {
            if (!result.success) {
                showErrorMessage(QStringLiteral("加载分类数据失败"));
                categoryOptionsLoading_ = false;
                updateActions();
                return;
            }
            if (!result.categories.has_value()) {
                showErrorMessage(QStringLiteral("未返回有效分类数据"));
                categoryOptionsLoading_ = false;
                updateActions();
                return;
            }

            if (!categoryComboBox_) {
                categoryOptionsLoading_ = false;
                updateActions();
                return;
            }

            // 添加分类选项
            for (const auto& category : result.categories.value()) {
                const QString categoryName = category.name;
                const quint32 categoryId = category.id;
                if (categoryId == 0)
                    continue;
                if (categoryName.trimmed().isEmpty())
                    continue;
                categoryComboBox_->addItem(categoryName, categoryId);
            }

            categoryOptionsLoading_ = false;
            updateActions();
        });
}