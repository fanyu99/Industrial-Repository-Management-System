#include "InboundPage.h"
#include "qnamespace.h"
#include <QAbstractItemView>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QPointer>
#include <QPushButton>
#include <QTableView>
#include <QVBoxLayout>

InboundPage::InboundPage(InboundService* inboundService, ProductService* productService, MasterDataService* masterDataService, InboundTableModel* tableModel, QWidget* parent)
    : QWidget(parent)
    , inboundService_(inboundService)
    , productService_(productService)
    , masterDataService_(masterDataService)
    , tableModel_(tableModel)
{
    if (!inboundService_ || !masterDataService_ || !productService_ || !tableModel_) {
        showErrorMessage(QStringLiteral("入库订单页初始化失败,相关服务/模型异常"));
        return;
    }
    setWindowTitle(QStringLiteral("入库订单页"));
    if (inboundService_) {
        currentUser_ = inboundService_->currentUser();
    }
    this->setObjectName(QStringLiteral("InboundPage"));
    // 搜索栏
    // 搜索框
    keywordEdit_ = new QLineEdit(this);
    keywordEdit_->setObjectName(QStringLiteral("InboundPage_keywordEdit"));
    keywordEdit_->setPlaceholderText(QStringLiteral("请输入订单号、供应商、操作人、仓库或备注"));
    // 状态选择框
    statusCombo_ = new QComboBox(this);
    statusCombo_->setObjectName(QStringLiteral("InboundPage_statusCombo"));
    statusCombo_->setPlaceholderText(QStringLiteral("请选择状态"));
    statusCombo_->addItem(QStringLiteral("所有状态"), QVariant());
    statusCombo_->addItem(QStringLiteral("待处理"), static_cast<int>(InboundOrderStatus::Draft));
    statusCombo_->addItem(QStringLiteral("已确认"), static_cast<int>(InboundOrderStatus::Confirmed));
    statusCombo_->addItem(QStringLiteral("已取消"), static_cast<int>(InboundOrderStatus::Cancelled));
    statusCombo_->setCurrentIndex(0);
    // 仓库选择框
    warehouseCombo_ = new QComboBox(this);
    warehouseCombo_->setObjectName(QStringLiteral("InboundPage_warehouseCombo"));
    warehouseCombo_->setPlaceholderText(QStringLiteral("请选择仓库"));
    warehouseCombo_->addItem(QStringLiteral("所有仓库"), QVariant());
    warehouseCombo_->setCurrentIndex(0);
    // 搜索/清除按钮
    searchBtn = new QPushButton(QStringLiteral("搜索"), this);
    searchBtn->setObjectName(QStringLiteral("InboundPage_searchBtn"));
    clearSearchBtn = new QPushButton(QStringLiteral("清除条件"), this);
    clearSearchBtn->setObjectName(QStringLiteral("InboundPage_clearSearchBtn"));
    keywordClearBtn = new QPushButton(QStringLiteral("\u2715"), this);
    keywordClearBtn->setObjectName(QStringLiteral("InboundPage_keywordClearBtn"));
    keywordClearBtn->setFixedSize(24, 24);
    keywordClearBtn->setToolTip(QStringLiteral("清除关键词"));
    keywordClearBtn->setVisible(false);
    keywordClearBtn->setCursor(Qt::PointingHandCursor);
    // 搜索栏布局
    QHBoxLayout* searchLayout = new QHBoxLayout();
    QWidget* keywordContainer = new QWidget(this);
    keywordContainer->setObjectName(QStringLiteral("InboundPage_keywordContainer"));
    QHBoxLayout* keywordLayout = new QHBoxLayout(keywordContainer);
    keywordLayout->setContentsMargins(0, 0, 0, 0);
    keywordLayout->setSpacing(2);
    keywordLayout->addWidget(keywordEdit_);
    keywordLayout->addWidget(keywordClearBtn);
    searchLayout->addWidget(keywordContainer);
    searchLayout->addWidget(statusCombo_);
    searchLayout->addWidget(warehouseCombo_);
    searchLayout->addWidget(searchBtn);
    searchLayout->addWidget(clearSearchBtn);
    searchLayout->setObjectName(QStringLiteral("InboundPage_searchLayout"));
    searchLayout->setStretch(0, 4);
    searchLayout->setStretch(1, 2);
    searchLayout->setStretch(2, 2);
    searchLayout->setStretch(3, 1);
    searchLayout->setStretch(4, 1);
    // 搜索栏
    QWidget* searchWidget = new QWidget(this);
    searchWidget->setLayout(searchLayout);
    searchWidget->setObjectName(QStringLiteral("InboundPage_searchWidget"));
    // 搜索栏信号槽连接
    connect(searchBtn, &QPushButton::clicked, this, &InboundPage::onSearchClicked);
    connect(keywordEdit_, &QLineEdit::returnPressed, this, &InboundPage::onSearchClicked);
    connect(clearSearchBtn, &QPushButton::clicked, this, &InboundPage::onClearSearchClicked);
    connect(keywordClearBtn, &QPushButton::clicked, this, &InboundPage::onClearKeywordClicked);
    connect(keywordEdit_, &QLineEdit::textChanged, this, [this](const QString& text) {
        keywordClearBtn->setVisible(!text.trimmed().isEmpty());
    });

    // 表格视图
    emptyLabel_ = new QLabel(QStringLiteral("暂无订单"), this);
    emptyLabel_->setAlignment(Qt::AlignCenter);
    emptyLabel_->hide(); // 隐藏,仅页面空状态显示
    emptyLabel_->setObjectName(QStringLiteral("InboundPage_emptyLabel"));
    tableView_ = new QTableView(this);
    tableView_->setObjectName(QStringLiteral("InboundPage_tableView"));
    tableView_->setModel(tableModel_);
    tableView_->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableView_->setSelectionMode(QAbstractItemView::SingleSelection);
    tableView_->horizontalHeader()->setStretchLastSection(true); // 最后一列自适应宽度
    tableView_->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    tableView_->setColumnWidth(static_cast<int>(InboundTableModel::Column::OrderNoColumn), 200);
    tableView_->setColumnWidth(static_cast<int>(InboundTableModel::Column::SupplierColumn), 140);
    tableView_->setColumnWidth(static_cast<int>(InboundTableModel::Column::StatusColumn), 60);
    tableView_->setColumnWidth(static_cast<int>(InboundTableModel::Column::OperatorNameColumn), 80);
    tableView_->setColumnWidth(static_cast<int>(InboundTableModel::Column::WarehouseNameColumn), 120);
    tableView_->setColumnWidth(static_cast<int>(InboundTableModel::Column::LineCountColumn), 80);
    tableView_->setColumnWidth(static_cast<int>(InboundTableModel::Column::TotalQuantityColumn), 80);
    tableView_->setColumnWidth(static_cast<int>(InboundTableModel::Column::CreateAtColumn), 160);
    tableView_->setColumnWidth(static_cast<int>(InboundTableModel::Column::UpdateAtColumn), 160);
    tableView_->setColumnWidth(static_cast<int>(InboundTableModel::Column::ConfirmAtColumn), 160);
    selectionModel_ = tableView_->selectionModel();
    // 总布局
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    //  按钮布局
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    createBtn = new QPushButton(QStringLiteral("创建入库订单"), this);
    confirmBtn = new QPushButton(QStringLiteral("确认入库订单"), this);
    reloadBtn = new QPushButton(QStringLiteral("重新加载"), this);
    detailBtn = new QPushButton(QStringLiteral("查看详情"), this);
    createBtn->setObjectName(QStringLiteral("InboundPage_createBtn"));
    confirmBtn->setObjectName(QStringLiteral("InboundPage_confirmBtn"));
    reloadBtn->setObjectName(QStringLiteral("InboundPage_reloadBtn"));
    detailBtn->setObjectName(QStringLiteral("InboundPage_detailBtn"));

    // 分页导航
    pageNavigator_ = new PageNavigator(this);
    pageNavigator_->setObjectName(QStringLiteral("InboundPage_pageNavigator"));

    // 添加布局和组件
    mainLayout->addWidget(searchWidget);
    buttonLayout->addWidget(createBtn);
    buttonLayout->addWidget(confirmBtn);
    buttonLayout->addWidget(detailBtn);
    buttonLayout->addWidget(reloadBtn);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addWidget(tableView_);
    mainLayout->addWidget(pageNavigator_);
    // 连接信号槽
    connect(createBtn, &QPushButton::clicked, this, &InboundPage::onCreateClicked);
    connect(confirmBtn, &QPushButton::clicked, this, &InboundPage::onConfirmClicked);
    connect(reloadBtn, &QPushButton::clicked, this, &InboundPage::onReloadClicked);
    connect(detailBtn, &QPushButton::clicked, this, &InboundPage::onDetailClicked);
    connect(tableView_, &QTableView::doubleClicked, this, &InboundPage::onDetailClicked); // //双击查看详情
    if (selectionModel_) {
        connect(selectionModel_, &QItemSelectionModel::selectionChanged, this, [this]() { updateActions(); });
    }
    // 转页后重新加载页面
    connect(pageNavigator_, &PageNavigator::pageChanged, this, [this](int page) {
        currentRequest_.page = page;
        reloadCurrentPage();
    }); // 翻页,重新加载页面

    // 加载数据
    loadWarehouseSearchOptions();
    reloadCurrentPage();
    updateActions();
}
// 将错误映射为标题
QString InboundPage::errorToTitle(const AppError& error) const
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
void InboundPage::showOperationError(const AppError& error)
{
    if (!error.errorMessage.isEmpty())
        QMessageBox::warning(this, errorToTitle(error), error.errorMessage);
}
// 显示错误消息
void InboundPage::showErrorMessage(const QString& message)
{
    if (!message.isEmpty())
        QMessageBox::warning(this, QStringLiteral("错误"), message);
}
// 显示信息消息
void InboundPage::showInformationMessage(const QString& message)
{
    if (!message.isEmpty())
        QMessageBox::information(this, QStringLiteral("信息"), message);
}
// 设置当前页的状态并显示消息
void InboundPage::setPageState(InboundPageState state)
{

    currentPageState_ = state;
    if (pageNavigator_)
        pageNavigator_->setLoading(currentPageState_ == InboundPageState::Loading);
    if (emptyLabel_)
        emptyLabel_->setVisible(currentPageState_ == InboundPageState::Empty);
    updateActions(); // 更新按钮状态
}
// 重新加载当前页面(最新请求为准)
void InboundPage::reloadCurrentPage()
{

    // 请求+1
    const auto requestSeq = ++listRequestSeq_;
    // 校验
    if (!inboundService_) {
        setPageState(InboundPageState::Error);
        showErrorMessage(QStringLiteral("入库服务不可用"));
        return;
    }
    if (!tableModel_) {
        setPageState(InboundPageState::Error);
        showErrorMessage(QStringLiteral("表格模型不可用"));
        return;
    }
    // 设置状态
    setPageState(InboundPageState::Loading);
    inboundService_->listOrders(currentFilter_, currentRequest_, this, [this, requestSeq](const InboundPageResult& result) {
        if (requestSeq != listRequestSeq_) // 以最新请求为准
            return;
        // 校验
        if (!result.success) {
            setPageState(InboundPageState::Error);
            if (result.error.has_value())
                showOperationError(result.error.value());
            else
                showErrorMessage(QStringLiteral("未知错误"));

            return;
        }

        // 设置页面
        tableModel_->setPage(result.page);
        if (result.page.items.isEmpty()) {
            setPageState(InboundPageState::Empty);
            pageNavigator_->updatePageInfo(tableModel_->page(), tableModel_->totalPages());
            return;
        }
        setPageState(InboundPageState::Ready);
        pageNavigator_->updatePageInfo(tableModel_->page(), tableModel_->totalPages());
    });
}
// 获取选中的订单
std::optional<InboundOrderListItemDto> InboundPage::selectedInboundDto() const
{
    if (!tableView_)
        return std::nullopt;
    if (!tableModel_)
        return std::nullopt;
    if (!selectionModel_)
        return std::nullopt;
    const QModelIndexList selectedRows = selectionModel_->selectedRows();
    if (selectedRows.isEmpty())
        return std::nullopt;
    const int row = selectedRows.first().row();
    const InboundOrderListItemDto order = tableModel_->itemAt(row);
    if (order.id == 0)
        return std::nullopt;
    return order;
}
// 加载仓库数据到入库编辑对话框
void InboundPage::loadInboundDialogWarehouseOptions(InboundEditDialog& dialog, bool isActiveOnly)
{
    if (!masterDataService_) {
        showErrorMessage(QStringLiteral("基础数据(仓库)服务不可用"));
        return;
    }

    const quint64 warehouseReloadId = dialog.beginWarehouseReload();
    QPointer<InboundEditDialog> dialogPtr(&dialog);

    masterDataService_->listWarehouses(
        &dialog,
        isActiveOnly,
        [this, dialogPtr, warehouseReloadId](const WarehouseListResult& result) {
            if (!dialogPtr || !dialogPtr->isCurrentWarehouseReload(warehouseReloadId)) {
                return;
            }

            if (!result.success) {
                dialogPtr->finishWarehouseReload(warehouseReloadId, false);
                showErrorMessage(QStringLiteral("加载仓库数据失败"));
                return;
            }

            QString errorMessage;
            for (const auto& warehouse : result.warehouses.value()) {
                if (!dialogPtr->addWarehouse(warehouse.name, warehouse.id, errorMessage)) {
                    dialogPtr->finishWarehouseReload(warehouseReloadId, false);
                    showErrorMessage(errorMessage);
                    return;
                }
            }

            dialogPtr->finishWarehouseReload(warehouseReloadId, true);
        });
}

