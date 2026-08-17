#include "OutboundOrderDetailDialog.h"
#include <QAbstractItemView>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextEdit>
#include <QVBoxLayout>
OutboundOrderDetailDialog::OutboundOrderDetailDialog(QWidget* parent)
    : QDialog(parent)
{
    setObjectName(QStringLiteral("OutboundOrderDetailDialog"));
    mainLayout = new QVBoxLayout(this);
    formLayout = new QFormLayout();
    orderNoLabel = new QLabel(this);
    orderNoLabel->setObjectName(QStringLiteral("OutboundOrderDetailDialog_orderNoLabel"));
    recipientLabel = new QLabel(this);
    recipientLabel->setObjectName(QStringLiteral("OutboundOrderDetailDialog_recipientLabel"));
    statusLabel = new QLabel(this);
    statusLabel->setObjectName(QStringLiteral("OutboundOrderDetailDialog_statusLabel"));
    operatorLabel = new QLabel(this);
    operatorLabel->setObjectName(QStringLiteral("OutboundOrderDetailDialog_operatorLabel"));
    warehouseLabel = new QLabel(this);
    warehouseLabel->setObjectName(QStringLiteral("OutboundOrderDetailDialog_warehouseLabel"));
    remarkEdit = new QTextEdit(this);
    remarkEdit->setObjectName(QStringLiteral("OutboundOrderDetailDialog_remarkEdit"));
    remarkEdit->setReadOnly(true);
    createdAtLabel = new QLabel(this);
    createdAtLabel->setObjectName(QStringLiteral("OutboundOrderDetailDialog_createdAtLabel"));
    updatedAtLabel = new QLabel(this);
    updatedAtLabel->setObjectName(QStringLiteral("OutboundOrderDetailDialog_updatedAtLabel"));
    confirmedAtLabel = new QLabel(this);
    confirmedAtLabel->setObjectName(QStringLiteral("OutboundOrderDetailDialog_confirmedAtLabel"));
    confirmedAtLabel->setVisible(false);
    lineCountLabel = new QLabel(this);
    lineCountLabel->setObjectName(QStringLiteral("OutboundOrderDetailDialog_lineCountLabel"));
    totalQuantityLabel = new QLabel(this);
    totalQuantityLabel->setObjectName(QStringLiteral("OutboundOrderDetailDialog_totalQuantityLabel"));
    totalAmountLabel = new QLabel(this);
    totalAmountLabel->setObjectName(QStringLiteral("OutboundOrderDetailDialog_totalAmountLabel"));
    formLayout->addRow(QStringLiteral("订单号"), orderNoLabel);
    formLayout->addRow(QStringLiteral("领用人"), recipientLabel);
    formLayout->addRow(QStringLiteral("状态"), statusLabel);
    formLayout->addRow(QStringLiteral("操作人"), operatorLabel);
    formLayout->addRow(QStringLiteral("仓库"), warehouseLabel);
    formLayout->addRow(QStringLiteral("备注"), remarkEdit);
    formLayout->addRow(QStringLiteral("创建时间"), createdAtLabel);
    formLayout->addRow(QStringLiteral("更新时间"), updatedAtLabel);
    formLayout->addRow(QStringLiteral("确认时间"), confirmedAtLabel);
    formLayout->addRow(QStringLiteral("明细数"), lineCountLabel);
    formLayout->addRow(QStringLiteral("总数量"), totalQuantityLabel);
    formLayout->addRow(QStringLiteral("总金额"), totalAmountLabel);

    detailLinesTable = new QTableWidget(this);
    detailLinesTable->setObjectName(QStringLiteral("OutboundOrderDetailDialog_detailLinesTable"));
    detailLinesTable->setColumnCount(5);
    detailLinesTable->setHorizontalHeaderLabels({ QStringLiteral("商品编号"), QStringLiteral("商品名称"), QStringLiteral("数量"), QStringLiteral("单价"), QStringLiteral("金额") });
    detailLinesTable->setSelectionMode(QAbstractItemView::NoSelection);
    detailLinesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    buttonBox->setObjectName(QStringLiteral("OutboundOrderDetailDialog_buttonBox"));
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(detailLinesTable);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);
}
QString OutboundOrderDetailDialog::mapStatusToString(OutboundOrderStatus status)
{
    switch (status) {
    case OutboundOrderStatus::Draft:
        return QStringLiteral("待处理");
    case OutboundOrderStatus::Confirmed:
        return QStringLiteral("已确认");
    case OutboundOrderStatus::Cancelled:
        return QStringLiteral("已取消");
    default:
        return QStringLiteral("未知状态");
    }
}
OutboundOrderDetailDialog::OutboundOrderDetailDialog(
    const OutboundOrderDetailDto& orderDetail_,
    QWidget* parent)
    : OutboundOrderDetailDialog(parent)
{
    setOrderDetail(orderDetail_);
}
void OutboundOrderDetailDialog::setOrderDetail(const OutboundOrderDetailDto& orderDetail)
{
    this->orderDetail = orderDetail;
    updateUI();
    setWindowTitle(QStringLiteral("%1出库订单详情").arg(orderDetail.orderNo));
}
void OutboundOrderDetailDialog::updateUI()
{
    if (!orderDetail.has_value())
        return;
    const auto& detail = orderDetail.value();
    if (orderNoLabel) {
        orderNoLabel->setText(detail.orderNo);
        orderNoLabel->setProperty("orderId", detail.id);
    }
    if (recipientLabel) {
        recipientLabel->setText(detail.recipient);
    }
    if (statusLabel) {
        statusLabel->setText(mapStatusToString(detail.status));
    }
    if (operatorLabel) {
        operatorLabel->setText(detail.operatorName);
        operatorLabel->setProperty("operatorId", detail.operatorId);
    }
    if (warehouseLabel) {
        warehouseLabel->setText(detail.warehouseName);
        warehouseLabel->setProperty("warehouseId", detail.warehouseId);
    }
    if (remarkEdit) {
        remarkEdit->setText(detail.remark);
    }
    if (createdAtLabel) {
        createdAtLabel->setText(detail.createdAt.toString("yyyy-MM-dd HH:mm:ss"));
    }
    if (updatedAtLabel) {
        updatedAtLabel->setText(detail.updatedAt.toString("yyyy-MM-dd HH:mm:ss"));
    }
    if (confirmedAtLabel && detail.confirmedAt.has_value()) {
        confirmedAtLabel->setVisible(true);
        confirmedAtLabel->setText(detail.confirmedAt.value().toString("yyyy-MM-dd HH:mm:ss"));
        QWidget* labelItem = formLayout->labelForField(confirmedAtLabel);
        if (labelItem)
            labelItem->setVisible(true);
    } else {
        confirmedAtLabel->setVisible(false);
        QWidget* labelItem = formLayout->labelForField(confirmedAtLabel);
        if (labelItem)
            labelItem->setVisible(false);
    }
    if (lineCountLabel) {
        lineCountLabel->setText(QString::number(detail.lineCount));
    }
    if (totalQuantityLabel) {
        totalQuantityLabel->setText(QString::number(detail.totalQuantity));
    }
    if (totalAmountLabel) {
        totalAmountLabel->setText(QString::number(detail.totalAmount, 'f', 2));
    }
    if (detailLinesTable) {
        detailLinesTable->setRowCount(detail.detailLines.size());
        for (int i = 0; i < detail.detailLines.size(); ++i) {
            detailLinesTable->setItem(i, 0, new QTableWidgetItem(detail.detailLines[i].productCode));
            detailLinesTable->item(i, 0)->setData(Qt::UserRole, detail.detailLines[i].productId);
            detailLinesTable->setItem(i, 1, new QTableWidgetItem(detail.detailLines[i].productName));
            detailLinesTable->setItem(i, 2, new QTableWidgetItem(QString::number(detail.detailLines[i].quantity)));
            detailLinesTable->setItem(i, 3, new QTableWidgetItem(QString::number(detail.detailLines[i].unitPrice, 'f', 2)));
            detailLinesTable->setItem(i, 4, new QTableWidgetItem(QString::number(detail.detailLines[i].subtotal, 'f', 2)));
        }
    }
}