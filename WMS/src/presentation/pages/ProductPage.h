// ProductPage 产品页:显示产品信息及相关操作按钮和输入框
#pragma once
#include "AppError.h"
#include "MasterDataService.h"
#include "PageNavigator.h"
#include "ProductEditDialog.h"
#include "ProductService.h"
#include "ProductTableModel.h"
#include "qcombobox.h"
#include <QCheckBox>
#include <QComboBox>
#include <QItemSelectionModel>
#include <QLineEdit>
#include <QPushButton>
#include <QString>
#include <QTableView>
#include <optional>
// 设置当前页的状态
enum class ProductPageState {
    Idle, // 空闲
    Loading, // 加载中
    Ready, // 就绪(有数据)
    Error, // 错误
    Empty // 空(无数据)
};

class ProductPage : public QWidget {
    Q_OBJECT
public:
    ProductPage(ProductService* ps, MasterDataService* masterDataService, ProductTableModel* tableModel, QWidget* parent = nullptr);
    ~ProductPage() = default;

    void reloadCurrentPage(); // 重新加载当前页面(在更新数据后调用,刷新页面显示,重新查询 > 乐观刷新)
    void setPageState(ProductPageState state); // 设置当前页的状态
    // 当前页状态(供测试/外部观察)
    [[nodiscard]] ProductPageState currentPageState() const noexcept { return currentPageState_; }
    void showOperationError(const AppError& error); // 显示操作错误消息
    void showErrorMessage(const QString& message); // 显示错误消息
    void showInformationMessage(const QString& message); // 显示信息消息
public slots:
    void onCreateClicked(); // 创建产品
    void onEditClicked(); // 编辑产品
    void onSetActiveClicked(); // 设置产品状态(启用/禁用)
    void onSearchClicked(); // 搜索产品
    void onClearClicked(); // 清除搜索条件
    void onClearKeywordClicked(); // 清除关键词

    ProductFilter readFilterFromControls() const; // 从控件读取筛选器
    void resetFilterControls(); // 重置筛选器控件
    void loadCategorySearchOptions(); // 加载分类搜索选项
    void setCurrentFilter(const ProductFilter& filter); // 设置当前筛选器
    [[nodiscard]] PageRequest currentPageRequest() const; // 获取当前分页请求参数
private:
    quint64 listRequestSeq_ { 0 }; // 列表请求序列号(用于判断是否是最新的请求)
    QPushButton* createBtn; // 创建产品按钮
    QPushButton* editBtn; // 编辑产品按钮
    QPushButton* setActiveBtn; // 设置产品状态按钮(启用/禁用)
    QPushButton* reloadBtn; // 重新加载按钮
    PageNavigator* pageNavigator_; // 分页导航
    ProductService* productService_; // 产品服务
    MasterDataService* masterDataService_; // 仓库基础数据服务
    ProductTableModel* tableModel_; // 产品表格模型
    std::optional<AuthenticatedUser> currentUser_; // 当前会话的用户
    // 搜索栏
    QLineEdit* keywordLineEdit_; // 搜索关键词输入框
    QPushButton* clearKeywordBtn_; // 清除关键词按钮
    QComboBox* categoryComboBox_; // 分类下拉选择框
    QCheckBox* activeOnlyCheckBox_; // 仅显示激活状态复选框
    QPushButton* searchBtn_; // 搜索按钮
    QPushButton* clearBtn_; // 清除按钮
    ProductFilter currentFilter_; // 筛选条件
    PageRequest currentRequest_; // 当前分页请求参数
    
    ProductPageState currentPageState_ { ProductPageState::Idle }; // 当前页的状态
    bool categoryOptionsLoading_ { false }; // 分类选项是否正在加载
    QTableView* tableView_; // 表格视图
    QLabel* emptyLabel_; // 空状态标签
    QItemSelectionModel* selectionModel_; // 选中模型
    // 获取当前选中的产品
    std::optional<ProductListItemDto> selectedProductDto() const;
    void loadProductDialogOptions(ProductEditDialog& dialog, bool activeOnly = true); // 将分类/单位服务加载到产品编辑对话框
    void updateActions(); // 更新操作按钮的状态
    QString errorToTitle(const AppError& error) const; // 将错误映射为标题
};