// 加载产品选项数据到入库编辑对话框
void InboundPage::loadInboundDialogProductOptions(InboundEditDialog& dialog, bool isActiveOnly)
{
    if (!productService_) {
        showErrorMessage(QStringLiteral("产品服务不可用"));
        return;
    }

    const quint64 productReloadId = dialog.beginProductReload();
    QPointer<InboundEditDialog> dialogPtr(&dialog);

    productService_->listProductOptions(
        &dialog,
        [this, dialogPtr, productReloadId](const ProductOptionsResult& result) {
            if (!dialogPtr || !dialogPtr->isCurrentProductReload(productReloadId)) {
                return;
            }

            if (!result.success) {
                dialogPtr->finishProductReload(productReloadId, false);
                showErrorMessage(QStringLiteral("加载产品选项失败"));
                return;
            }

            dialogPtr->setProductOptions(result.productOptions);
            dialogPtr->finishProductReload(productReloadId, true);
        },
        isActiveOnly);
}

// 总API:加载仓库与产品数据到入库编辑对话框
void InboundPage::loadInboundDialogOptions(InboundEditDialog& dialog, bool isActiveOnly)
{
    loadInboundDialogWarehouseOptions(dialog, isActiveOnly);
    loadInboundDialogProductOptions(dialog, isActiveOnly);
}
// 槽函数

