#include "PageNavigator.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>

PageNavigator::PageNavigator(QWidget* parent)
    : QWidget(parent)
{
    this->goButton_ = new QPushButton(QStringLiteral("转到"));
    this->prevButton_ = new QPushButton(QStringLiteral("上一页"));
    this->nextButton_ = new QPushButton(QStringLiteral("下一页"));
    this->firstButton_ = new QPushButton(QStringLiteral("第一页"));
    this->lastButton_ = new QPushButton(QStringLiteral("最后一页"));
    this->pageSpinBox_ = new QSpinBox(this);
    this->pageSpinBox_->setMinimum(1);
    this->pageInfoLabel_ = new QLabel(QStringLiteral("第1页/共1页"), this);
    this->pageInfoLabel_->setAlignment(Qt::AlignCenter);
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->addWidget(firstButton_);
    layout->addWidget(prevButton_);
    layout->addWidget(pageInfoLabel_);
    layout->addWidget(nextButton_);
    layout->addWidget(lastButton_);
    layout->addWidget(goButton_);
    layout->addWidget(pageSpinBox_);

    // 按钮发送信号
    connect(prevButton_, &QPushButton::clicked, this, [this]() { emit pageChanged(pageSpinBox_->value() - 1); });
    connect(nextButton_, &QPushButton::clicked, this, [this]() { emit pageChanged(pageSpinBox_->value() + 1); });
    connect(goButton_, &QPushButton::clicked, this, [this]() { emit pageChanged(pageSpinBox_->value()); });
    connect(pageSpinBox_, &QSpinBox::returnPressed, this, [this]() { emit pageChanged(pageSpinBox_->value()); });
    connect(firstButton_, &QPushButton::clicked, this, [this]() { emit pageChanged(1); });
    connect(lastButton_, &QPushButton::clicked, this, [this]() { emit pageChanged(pageSpinBox_->maximum()); });
}
// 对页码进行校验并设置按钮状态(<=0设置1,>totalPages设置totalPages)
void PageNavigator::checkPage(int& page, int& totalPages)
{
    if (page <= 0)
        page = 1;
    if (totalPages <= 0)
        totalPages = 1;
    if (page > totalPages)
        page = totalPages;

    firstButton_->setEnabled(page != 1);
    prevButton_->setEnabled(page != 1);
    nextButton_->setEnabled(page != totalPages);
    lastButton_->setEnabled(page != totalPages);
}

// 更新分页信息
void PageNavigator::updatePageInfo(int currentPage, int totalPages)
{
    this->checkPage(currentPage, totalPages); // 校验页码并设置按钮状态
    this->pageInfoLabel_->setText(
        QStringLiteral("第%1页/共%2页").arg(currentPage).arg(totalPages));
    this->pageSpinBox_->setValue(currentPage);
    this->pageSpinBox_->setRange(1, totalPages);
}
// 设置加载状态(禁用所有部件)
void PageNavigator::setLoading(bool loading)
{
    this->prevButton_->setEnabled(!loading);
    this->nextButton_->setEnabled(!loading);
    this->goButton_->setEnabled(!loading);
    this->pageSpinBox_->setEnabled(!loading);
    this->firstButton_->setEnabled(!loading);
    this->lastButton_->setEnabled(!loading);
}