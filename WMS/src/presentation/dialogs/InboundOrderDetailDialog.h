#pragma once
#include "InboundDto.h"
#include <QDialog>
#include <QString>
#include <QWidget>
#include <QLabel>
#include <QTableWidget>
#include <QTextEdit>
#include <optional>
#include <QVBoxLayout>
#include <QFormLayout>
class InboundOrderDetailDialog : public QDialog {
    Q_OBJECT
public:
    explicit InboundOrderDetailDialog(QWidget* parent = nullptr);
    explicit InboundOrderDetailDialog(const InboundOrderDetailDto& orderDetail_, QWidget* parent = nullptr);
    ~InboundOrderDetailDialog() = default;
    // 映射状态为字符串
    static QString mapStatusToString(InboundOrderStatus status);
    // 设置订单详情
    void setOrderDetail(const InboundOrderDetailDto& orderDetail);
    void updateUI(); // 更新UI
    

private:
    std::optional<InboundOrderDetailDto> orderDetail;
    QLabel* orderNoLabel{nullptr};
    QLabel* supplierLabel{nullptr};
    QLabel* statusLabel{nullptr};
    QLabel* operatorLabel{nullptr};
    QLabel* warehouseLabel{nullptr};
    QTextEdit* remarkEdit{nullptr};
    QLabel* createdAtLabel{nullptr};
    QLabel* updatedAtLabel{nullptr};
    QLabel* confirmedAtLabel{nullptr};
    QLabel* lineCountLabel{nullptr};
    QLabel* totalQuantityLabel{nullptr};
    QLabel* totalAmountLabel{nullptr};
    QTableWidget* detailLinesTable { nullptr };
    QFormLayout* formLayout { nullptr };
    QVBoxLayout* mainLayout { nullptr };
};