// 创建草稿订单
void InboundPage::onCreateClicked()
{
    // 校验
    if (!inboundService_) {
        showErrorMessage(QStringLiteral("入库服务不可用"));
        return;
    }
    if (!tableModel_) {
        showErrorMessage(QStringLiteral("表格模型不可用"));
        return;
    }
    if (!inboundService_->hasPermission(Permission::CreateInboundOrders)) {
        showOperationError(AppError::permissionDenied());
        return;
    }
    // 创建编辑框
    InboundEditDialog dialog(InboundEditMode::Create, this);
    // 设置操作人
    auto user = inboundService_->currentUser();
    if (!user.has_value()) {
        showErrorMessage(QStringLiteral("当前用户不存在"));
        return;
    }
    dialog.setOperatorName(user->userName);
    // 加载仓库与产品选项数据
    connect(&dialog, &InboundEditDialog::masterdataActiveOnlyChanged, this, [this, &dialog](bool activeOnly) {
        loadInboundDialogWarehouseOptions(dialog, activeOnly);
    });

    loadInboundDialogWarehouseOptions(dialog, dialog.isMasterdataActiveOnly()); // 加载仓库数据
    loadInboundDialogProductOptions(dialog, true);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    QString errorMessage;
    if (!dialog.validateInput(errorMessage)) {
        showErrorMessage(errorMessage);
        return;
    }
    const CreateInboundOrderRequest request = dialog.createRequest(); // 获取创建订单请求
    setPageState(InboundPageState::Loading); // 设置加载
    inboundService_->createDraft(request, this, [this](const InboundOperationResult& result) {
        if (!result.success) {
            setPageState(InboundPageState::Error);
            if (result.error.has_value()) {
                showOperationError(result.error.value());
            } else {
                showErrorMessage(QStringLiteral("创建订单失败,未知错误"));
            }
            return;
        }
        showInformationMessage(QStringLiteral("创建入库订单成功"));
        reloadCurrentPage();
    });
}
// 确认订单
void InboundPage::onConfirmClicked()
{
    // 校验
    if (!inboundService_) {
        showErrorMessage(QStringLiteral("入库服务不可用"));
        return;
    }
    if (!tableModel_) {
        showErrorMessage(QStringLiteral("表格模型不可用"));
        return;
    }
    if (!inboundService_->hasPermission(Permission::ConfirmInboundOrders)) {
        showOperationError(AppError::permissionDenied());
        return;
    }
    if (!selectionModel_) {
        showErrorMessage(QStringLiteral("选择模型不可用"));
        return;
    }
    if (!tableView_) {
        showErrorMessage(QStringLiteral("表格视图不可用"));
        return;
    }
    std::optional<InboundOrderListItemDto> orderDto = selectedInboundDto();
    if (!orderDto.has_value()) {
        showInformationMessage(QStringLiteral("请选择订单"));
        return;
    }
    // 二次确认入库
    const auto answer = QMessageBox::question(
        this,
        QStringLiteral("确认入库"),
        QStringLiteral("确认入库订单 %1 吗?\n供应商: %2\n仓库: %3\n订单号:%4").arg(orderDto.value().orderNo).arg(orderDto.value().supplier).arg(orderDto.value().warehouseName).arg(orderDto.value().orderNo));
    if (answer != QMessageBox::Yes)
        return;
    setPageState(InboundPageState::Loading); // 设置加载状态,防止重复点击
    // 确认订单
    inboundService_->confirmOrder(orderDto.value().id, this, [this](const InboundOperationResult& result) {
        if (!result.success) {
            setPageState(InboundPageState::Error);
            if (result.error.has_value())
                showOperationError(result.error.value());
            else
                showErrorMessage(QStringLiteral("未知错误"));
            return;
        }
        showInformationMessage(QStringLiteral("确认成功"));
        reloadCurrentPage();
    });
}
// 重新加载
void InboundPage::onReloadClicked()
{
    reloadCurrentPage();
}
// 查看详情
void InboundPage::onDetailClicked()
{
    if (!inboundService_) {
        showErrorMessage(QStringLiteral("入库服务不可用"));
        return;
    }
    // 校验当前是否选中订单
    std::optional<InboundOrderListItemDto> orderDto = selectedInboundDto();
    if (!orderDto.has_value()) {
        showInformationMessage(QStringLiteral("请选择订单"));
        return;
    }
    // 校验权限
    if (!inboundService_->hasPermission(Permission::ViewInboundOrders)) {
        showOperationError(AppError::permissionDenied());
        return;
    }
    if (currentPageState_ != InboundPageState::Ready)
        return;
    // service进行查询获取订单详情
    const quint64 requestSeq = ++detailRequestSeq_;
    inboundService_->getOrderDetail(orderDto.value().id, this, [this, requestSeq](const InboundOrderDetailResult& result) {
        if (detailRequestSeq_ != requestSeq)
            return;
        if (!result.success) {
            showErrorMessage(QStringLiteral("查询订单详情失败: %1").arg(result.error.has_value() ? result.error.value().errorMessage : QStringLiteral("未知错误")));
            return;
        }
        if (result.error.has_value()) {
            showErrorMessage(QStringLiteral("查询订单详情失败: %1").arg(result.error.value().errorMessage));
            return;
        }
        if (!result.orderDetail.has_value()) {
            showErrorMessage(QStringLiteral("查询订单详情失败: 未返回订单详情"));
            return;
        }
        const auto& orderDetail = result.orderDetail.value();
        // 显示订单详情
        InboundOrderDetailDialog* dialog = new InboundOrderDetailDialog(orderDetail, this); // open非阻塞,需要手动创建内存(堆)
        dialog->setAttribute(Qt::WA_DeleteOnClose); // 关闭自动销毁
        dialog->open();
    });
}

