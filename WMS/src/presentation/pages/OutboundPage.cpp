#include "OutboundPage.h"
#include "OutboundOrderDetailDialog.h"
#include "qnamespace.h"
#include <QAbstractItemView>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QMessageBox>
#include <QPointer>
#include <QPushButton>
#include <QTableView>
#include <QVBoxLayout>
#include <QVariant>
OutboundPage::OutboundPage(OutboundService* outboundService, ProductService* productService, MasterDataService* masterDataService, OutboundTableModel* tableModel, QWidget* parent)
    : QWidget(parent)
    , outboundService_(outboundService)
    , productService_(productService)
    , masterDataService_(masterDataService)
    , tableModel_(tableModel)
{
    if (!outboundService_ || !masterDataService_ || !productService_ || !tableModel_) {
        showErrorMessage(QStringLiteral("出库订单页初始化失败,相关服务/模型异常"));
        return;
    }
    setWindowTitle(QStringLiteral("出库订单页"));
    if (outboundService_) {
        currentUser_ = outboundService_->currentUser();
    }
    this->setObjectName(QStringLiteral("OutboundPage"));
    // 搜索栏
    keywordEdit_ = new QLineEdit(this);
    keywordEdit_->setObjectName(QStringLiteral("OutboundPage_keywordEdit"));
    keywordEdit_->setPlaceholderText(QStringLiteral("请输入订单号、接收人、操作人、仓库或备注"));

    statusCombo_ = new QComboBox(this);
    statusCombo_->setObjectName(QStringLiteral("OutboundPage_statusCombo"));
    statusCombo_->setPlaceholderText(QStringLiteral("请选择状态"));
    statusCombo_->addItem(QStringLiteral("所有状态"), QVariant());
    statusCombo_->addItem(QStringLiteral("草稿"), static_cast<int>(OutboundOrderStatus::Draft));
    statusCombo_->addItem(QStringLiteral("已确认"), static_cast<int>(OutboundOrderStatus::Confirmed));
    statusCombo_->addItem(QStringLiteral("已取消"), static_cast<int>(OutboundOrderStatus::Cancelled));
    statusCombo_->setCurrentIndex(0);

    warehouseCombo_ = new QComboBox(this);
    warehouseCombo_->setObjectName(QStringLiteral("OutboundPage_warehouseCombo"));
    warehouseCombo_->setPlaceholderText(QStringLiteral("请选择仓库"));
    warehouseCombo_->addItem(QStringLiteral("所有仓库"), QVariant());
    warehouseCombo_->setCurrentIndex(0);

    searchBtn = new QPushButton(QStringLiteral("搜索"), this);
    searchBtn->setObjectName(QStringLiteral("OutboundPage_searchBtn"));
    clearSearchBtn = new QPushButton(QStringLiteral("清除条件"), this);
    clearSearchBtn->setObjectName(QStringLiteral("OutboundPage_clearSearchBtn"));

    QHBoxLayout* searchLayout = new QHBoxLayout();
    searchLayout->addWidget(keywordEdit_);
    searchLayout->addWidget(statusCombo_);
    searchLayout->addWidget(warehouseCombo_);
    searchLayout->addWidget(searchBtn);
    searchLayout->addWidget(clearSearchBtn);
    searchLayout->addStretch(1);
    searchLayout->setObjectName(QStringLiteral("OutboundPage_searchLayout"));

    QWidget* searchWidget = new QWidget(this);
    searchWidget->setLayout(searchLayout);
    searchWidget->setObjectName(QStringLiteral("OutboundPage_searchWidget"));

    connect(searchBtn, &QPushButton::clicked, this, &OutboundPage::onSearchClicked);
    connect(clearSearchBtn, &QPushButton::clicked, this, &OutboundPage::onClearSearchClicked);

    // 表格视图
    tableView_ = new QTableView(this);
    tableView_->setObjectName(QStringLiteral("OutboundPage_tableView"));
    tableView_->setModel(tableModel_);
    tableView_->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableView_->setSelectionMode(QAbstractItemView::SingleSelection);
    tableView_->horizontalHeader()->setStretchLastSection(true); // 最后一列自适应宽度
    tableView_->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    tableView_->setColumnWidth(static_cast<int>(OutboundTableModel::Column::OrderNoColumn), 200);
    tableView_->setColumnWidth(static_cast<int>(OutboundTableModel::Column::RecipientColumn), 140);
    tableView_->setColumnWidth(static_cast<int>(OutboundTableModel::Column::StatusColumn), 60);
    tableView_->setColumnWidth(static_cast<int>(OutboundTableModel::Column::OperatorNameColumn), 80);
    tableView_->setColumnWidth(static_cast<int>(OutboundTableModel::Column::WarehouseNameColumn), 120);
    tableView_->setColumnWidth(static_cast<int>(OutboundTableModel::Column::LineCountColumn), 80);
    tableView_->setColumnWidth(static_cast<int>(OutboundTableModel::Column::TotalQuantityColumn), 80);
    tableView_->setColumnWidth(static_cast<int>(OutboundTableModel::Column::CreateAtColumn), 160);
    tableView_->setColumnWidth(static_cast<int>(OutboundTableModel::Column::UpdateAtColumn), 160);
    tableView_->setColumnWidth(static_cast<int>(OutboundTableModel::Column::ConfirmAtColumn), 160);
    selectionModel_ = tableView_->selectionModel();
    // 总布局
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    //  按钮布局
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    createBtn = new QPushButton(QStringLiteral("创建出库订单"), this);
    confirmBtn = new QPushButton(QStringLiteral("确认出库订单"), this);
    viewDetailBtn = new QPushButton(QStringLiteral("查看详情"), this);
    reloadBtn = new QPushButton(QStringLiteral("重新加载"), this);
    createBtn->setObjectName(QStringLiteral("OutboundPage_createBtn"));
    confirmBtn->setObjectName(QStringLiteral("OutboundPage_confirmBtn"));
    viewDetailBtn->setObjectName(QStringLiteral("OutboundPage_viewDetailBtn"));
    reloadBtn->setObjectName(QStringLiteral("OutboundPage_reloadBtn"));
    // 分页导航
    pageNavigator_ = new PageNavigator(this);
    pageNavigator_->setObjectName(QStringLiteral("OutboundPage_pageNavigator"));

    // 添加布局和组件
    mainLayout->addWidget(searchWidget);
    buttonLayout->addWidget(createBtn);
    buttonLayout->addWidget(confirmBtn);
    buttonLayout->addWidget(viewDetailBtn);
    buttonLayout->addWidget(reloadBtn);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addWidget(tableView_);
    mainLayout->addWidget(pageNavigator_);
    // 连接信号槽
    connect(createBtn, &QPushButton::clicked, this, &OutboundPage::onCreateClicked);
    connect(confirmBtn, &QPushButton::clicked, this, &OutboundPage::onConfirmClicked);
    connect(viewDetailBtn, &QPushButton::clicked, this, &OutboundPage::onViewDetailClicked);
    connect(tableView_, &QTableView::doubleClicked, this, &OutboundPage::onViewDetailClicked);
    connect(reloadBtn, &QPushButton::clicked, this, &OutboundPage::onReloadClicked);
    if (selectionModel_) {
        connect(selectionModel_, &QItemSelectionModel::selectionChanged, this, [this]() { updateActions(); });
    }
    // 转页后重新加载页面
    connect(pageNavigator_, &PageNavigator::pageChanged, this, [this](int page) {
        currentRequest_.page = page;
        reloadCurrentPage();
    }); // 翻页,重新加载页面

    loadWarehouseSearchOptions();
    reloadCurrentPage();
    updateActions();
}
// 将错误映射为标题
QString OutboundPage::errorToTitle(const AppError& error) const
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
void OutboundPage::showOperationError(const AppError& error)
{
    if (!error.errorMessage.isEmpty())
        QMessageBox::warning(this, errorToTitle(error), error.errorMessage);
}
// 显示错误消息
void OutboundPage::showErrorMessage(const QString& message)
{
    if (!message.isEmpty())
        QMessageBox::warning(this, QStringLiteral("错误"), message);
}
// 显示信息消息
void OutboundPage::showInformationMessage(const QString& message)
{
    if (!message.isEmpty())
        QMessageBox::information(this, QStringLiteral("信息"), message);
}
// 设置当前页的状态并显示消息
void OutboundPage::setPageState(OutboundPageState state)
{
    currentPageState_ = state;
    if (pageNavigator_)
        pageNavigator_->setLoading(currentPageState_ == OutboundPageState::Loading);
    updateActions(); // 更新按钮状态
}
// 重新加载当前页面(最新请求为准)
void OutboundPage::reloadCurrentPage()
{
    // 请求+1
    const auto requestSeq = ++listRequestSeq_;
    // 校验
    if (!outboundService_) {
        setPageState(OutboundPageState::Error);
        showErrorMessage(QStringLiteral("出库服务不可用"));
        return;
    }
    if (!tableModel_) {
        setPageState(OutboundPageState::Error);
        showErrorMessage(QStringLiteral("表格模型不可用"));
        return;
    }
    // 设置状态
    setPageState(OutboundPageState::Loading);
    outboundService_->listOrders(currentFilter_, currentRequest_, this, [this, requestSeq](const OutboundPageResult& result) {
        if (requestSeq != listRequestSeq_) // 以最新请求为准
            return;
        // 校验
        if (!result.success) {
            setPageState(OutboundPageState::Error);
            if (result.error.has_value())
                showOperationError(result.error.value());
            else
                showErrorMessage(QStringLiteral("未知错误"));

            return;
        }

        // 设置页面
        tableModel_->setPage(result.page);
        if (result.page.items.isEmpty()) {
            setPageState(OutboundPageState::Empty);
            pageNavigator_->updatePageInfo(tableModel_->page(), tableModel_->totalPages());
            return;
        }
        setPageState(OutboundPageState::Ready);
        pageNavigator_->updatePageInfo(tableModel_->page(), tableModel_->totalPages());
    });
}
// 获取选中的订单
std::optional<OutboundOrderListItemDto> OutboundPage::selectedOutboundDto() const
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
    const OutboundOrderListItemDto order = tableModel_->itemAt(row);
    if (order.id == 0)
        return std::nullopt;
    return order;
}
// 加载仓库数据到出库编辑对话框
void OutboundPage::loadOutboundDialogWarehouseOptions(OutboundEditDialog& dialog, bool isActiveOnly)
{
    if (!masterDataService_) {
        showErrorMessage(QStringLiteral("基础数据(仓库)服务不可用"));
        return;
    }

    const quint64 warehouseReloadId = dialog.beginWarehouseReload();
    QPointer<OutboundEditDialog> dialogPtr(&dialog);

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

// 加载产品选项数据到出库编辑对话框
void OutboundPage::loadOutboundDialogProductOptions(OutboundEditDialog& dialog, bool isActiveOnly)
{
    if (!productService_) {
        showErrorMessage(QStringLiteral("产品服务不可用"));
        return;
    }

    const quint64 productReloadId = dialog.beginProductReload();
    QPointer<OutboundEditDialog> dialogPtr(&dialog);

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

// 总API:加载仓库与产品数据到出库编辑对话框
void OutboundPage::loadOutboundDialogOptions(OutboundEditDialog& dialog, bool isActiveOnly)
{
    loadOutboundDialogWarehouseOptions(dialog, isActiveOnly);
    loadOutboundDialogProductOptions(dialog, isActiveOnly);
}
// 槽函数

// 创建草稿订单
void OutboundPage::onCreateClicked()
{
    // 校验
    if (!outboundService_) {
        showErrorMessage(QStringLiteral("出库服务不可用"));
        return;
    }
    if (!tableModel_) {
        showErrorMessage(QStringLiteral("表格模型不可用"));
        return;
    }
    if (!outboundService_->hasPermission(Permission::CreateOutboundOrders)) {
        showOperationError(AppError::permissionDenied());
        return;
    }
    // 创建编辑框
    OutboundEditDialog dialog(OutboundEditMode::Create, this);
    // 设置操作人
    auto user = outboundService_->currentUser();
    if (!user.has_value()) {
        showErrorMessage(QStringLiteral("当前用户不存在"));
        return;
    }
    dialog.setOperatorName(user->userName);
    // 加载仓库与产品选项数据
    connect(&dialog, &OutboundEditDialog::masterdataActiveOnlyChanged, this, [this, &dialog](bool activeOnly) {
        loadOutboundDialogWarehouseOptions(dialog, activeOnly);
    });

    loadOutboundDialogWarehouseOptions(dialog, dialog.isMasterdataActiveOnly()); // 加载仓库数据
    loadOutboundDialogProductOptions(dialog, true);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    QString errorMessage;
    if (!dialog.validateInput(errorMessage)) {
        showErrorMessage(errorMessage);
        return;
    }
    const CreateOutboundOrderRequest request = dialog.createRequest(); // 获取创建订单请求
    setPageState(OutboundPageState::Loading); // 设置加载
    outboundService_->createDraft(request, this, [this](const OutboundOperationResult& result) {
        if (!result.success) {
            setPageState(OutboundPageState::Error);
            if (result.error.has_value()) {
                showOperationError(result.error.value());
            } else {
                showErrorMessage(QStringLiteral("创建订单失败,未知错误"));
            }
            return;
        }
        showInformationMessage(QStringLiteral("创建出库订单成功"));
        reloadCurrentPage();
    });
}
// 确认订单
void OutboundPage::onConfirmClicked()
{
    // 校验
    if (!outboundService_) {
        showErrorMessage(QStringLiteral("出库服务不可用"));
        return;
    }
    if (!tableModel_) {
        showErrorMessage(QStringLiteral("表格模型不可用"));
        return;
    }
    if (!outboundService_->hasPermission(Permission::ConfirmOutboundOrders)) {
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
    std::optional<OutboundOrderListItemDto> orderDto = selectedOutboundDto();
    if (!orderDto.has_value()) {
        showInformationMessage(QStringLiteral("请选择订单"));
        return;
    }
    // 二次确认出库
    const auto answer = QMessageBox::question(
        this,
        QStringLiteral("确认出库"),
        QStringLiteral("确认出库订单 %1 吗?\n接收人: %2\n仓库: %3\n订单号:%4").arg(orderDto.value().orderNo).arg(orderDto.value().recipient).arg(orderDto.value().warehouseName).arg(orderDto.value().orderNo));
    if (answer != QMessageBox::Yes)
        return;
    setPageState(OutboundPageState::Loading); // 设置加载状态,防止重复点击
    // 确认订单
    outboundService_->confirmOrder(orderDto.value().id, this, [this](const OutboundOperationResult& result) {
        if (!result.success) {
            setPageState(OutboundPageState::Error);
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
void OutboundPage::onReloadClicked()
{
    reloadCurrentPage();
}
// 更新操作按钮的状态
void OutboundPage::updateActions()
{
    const auto selectedDto = selectedOutboundDto();
    bool loading = (currentPageState_ == OutboundPageState::Loading);
    bool isError = (currentPageState_ == OutboundPageState::Error);
    bool hasSelection = selectedDto.has_value();
    bool canCreate = outboundService_ && outboundService_->hasPermission(Permission::CreateOutboundOrders);
    bool canConfirm = hasSelection && outboundService_ && outboundService_->hasPermission(Permission::ConfirmOutboundOrders) && selectedDto.value().status == OutboundOrderStatus::Draft;
    bool canViewDetail = hasSelection && outboundService_ && outboundService_->hasPermission(Permission::ViewOutboundOrders) && currentPageState_ == OutboundPageState::Ready;
    // 设置按钮状态
    createBtn->setEnabled(!loading && canCreate);
    confirmBtn->setEnabled(!loading && canConfirm && hasSelection && currentPageState_ == OutboundPageState::Ready);
    viewDetailBtn->setEnabled(!loading && canViewDetail);
    reloadBtn->setEnabled(!loading);
    searchBtn->setEnabled(!loading);
    clearSearchBtn->setEnabled(!loading);
}

OutboundOrderFilter OutboundPage::readFilterFromControls() const
{
    OutboundOrderFilter filter;
    if (!keywordEdit_ || !statusCombo_ || !warehouseCombo_)
        return filter;

    filter.keyword = keywordEdit_->text().trimmed();

    const QVariant statusData = statusCombo_->currentData();
    if (statusData.isValid() && !statusData.isNull()) {
        filter.status = static_cast<OutboundOrderStatus>(statusData.toInt());
    }

    const QVariant warehouseData = warehouseCombo_->currentData();
    bool ok = false;
    const quint32 warehouseId = warehouseData.toUInt(&ok);
    if (ok && warehouseId > 0)
        filter.warehouseId = warehouseId;

    return filter;
}

void OutboundPage::resetFilterControls()
{
    keywordEdit_->clear();
    statusCombo_->setCurrentIndex(0);
    warehouseCombo_->setCurrentIndex(0);
}

void OutboundPage::onSearchClicked()
{
    currentFilter_ = readFilterFromControls();
    currentRequest_.page = 1;
    reloadCurrentPage();
}

void OutboundPage::onClearSearchClicked()
{
    resetFilterControls();
    currentFilter_ = {};
    currentRequest_.page = 1;
    reloadCurrentPage();
}

void OutboundPage::loadWarehouseSearchOptions()
{
    if (!masterDataService_) {
        showErrorMessage(QStringLiteral("主数据服务不可用"));
        return;
    }
    if (!warehouseCombo_)
        return;

    warehouseCombo_->setEnabled(false);
    while (warehouseCombo_->count() > 1) {
        warehouseCombo_->removeItem(1);
    }

    masterDataService_->listWarehouses(
        this,
        false,
        [this](const WarehouseListResult& result) {
            if (!result.success) {
                showErrorMessage(QStringLiteral("加载仓库数据失败"));
                warehouseCombo_->setEnabled(true);
                return;
            }
            if (!result.warehouses.has_value()) {
                showErrorMessage(QStringLiteral("未返回有效仓库数据"));
                warehouseCombo_->setEnabled(true);
                return;
            }

            if (!warehouseCombo_)
                return;

            for (const auto& warehouse : result.warehouses.value()) {
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
// 查看详情
void OutboundPage::onViewDetailClicked()
{
    if (!outboundService_) {
        showErrorMessage(QStringLiteral("出库服务不可用"));
        return;
    }
    std::optional<OutboundOrderListItemDto> orderDto = selectedOutboundDto();
    if (!orderDto.has_value()) {
        showInformationMessage(QStringLiteral("请选择订单"));
        return;
    }
    if (!outboundService_->hasPermission(Permission::ViewOutboundOrders)) {
        showOperationError(AppError::permissionDenied());
        return;
    }
    if(currentPageState_!=OutboundPageState::Ready)
        return;
    const quint64 requestSeq = ++detailRequestSeq_;
    outboundService_->getOrderDetail(orderDto.value().id, this, [this, requestSeq](const OutboundOrderDetailResult& result) {
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
        OutboundOrderDetailDialog* dialog=new OutboundOrderDetailDialog(orderDetail, this);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->open();
    });
}