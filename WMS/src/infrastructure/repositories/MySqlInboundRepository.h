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
    static int linesColumns;
    static int detailHeaderColumns;
    static int detailLineColumns;
    explicit MySqlInboundRepository(DatabaseExecutor& executor, QObject* parent = nullptr);
    ~MySqlInboundRepository() = default;
    // 创建草稿订单
    void createDraft(
        const InboundOrder& order,
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
        const InboundOrderFilter& filter,
        const PageRequest& request,
        QObject* owner,
        PageCallback callback) override;
    // 确认订单
    void confirmOrder(
        quint32 id,
        const AuditContext& auditContext,
        QObject* owner,
        OperateCallback callback) override;
    // 获取订单详情
    void getOrderDetail(
        quint32 id,
        QObject* owner,
        DetailCallback callback) override;
    // 将数据库错误映射为应用错误
    // operationContext: 操作上下文标识(如"createDraft"、"confirmOrder"),为空时仅按错误码映射
    // failedStatementIndex: 失败语句在事务中的索引,<0时忽略精确映射
    static AppError mapDatabaseErrorToAppError(
        const DatabaseError& error,
        const QString& operationContext = QString(),
        int failedStatementIndex = -1);
    // 将数据库中的订单状态(字符串)转换为枚举
    static InboundOrderStatus mapDatabaseStatusToEnum(const QString& status = "draft");
    // 将枚举转换为数据库订单状态
    static QString mapEnumToDatabaseStatus(InboundOrderStatus status = InboundOrderStatus::Draft);

    // 将订单行(明细)映射为订单
    static std::optional<InboundOrderLine> mapInboundOrderLine(
        const QStringList& columns,
        const QVariantList& row);
    // 将订单头映射为订单头详情Dto
    static std::optional<InboundOrderDetailDto> mapInboundOrderDetailHeader(
        const QStringList& columns,
        const QVariantList& row);
    // 将订单行映射为订单行详情Dto
    static std::optional<InboundOrderDetailLineDto> mapInboundOrderDetailLine(
        const QStringList& columns,
        const QVariantList& row);
private slots:
    // 数据库任务完成
    void onTaskFinished(const DatabaseResult& result);


private:
    DatabaseExecutor& executor_;
    QHash<QUuid, PendingRequest> pending_;
};