// 更新操作按钮的状态
void InboundPage::updateActions()
{
    const auto selectedDto = selectedInboundDto();
    bool loading = (currentPageState_ == InboundPageState::Loading);
    bool isError = (currentPageState_ == InboundPageState::Error);
    bool hasSelection = selectedDto.has_value();
    bool canCreate = inboundService_ && inboundService_->hasPermission(Permission::CreateInboundOrders);
    bool canConfirm = hasSelection && inboundService_ && inboundService_->hasPermission(Permission::ConfirmInboundOrders) && selectedDto.value().status == InboundOrderStatus::Draft;
    bool canViewDetail = hasSelection && inboundService_ && inboundService_->hasPermission(Permission::ViewInboundOrders) && currentPageState_ == InboundPageState::Ready;
    // 设置按钮状态
    tableView_->setEnabled(!loading && !isError);
    if (createBtn)
        createBtn->setEnabled(!loading && canCreate);
    if (confirmBtn)
        confirmBtn->setEnabled(!loading && canConfirm && hasSelection && currentPageState_ == InboundPageState::Ready);
    if (reloadBtn)
        reloadBtn->setEnabled(!loading);
    if (searchBtn)
        searchBtn->setEnabled(!loading);
    if (clearSearchBtn)
        clearSearchBtn->setEnabled(!loading);
    if (detailBtn)
        detailBtn->setEnabled(!loading && canViewDetail);
    if (keywordEdit_)
        keywordEdit_->setEnabled(!loading);
    if (statusCombo_)
        statusCombo_->setEnabled(!loading);
    if (warehouseCombo_)
        warehouseCombo_->setEnabled(!loading);
}
// 从控件读取筛选器
InboundOrderFilter InboundPage::readFilterFromControls() const
{
    InboundOrderFilter filter;
    if (!keywordEdit_ || !statusCombo_ || !warehouseCombo_)
        return filter;
    // 关键字
    filter.keyword = keywordEdit_->text().trimmed();
    // 状态
    const QVariant statusData = statusCombo_->currentData();
    if (statusData.isValid() && !statusData.isNull()) {
        filter.status = static_cast<InboundOrderStatus>(statusData.toInt());
    }
    // 仓库
    const QVariant warehouseData = warehouseCombo_->currentData();
    bool ok = false;
    const quint32 warehouseId = warehouseData.toUInt(&ok);
    if (ok && warehouseId > 0)
        filter.warehouseId = warehouseId;
    return filter;
}
// 重置筛选器控件
void InboundPage::resetFilterControls()
{
    keywordEdit_->clear();
    statusCombo_->setCurrentIndex(0);
    warehouseCombo_->setCurrentIndex(0);
}
// 搜索功能
// 搜索
void InboundPage::onSearchClicked()
{
    currentFilter_ = readFilterFromControls();
    currentRequest_.page = 1;
    reloadCurrentPage();
}
// 清除搜索
void InboundPage::onClearSearchClicked()
{
    resetFilterControls();
    currentFilter_ = {};
    currentRequest_.page = 1;
    reloadCurrentPage();
}

