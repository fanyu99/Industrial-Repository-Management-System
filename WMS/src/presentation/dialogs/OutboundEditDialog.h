#pragma once
#include "OptionLoadTracker.h"
#include "OutboundDto.h"
#include "OutboundRequests.h"
#include "ProductDto.h"
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
enum class OutboundEditMode {
    Create, // 创建出库单
};
class OutboundEditDialog : public QDialog {
    Q_OBJECT
public:
    // 订单明细列枚举
    enum class LineColumn {
        Product = 0, // 产品
        Quantity,
        UnitPrice,
        CountColumn // 获取订单列数
    };
    OutboundEditDialog(OutboundEditMode mode = OutboundEditMode::Create, QWidget* parent = nullptr);
    ~OutboundEditDialog() = default;
    bool isMasterdataActiveOnly() const noexcept;
    void setOutboundOrder(const OutboundOrderListItemDto& order); // 设置编辑的订单
    void setProductOptions(const QVector<ProductOptionDto>& productOptions); // 设置产品选项
    bool validateInput(QString& errorMessage) const noexcept; // 校验输入
    void setMode(OutboundEditMode mode); // 设置编辑模式
    bool addWarehouse(const QString& warehouseName, quint32 warehouseId, QString& errorMessage); // 添加仓库
    // 获取创建出库单请求
    CreateOutboundOrderRequest createRequest() const noexcept;
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
    // 仓库与产品数据动态刷新
public:
    quint64 beginWarehouseReload();
    bool isCurrentWarehouseReload(quint64 reloadId) const noexcept;
    void finishWarehouseReload(quint64 reloadId, bool success);
    void clearWarehouseOptions(); // 清除仓库选项

    quint64 beginProductReload();
    bool isCurrentProductReload(quint64 reloadId) const noexcept;
    void finishProductReload(quint64 reloadId, bool success);
    void clearProductOptions(); // 清除产品选项
    void setProductComboEnabled(bool enabled); // 设置产品选项可用
    void syncOptionControls(); // 同步选项刷新状态,更新对应组件状态

    [[nodiscard]] const QVector<ProductOptionDto>& productOptions() const noexcept { return productOptions_; }
    [[nodiscard]] QPushButton* addLineBtn() const noexcept { return addLineBtn_; }
    [[nodiscard]] OptionLoadState warehouseLoadState() const noexcept { return warehouseLoad_.state; }
    [[nodiscard]] OptionLoadState productLoadState() const noexcept { return productLoad_.state; }

private:
    QVector<ProductOptionDto> productOptions_; // 产品选项
    QLineEdit* orderNoEdit_; // 出库订单编号输入框
    QLineEdit* recipientEdit_; // 接收人输入框
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
    OutboundEditMode editMode_; // 编辑模式
    OptionLoadTracker warehouseLoad_; // 刷新仓库上下文
    OptionLoadTracker productLoad_; // 刷新产品上下文
    quint32 pendingWarehouseId_ { 0 }; // 待回显仓库ID
};