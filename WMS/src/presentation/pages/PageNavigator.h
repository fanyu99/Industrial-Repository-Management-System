// 分页导航器
#pragma once
#include <QObject>
#include <QWidget>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>
class PageNavigator : public QWidget
{
    Q_OBJECT
public:
    PageNavigator(QWidget *parent = nullptr);
    ~PageNavigator() = default;
    // 更新分页信息
    void updatePageInfo(int currentPage, int totalPages);
    // 设置加载状态(禁用所有部件)
    void setLoading(bool loading);
    // 对页码进行校验并设置按钮状态(<=0设置1,>totalPages设置totalPages)
    void checkPage(int &page,int &totalPages);
signals:
    void pageChanged(int page); // 转到页码信号
private:
    
    QLabel* pageInfoLabel_; // 分页信息标签
    QSpinBox* pageSpinBox_; // 页码选择框
    QPushButton* firstButton_; // 第一页按钮
    QPushButton* lastButton_; // 最后一页按钮
    QPushButton* prevButton_; // 上一页按钮
    QPushButton* nextButton_; // 下一页按钮
    QPushButton* goButton_; // 转到按钮
};