void InboundPage::onClearKeywordClicked()
{
    keywordEdit_->clear();
    keywordEdit_->setFocus();
}
// 加载仓库搜索选项
void InboundPage::loadWarehouseSearchOptions()
{
    if (!masterDataService_) {
        showErrorMessage(QStringLiteral("主数据服务不可用"));
        return;
    }
    if (!warehouseCombo_)
        return;
    warehouseCombo_->setEnabled(false);
    // 移除除了"所有仓库"的所有选项
    while (warehouseCombo_->count() > 1) {
        warehouseCombo_->removeItem(1);
    }
    // 加载仓库
    masterDataService_->listWarehouses(
        this,
        false,
        [this](const WarehouseListResult& result) {
            if (!result.success) {
                showErrorMessage(QStringLiteral("加载仓库数据失败"));
                return;
            }
            if (!result.warehouses.has_value()) {
                showErrorMessage(QStringLiteral("未返回有效仓库数据"));
                return;
            }

            if (!warehouseCombo_)
                return;

            // 添加仓库
            for (const auto warehouse : result.warehouses.value()) {
                const QString warehouseName = warehouse.name;
                const quint32 warehouseId = warehouse.id;
                if (warehouseId == 0)
                    continue;
                if (warehouseName.trimmed().isEmpty())
                    continue;
                warehouseCombo_->addItem(warehouseName, warehouseId);
            }
            warehouseCombo_->setEnabled(true);
        });
}