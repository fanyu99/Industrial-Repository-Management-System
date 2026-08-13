#pragma once
#include "InboundDto.h"
#include "InboundRequests.h"
#include <QAbstractItemView>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTextEdit>
#include <QVBoxLayout>
// 编辑模式
enum class InboundEditMode {
    Create, // 创建入库单
};
class InboundEditDialog : public QDialog {
    Q_OBJECT
public:
    // 订单明细列枚举
    enum class LineColumn {
        ProductId = 0,
        Quantity,
        UnitPrice,
        CountColumn // 获取订单列数
    };
    InboundEditDialog(InboundEditMode mode = InboundEditMode::Create, QWidget* parent = nullptr);
    ~InboundEditDialog() = default;
    bool isMasterdataActiveOnly() const noexcept;
    void setInboundOrder(const InboundOrderListItemDto& order); // 设置编辑的订单
    bool validateInput(QString& errorMessage) const noexcept; // 校验输入
    void setMode(InboundEditMode mode); // 设置编辑模式
    bool addWarehouse(const QString& warehouseName, quint32 warehouseId, QString& errorMessage); // 添加仓库
    // 设置加载中
    void setOptionsLoading(bool loading);
    // 获取创建入库单请求
    CreateInboundOrderRequest createRequest() const noexcept;
    // 添加空订单行
    void addEmptyLine();
    // 删除选中行
    void removeSelectedLine();
    // 更新订单行数据
    void updateLineSummary();
    // 获取当前对话用户并设置
    void setOperatorName(const QString& operatorName);
    // 信号
signals:
    // 仅显示活跃数据复选框状态改变时触发
    void masterdataActiveOnlyChanged(bool activeOnly);
    // 仓库数据动态刷新
public:
    quint64 beginWarehouseReload();
    bool isCurrentWarehouseReload(quint64 reloadId) const noexcept;
    void finishWarehouseReload(quint64 reloadId, bool success);
    void clearWarehouseOptions(); // 清除仓库选项
private:
    QLineEdit* orderNoEdit_; // 入库订单编号输入框
    QLineEdit* supplierEdit_; // 供应商输入框
    QLineEdit* operatorNameEdit_; // 操作人输入框
    QComboBox* warehouseComboBox_; // 仓库下拉框
    QLineEdit* lineCountEdit_; // 明细数
    QLineEdit* totalQuantityEdit_;
    QCheckBox* masterdataActiveOnlyCheckBox_; // 仅显示活跃数据复选框
    QTextEdit* remarkEdit_; // 备注
    QTableWidget* linesTable_; // 订单明细表
    QPushButton* addLineBtn_; // 添加订单行
    QPushButton* removeLineBtn_; // 删除订单行
    QDialogButtonBox* sureButtonBox_; // 确认按钮框
    InboundEditMode editMode_; // 编辑模式
    quint64 warehouseReloadId_ { 0 }; // 当前仓库重新加载ID
    quint32 pendingWarehouseId_ { 0 }; // 待回显仓库ID
};