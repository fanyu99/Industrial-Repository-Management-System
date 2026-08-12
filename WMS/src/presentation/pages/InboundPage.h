#pragma once
#include "AppError.h"
#include "PageNavigator.h"
#include "InboundEditDialog.h"
#include "InboundService.h"
#include "InboundTableModel.h"
#include "MasterDataService.h"
#include <QPushButton>
#include <QTableView>
#include <QWidget>
enum class InboundPageState {
    Idle,
    Loading,
    Ready,
    Error,
    Empty
};

class InboundPage : public QWidget {
    Q_OBJECT
public:
    InboundPage(InboundService* inboundService, MasterDataService* masterDataService, InboundTableModel* tableModel, QWidget* parent = nullptr);
    ~InboundPage() = default;
    void reloadCurrentPage(); // 重新加载
    void setPageState(InboundPageState state); // 设置页面状态
    [[nodiscard]] InboundPageState currentPageState() const noexcept; // 当前页面状态
    void showOperationError(const AppError& error); // 显示操作错误
    void showErrorMessage(const QString& message); // 显示错误信息
    void showInformationMessage(const QString& message); // 显示信息
    void loadInboundDialogOptions(InboundEditDialog& dialog, bool isActiveOnly = false); // 加载分类/单位服务数据
public slots:
    void onCreateClicked(); // 创建草稿
    void onConfirmClicked(); // 确认订单
    void onReloadClicked(); // 重新加载

private:
    quint64 listRequestSeq_ { 0 }; // 列表请求序列号
                                   //
    QPushButton* createBtn; // 创建草稿按钮
    QPushButton* confirmBtn; // 确认按钮
    QPushButton* reloadBtn; // 重新加载按钮
    PageNavigator* pageNavigator_; // 分页导航

    InboundService* inboundService_;
    MasterDataService* masterDataService_;
    InboundTableModel* tableModel_;

    InboundPageState currentPageState_ { InboundPageState::Idle }; // 当前页面状态
    std::optional<AuthenticatedUser> currentUser_; // 当前用户
    InboundOrderFilter currentFilter_; // 订单筛选器
    PageRequest currentRequest_; // 当前页面请求
    QTableView* tableView_; // 订单表格视图
    QItemSelectionModel* selectionModel_;

    std::optional<InboundOrderListItemDto> selectedInboundDto() const; // 当前选中的订单
    void updateActions(); // 更新操作按钮状态
    QString errorToTitle(const AppError& error) const; // 错误映射为标题
};