#pragma once
#include "OutboundDto.h"
#include <QDialog>
#include <QFormLayout>
#include <QLabel>
#include <QString>
#include <QTableWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>
#include <optional>
class OutboundOrderDetailDialog : public QDialog {
    Q_OBJECT
public:
    explicit OutboundOrderDetailDialog(QWidget* parent = nullptr);
    explicit OutboundOrderDetailDialog(const OutboundOrderDetailDto& orderDetail_, QWidget* parent = nullptr);
    ~OutboundOrderDetailDialog() = default;
    static QString mapStatusToString(OutboundOrderStatus status);
    void setOrderDetail(const OutboundOrderDetailDto& orderDetail);
    void updateUI();

private:
    std::optional<OutboundOrderDetailDto> orderDetail;
    QLabel* orderNoLabel { nullptr };
    QLabel* recipientLabel { nullptr };
    QLabel* statusLabel { nullptr };
    QLabel* operatorLabel { nullptr };
    QLabel* warehouseLabel { nullptr };
    QTextEdit* remarkEdit { nullptr };
    QLabel* createdAtLabel { nullptr };
    QLabel* updatedAtLabel { nullptr };
    QLabel* confirmedAtLabel { nullptr };
    QLabel* lineCountLabel { nullptr };
    QLabel* totalQuantityLabel { nullptr };
    QLabel* totalAmountLabel { nullptr };
    QTableWidget* detailLinesTable { nullptr };
    QFormLayout* formLayout { nullptr };
    QVBoxLayout* mainLayout { nullptr };
};