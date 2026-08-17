#pragma once
#include "AppError.h"
#include "InboundEditDialog.h"
#include "InboundOrderDetailDialog.h"
#include "InboundService.h"
#include "InboundTableModel.h"
#include "MasterDataService.h"
#include "PageNavigator.h"
#include "ProductService.h"
#include "qabstractspinbox.h"
#include "qcombobox.h"
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
    InboundPage(InboundService* inboundService, ProductService* productService, MasterDataService* masterDataService, InboundTableModel* tableModel, QWidget* parent = nullptr);
    ~InboundPage() = default;
    void reloadCurrentPage(); // 重新加载
    void setPageState(InboundPageState state); // 设置页面状态
    [[nodiscard]] InboundPageState currentPageState() const noexcept { return currentPageState_; }
    void showOperationError(const AppError& error); // 显示操作错误
    void showErrorMessage(const QString& message); // 显示错误信息
    void showInformationMessage(const QString& message); // 显示信息
    void loadInboundDialogOptions(InboundEditDialog& dialog, bool isActiveOnly = true); // 总API:加载仓库与产品数据
    void loadInboundDialogWarehouseOptions(InboundEditDialog& dialog, bool isActiveOnly = true); // 加载仓库数据
    void loadInboundDialogProductOptions(InboundEditDialog& dialog, bool isActiveOnly = true); // 加载产品选项数据
    InboundOrderFilter readFilterFromControls() const; // 从控件读取筛选器
    void resetFilterControls(); // 重置筛选器控件
    void loadWarehouseSearchOptions(); // 加载仓库搜索选项
public slots:
    void onCreateClicked(); // 创建草稿
    void onConfirmClicked(); // 确认订单
    void onReloadClicked(); // 重新加载
    void onDetailClicked(); // 查看详情

    void onSearchClicked(); // 搜索订单
    void onClearSearchClicked(); // 清除所有条件
    void onClearKeywordClicked(); // 清除搜索关键词

private:
    // 搜索功能
    QLineEdit* keywordEdit_; // 搜索关键词输入框
    QComboBox* statusCombo_; // 订单状态下拉框
    QComboBox* warehouseCombo_; // 仓库下拉框(显示所有仓库,包括停用仓库)
    QPushButton* searchBtn; // 搜索按钮
    QPushButton* clearSearchBtn; // 清除所有条件按钮
    QPushButton* keywordClearBtn; // 清除搜索关键词按钮(内嵌✕)

    quint64 listRequestSeq_ { 0 }; // 列表请求序列号
    quint64 detailRequestSeq_ { 0 }; // 详情请求序列号
                                     //
    QPushButton* createBtn; // 创建草稿按钮
    QPushButton* confirmBtn; // 确认按钮
    QPushButton* reloadBtn; // 重新加载按钮
    QPushButton* detailBtn; // 详情按钮
    PageNavigator* pageNavigator_; // 分页导航

    InboundService* inboundService_;
    ProductService* productService_; // 用于获取产品相关选项
    MasterDataService* masterDataService_;
    InboundTableModel* tableModel_;

    InboundPageState currentPageState_ { InboundPageState::Idle }; // 当前页面状态
    std::optional<AuthenticatedUser> currentUser_; // 当前用户
    InboundOrderFilter currentFilter_; // 订单筛选器
    PageRequest currentRequest_; // 当前页面请求
    QTableView* tableView_; // 订单表格视图
    QLabel* emptyLabel_; // 空状态标签
    QItemSelectionModel* selectionModel_;

    std::optional<InboundOrderListItemDto> selectedInboundDto() const; // 当前选中的订单
    void updateActions(); // 更新操作按钮状态
    QString errorToTitle(const AppError& error) const; // 错误映射为标题
};