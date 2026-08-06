#pragma once
#include "AppError.h"
#include "DatabaseExecutor.h"
#include "DatabaseTypes.h"
#include "IInboundRepository.h"
#include <QHash>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
class MySqlInboundRepository : public QObject, public IInboundRepository {
    Q_OBJECT
public:
    static int headerColumns;
    static int linesColumns ;
    explicit MySqlInboundRepository(DatabaseExecutor& executor, QObject* parent = nullptr);
    ~MySqlInboundRepository() = default;
    // 创建草稿订单
    void createDraft(
        const InboundOrder& order,
        QObject* owner,
        OperateCallback callback) override;
    // 根据id查询订单
    void findById(
        quint32 id,
        QObject* owner,
        OperateCallback callback) override;
    // 根据编号查询订单
    void findByOrderNo(
        const QString& orderNo,
        QObject* owner,
        OperateCallback callback) override;
    // 分页查询订单
    void listOrders(
        const InboundOrderFilter& filter,
        const PageRequest& request,
        QObject* owner,
        PageCallback callback) override;
    // 确认订单
    void confirmOrder(
        quint32 id,
        quint32 operatorId,
        QObject* owner,
        OperateCallback callback) override;
    // 将数据库错误映射为应用错误
    static AppError mapDatabaseErrorToAppError(const DatabaseError& error);
    // 将数据库中的订单状态(字符串)转换为枚举
    static InboundOrderStatus mapDatabaseStatusToEnum(const QString& status="draft");
    // 将枚举转换为数据库订单状态
    static QString mapEnumToDatabaseStatus(InboundOrderStatus status=InboundOrderStatus::Draft);
    
    private slots:
    // 数据库任务完成
    void onTaskFinished(const DatabaseResult& result);

private:
    DatabaseExecutor& executor_;
    // 将订单行(明细)映射为订单
    static std::optional<InboundOrderLine> mapInboundOrderLine(
        const QStringList& columns,
        const QVariantList& row);
    QHash<QUuid, PendingRequest> pending_;
};
