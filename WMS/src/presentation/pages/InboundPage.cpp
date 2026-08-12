#include "InboundPage.h"
#include <QAbstractItemView>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPointer>
#include <QTableView>
#include <QVBoxLayout>
#include <QPushButton>

InboundPage::InboundPage(InboundService* inboundService, MasterDataService* masterDataService, InboundTableModel* tableModel, QWidget* parent)
    : QWidget(parent)
    , inboundService_(inboundService)
    , masterDataService_(masterDataService)
    , tableModel_(tableModel)
{
    if (!inboundService_ || !masterDataService_ || !tableModel_) {
        showErrorMessage(QStringLiteral("入库订单页初始化失败,相关服务/模型异常"));
        return;
    }
    setWindowTitle(QStringLiteral("入库订单页"));
    if (inboundService_) {
        currentUser_ = inboundService_->currentUser();
    }
    this->setObjectName(QStringLiteral("InboundPage"));
    // 表格视图
    tableView_ = new QTableView(this);
    tableView_->setObjectName(QStringLiteral("InboundPage_tableView"));
    tableView_->setModel(tableModel_);
    tableView_->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableView_->setSelectionMode(QAbstractItemView::SingleSelection);
    selectionModel_ = tableView_->selectionModel();
    // 总布局
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    //  按钮布局
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    createBtn = new QPushButton(QStringLiteral("创建入库订单"), this);
    confirmBtn = new QPushButton(QStringLiteral("确认入库订单"), this);
    reloadBtn = new QPushButton(QStringLiteral("重新加载"), this);
    createBtn->setObjectName(QStringLiteral("InboundPage_createBtn"));
    confirmBtn->setObjectName(QStringLiteral("InboundPage_confirmBtn"));
    reloadBtn->setObjectName(QStringLiteral("InboundPage_reloadBtn"));
    // 分页导航
    pageNavigator_ = new PageNavigator(this);
    pageNavigator_->setObjectName(QStringLiteral("InboundPage_pageNavigator"));

    // 添加布局和组件
    buttonLayout->addWidget(createBtn);
    buttonLayout->addWidget(confirmBtn);
    buttonLayout->addWidget(reloadBtn);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addWidget(tableView_);
    mainLayout->addWidget(pageNavigator_);
    // 连接信号槽
    connect(createBtn, &QPushButton::clicked, this, &InboundPage::onCreateClicked);
    connect(confirmBtn, &QPushButton::clicked, this, &InboundPage::onConfirmClicked);
    connect(reloadBtn, &QPushButton::clicked, this, &InboundPage::onReloadClicked);
    if (selectionModel_) {
        connect(selectionModel_, &QItemSelectionModel::selectionChanged, this, [this]() { updateActions(); });
    }
    // 转页后重新加载页面
    connect(pageNavigator_, &PageNavigator::pageChanged, this, [this](int page) {
        currentRequest_.page = page;
        reloadCurrentPage();
    }); // 翻页,重新加载页面

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
        pageNavigator_->updatePageInfo(result.page.page, tableModel_->totalPages()); // 更新信息
        if (result.page.items.isEmpty()) {
            setPageState(InboundPageState::Empty);
            return;
        } else
            setPageState(InboundPageState::Ready);
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
// 加载仓库服务数据
void InboundPage::loadInboundDialogOptions(InboundEditDialog& dialog, bool isActiveOnly)
{
    // 校验
    if (!masterDataService_) {
        showErrorMessage(QStringLiteral("基础数据(仓库)服务不可用"));
        return;
    }
    const quint64 reloadId = dialog.beginWarehouseReload();

    QPointer<InboundEditDialog> dialogPtr(&dialog);
    masterDataService_->listWarehouses(
        &dialog,
        isActiveOnly,
        [this, dialogPtr, reloadId](const WarehouseListResult& result) {
            if (!dialogPtr || !dialogPtr->isCurrentWarehouseReload(reloadId)) {
                return;
            }

            if (!result.success) {
                dialogPtr->finishWarehouseReload(reloadId, false);
                showErrorMessage(QStringLiteral("加载仓库数据失败"));
                return;
            }

            QString errorMessage;
            for (const auto& warehouse : result.warehouses.value()) {
                if (!dialogPtr->addWarehouse(warehouse.name, warehouse.id, errorMessage)) {
                    dialogPtr->finishWarehouseReload(reloadId, false);
                    showErrorMessage(errorMessage);
                    return;
                }
            }

            dialogPtr->finishWarehouseReload(reloadId, true);
        });
}
// 槽函数

// TODO:创建草稿订单
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
    // 加载分类/单位/仓库服务数据
    connect(&dialog, &InboundEditDialog::masterdataActiveOnlyChanged, this, [this, &dialog](bool activeOnly) {
        loadInboundDialogOptions(dialog, activeOnly);
    });
    loadInboundDialogOptions(dialog, dialog.isMasterdataActiveOnly());
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    QString errorMessage;
    if (!dialog.validateInput(errorMessage)) {
        showErrorMessage(errorMessage);
        return;
    }
    // TODO: 后续创建订单明细请求+创建订单请求...
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
        QStringLiteral("确认入库订单 %1 吗?\n供应商: %2\n仓库: %3\n").arg(orderDto.value().orderNo).arg(orderDto.value().supplier).arg(orderDto.value().warehouseName),
        QStringLiteral("订单编号:%1\n").arg(orderDto.value().orderNo));
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
// 更新操作按钮的状态
void InboundPage::updateActions()
{
    const auto selectedDto = selectedInboundDto();
    bool loading = (currentPageState_ == InboundPageState::Loading);
    bool hasSelection = selectedDto.has_value();
    bool canCreate = inboundService_ && inboundService_->hasPermission(Permission::CreateInboundOrders);
    bool canConfirm = hasSelection && inboundService_ && inboundService_->hasPermission(Permission::ConfirmInboundOrders) && selectedDto.value().status == InboundOrderStatus::Draft;
    // 设置按钮状态
    createBtn->setEnabled(!loading && canCreate);
    confirmBtn->setEnabled(!loading && canConfirm && hasSelection && currentPageState_ == InboundPageState::Ready);
    reloadBtn->setEnabled(!loading);
}