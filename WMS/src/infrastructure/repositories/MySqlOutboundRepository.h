#pragma once
#include "AppError.h"
#include "DatabaseExecutor.h"
#include "DatabaseTypes.h"
#include "IOutboundRepository.h"
#include <QHash>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
class MySqlOutboundRepository : public QObject, public IOutboundRepository {
    Q_OBJECT
public:
    static int headerColumns;
    static int linesColumns;
    explicit MySqlOutboundRepository(DatabaseExecutor& executor, QObject* parent = nullptr);
    ~MySqlOutboundRepository() = default;
    // 创建草稿订单
    void createDraft(
        const OutboundOrder& order,
        const AuditContext& auditContext,
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
        const OutboundOrderFilter& filter,
        const PageRequest& request,
        QObject* owner,
        PageCallback callback) override;
    // 确认订单(出库:先预读明细,再按行条件扣减库存,任一失败整体回滚)
    void confirmOrder(
        quint32 id,
        const AuditContext& auditContext,
        QObject* owner,
        OperateCallback callback) override;
    // 将数据库错误映射为应用错误
    // operationContext: 操作上下文标识(如"createDraft"、"confirmOrder"),为空时仅按错误码映射
    // failedStatementIndex: 失败语句在事务中的索引,<0时忽略精确映射
    // lineCount: 订单行数(仅 confirmOrder 用于区分"逐行扣减区间"与"审计日志"的失败索引),<0 时按宽松区间映射
    static AppError mapDatabaseErrorToAppError(
        const DatabaseError& error,
        const QString& operationContext = QString(),
        int failedStatementIndex = -1,
        int lineCount = -1);
    // 将数据库中的订单状态(字符串)转换为枚举
    static OutboundOrderStatus mapDatabaseStatusToEnum(const QString& status = "draft");
    // 将枚举转换为数据库订单状态
    static QString mapEnumToDatabaseStatus(OutboundOrderStatus status = OutboundOrderStatus::Draft);
    // 将订单行(明细)映射为订单
    static std::optional<OutboundOrderLine> mapOutboundOrderLine(
        const QStringList& columns,
        const QVariantList& row);
private slots:
    // 数据库任务完成
    void onTaskFinished(const DatabaseResult& result);

private:
    DatabaseExecutor& executor_;
    QHash<QUuid, PendingRequest> pending_;
};
