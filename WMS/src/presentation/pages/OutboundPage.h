#pragma once
#include "AppError.h"
#include "MasterDataService.h"
#include "OutboundEditDialog.h"
#include "OutboundService.h"
#include "OutboundTableModel.h"
#include "PageNavigator.h"
#include "ProductService.h"
#include "qcombobox.h"
#include <QLineEdit>
#include <QPushButton>
#include <QTableView>
#include <QWidget>
enum class OutboundPageState {
    Idle,
    Loading,
    Ready,
    Error,
    Empty
};

class OutboundPage : public QWidget {
    Q_OBJECT
public:
    OutboundPage(OutboundService* outboundService, ProductService* productService, MasterDataService* masterDataService, OutboundTableModel* tableModel, QWidget* parent = nullptr);
    ~OutboundPage() = default;
    void reloadCurrentPage(); // 重新加载
    void setPageState(OutboundPageState state); // 设置页面状态
    [[nodiscard]] OutboundPageState currentPageState() const noexcept { return currentPageState_; }
    void showOperationError(const AppError& error); // 显示操作错误
    void showErrorMessage(const QString& message); // 显示错误信息
    void showInformationMessage(const QString& message); // 显示信息
    void loadOutboundDialogOptions(OutboundEditDialog& dialog, bool isActiveOnly = true); // 总API:加载仓库与产品数据
    void loadOutboundDialogWarehouseOptions(OutboundEditDialog& dialog, bool isActiveOnly = true); // 加载仓库数据
    void loadOutboundDialogProductOptions(OutboundEditDialog& dialog, bool isActiveOnly = true); // 加载产品选项数据
    OutboundOrderFilter readFilterFromControls() const; // 从控件读取筛选器
    void resetFilterControls(); // 重置筛选器控件
    void loadWarehouseSearchOptions(); // 加载仓库搜索选项
public slots:
    void onCreateClicked(); // 创建草稿
    void onConfirmClicked(); // 确认订单
    void onReloadClicked(); // 重新加载
    void onSearchClicked(); // 搜索订单
    void onClearSearchClicked(); // 清除搜索
    void onViewDetailClicked(); // 查看详情

private:
    // 搜索功能
    QLineEdit* keywordEdit_; // 搜索关键词输入框
    QComboBox* statusCombo_; // 订单状态下拉框
    QComboBox* warehouseCombo_; // 仓库下拉框(显示所有仓库,包括停用仓库)
    QPushButton* searchBtn; // 搜索按钮
    QPushButton* clearSearchBtn; // 清除搜索按钮

    quint64 listRequestSeq_ { 0 }; // 列表请求序列号
    quint64 detailRequestSeq_ { 0 }; // 详情请求序列号
                                     //
    QPushButton* createBtn; // 创建草稿按钮
    QPushButton* confirmBtn; // 确认按钮
    QPushButton* reloadBtn; // 重新加载按钮
    QPushButton* viewDetailBtn; // 查看详情按钮
    PageNavigator* pageNavigator_; // 分页导航

    OutboundService* outboundService_;
    ProductService* productService_; // 用于获取产品相关选项
    MasterDataService* masterDataService_;
    OutboundTableModel* tableModel_;

    OutboundPageState currentPageState_ { OutboundPageState::Idle }; // 当前页面状态
    std::optional<AuthenticatedUser> currentUser_; // 当前用户
    OutboundOrderFilter currentFilter_; // 订单筛选器
    PageRequest currentRequest_; // 当前页面请求
    QTableView* tableView_; // 订单表格视图
    QItemSelectionModel* selectionModel_;

    std::optional<OutboundOrderListItemDto> selectedOutboundDto() const; // 当前选中的订单
    void updateActions(); // 更新操作按钮状态
    QString errorToTitle(const AppError& error) const; // 错误映射为标题
};