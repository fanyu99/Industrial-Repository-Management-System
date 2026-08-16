#include "MySqlOutboundRepository.h"
#include <QUuid>
#include <QVariantMap>

// TODO: 测试repository以及service

int MySqlOutboundRepository::headerColumns = 10;
int MySqlOutboundRepository::linesColumns = 5;
int MySqlOutboundRepository::detailHeaderColumns = 12;
int MySqlOutboundRepository::detailLineColumns = 6;
MySqlOutboundRepository::MySqlOutboundRepository(
    DatabaseExecutor& executor,
    QObject* parent)
    : executor_ { executor }
    , QObject(parent)
{
    // 连接任务执行器的taskFinished信号
    connect(&executor_, &DatabaseExecutor::taskFinished, this, &MySqlOutboundRepository::onTaskFinished);
}
// 映射数据库错误到应用错误
AppError MySqlOutboundRepository::mapDatabaseErrorToAppError(
    const DatabaseError& error,
    const QString& operationContext,
    int failedStatementIndex,
    int lineCount)
{
    // 有具体操作上下文时,进行精确错误映射
    if (!operationContext.isEmpty() && failedStatementIndex >= 0) {
        // confirmOrder: 确认出库订单
        if (operationContext == QStringLiteral("confirmOrder")) {
            // 语句[1]: 更新出库订单状态 → 订单不存在或状态不是草稿
            if (failedStatementIndex == 1 && error.code == DatabaseErrorCode::None) {
                return AppError {
                    AppErrorCategory::Validation,
                    AppErrorCode::InvalidOutboundOrder,
                    QStringLiteral("确认出库失败,订单不存在或状态不是草稿")
                };
            }
            // 语句[2..2+N-1]: 逐行条件扣减库存(无库存记录或库存不足均映射为业务错误)
            // 业务不满足预期(affectedRows==0)不是数据库执行错误
            if (failedStatementIndex >= 2
                && (lineCount < 0 || failedStatementIndex < 2 + lineCount)
                && error.code == DatabaseErrorCode::None) {
                return AppError::insufficientStock();
            }
            // 语句[3+N]: 写入审计日志失败(内部完整性错误)
            if (lineCount >= 0 && failedStatementIndex == 3 + lineCount
                && error.code == DatabaseErrorCode::None) {
                return AppError::repositoryFailure(QStringLiteral("确认出库失败,审计日志写入异常"));
            }
            // 语句[2+N]: 库存流水唯一键冲突(uk_movement_idem)或编号冲突
            if (error.nativeErrorCode == QStringLiteral("1062")) {
                if (error.databaseText.contains(QStringLiteral("uk_movement_idem")))
                    return AppError::databaseFailure(QStringLiteral("库存流水编号已存在,请勿重复确认"));
                else
                    return AppError::databaseFailure(QStringLiteral("库存流水编号冲突,请重试"));
            }
        }
        // createDraft: 创建草稿订单
        if (operationContext == QStringLiteral("createDraft")) {
            // 语句[2]: INSERT INTO outbound_orders (order_no 唯一键冲突)
            if (failedStatementIndex == 2 && error.nativeErrorCode == QStringLiteral("1062")) {
                return AppError::databaseFailure(QStringLiteral("订单编号已存在，请勿重复创建"));
            }
            // 外键约束: product_id 不存在
            if (error.nativeErrorCode == QStringLiteral("1452") || error.nativeErrorCode == QStringLiteral("23000")) {
                return AppError::databaseFailure(QStringLiteral("创建草稿订单失败,订单行中的产品ID不存在或已停用"));
            }
        }
    }
    // 一般错误
    switch (error.code) {
    case DatabaseErrorCode::QueueFull:
        return AppError::repositoryFailure(QStringLiteral("数据库任务队列已满,请稍后重试"));
    case DatabaseErrorCode::InvalidTask:
        return AppError::repositoryFailure(QStringLiteral("任务无效"));
    case DatabaseErrorCode::ShuttingDown:
        return AppError {
            AppErrorCategory::System,
            AppErrorCode::UnknownError,
            QStringLiteral("系统正在关闭,请稍后重试")
        };
    case DatabaseErrorCode::PrepareFailed:
    case DatabaseErrorCode::ExecuteFailed:
    case DatabaseErrorCode::TransactionFailed: {
        if (error.nativeErrorCode == "1062")
            return AppError::databaseFailure(QStringLiteral("目标已存在:%1").arg(error.databaseText));
        return AppError::databaseFailure(QStringLiteral("数据库操作失败:%1").arg(error.databaseText));
    }
    case DatabaseErrorCode::Cancelled:
        return AppError::databaseFailure(QStringLiteral("操作已取消"));
    case DatabaseErrorCode::ConnectionFailed:
        return AppError::databaseFailure(QStringLiteral("数据库连接失败"));
    case DatabaseErrorCode::ResultTooLarge:
        return AppError::databaseFailure(QStringLiteral("查询结果集过大"));
    default:
        return AppError {
            AppErrorCategory::Database,
            AppErrorCode::UnknownError,
            QStringLiteral("数据库错误")
        };
    }
}
// 映射数据库订单状态(字符串)到枚举值
OutboundOrderStatus MySqlOutboundRepository::mapDatabaseStatusToEnum(const QString& status)
{
    if (status == "draft")
        return OutboundOrderStatus::Draft;
    if (status == "confirmed")
        return OutboundOrderStatus::Confirmed;
    if (status == "cancelled")
        return OutboundOrderStatus::Cancelled;
    return OutboundOrderStatus::Draft; // 默认草稿
}
// 映射枚举值到数据库订单状态
QString MySqlOutboundRepository::mapEnumToDatabaseStatus(OutboundOrderStatus status)
{
    if (status == OutboundOrderStatus::Draft)
        return QString("draft");
    if (status == OutboundOrderStatus::Confirmed)
        return QString("confirmed");
    if (status == OutboundOrderStatus::Cancelled)
        return QString("cancelled");
    return QString("draft"); // 默认草稿
}
// 映射订单行(明细)到订单行结构体
std::optional<OutboundOrderLine> MySqlOutboundRepository::mapOutboundOrderLine(
    const QStringList& columns,
    const QVariantList& row)
{
    if (columns.size() != MySqlOutboundRepository::linesColumns)
        return std::nullopt;
    OutboundOrderLine line;
    for (int i = 0; i < MySqlOutboundRepository::linesColumns; ++i) {
        const QString& colName = columns[i];
        if (colName == "lineId") {
            line.id = row[i].toUInt();
        } else if (colName == "orderId") {
            line.orderId = row[i].toUInt();
        } else if (colName == "productId") {
            line.productId = row[i].toUInt();
        } else if (colName == "quantity") {
            line.quantity = row[i].toInt();
        } else if (colName == "unitPrice") {
            line.unitPrice = row[i].toDouble();
        }
    }
    if (line.id == 0 || line.orderId == 0 || line.productId == 0 || line.quantity <= 0 || line.unitPrice < 0.0)
        return std::nullopt;
    return line;
}
// 执行器任务完成后处理结果
void MySqlOutboundRepository::onTaskFinished(const DatabaseResult& result)
{
    // 找到对应的请求和回调函数
    auto ite = pending_.find(result.requestId);
    if (ite == pending_.end()) {
        return;
    }
    auto pending = std::move(ite.value());
    pending_.erase(ite); // 移除
    if (pending.owner.isNull())
        return;
    // 处理结果
    if (pending.handler)
        pending.handler(result);
}
// 根据ID查询出库订单
void MySqlOutboundRepository::findById(
    quint32 id,
    QObject* owner,
    OperateCallback callback)
{
    // 校验参数
    QPointer<QObject> ownerPtr(owner);
    if (ownerPtr.isNull() || !callback)
        return;
    if (id == 0) {
        callback(OutboundOperationResult {
            false,
            std::nullopt,
            AppError {
                AppErrorCategory::Validation,
                AppErrorCode::InvalidOutboundOrder,
                QStringLiteral("订单ID无效") } }

        );
        return;
    }
    // 创建事务型语句(一句查订单Header,一句查Lines)
    DatabaseTask task;
    task.type = DatabaseTaskType::Transaction;
    task.requestId = QUuid::createUuid();

    // 1.查询Header
    DatabaseStatement statement1;
    statement1.type = StatementType::Query;
    statement1.sql = QStringLiteral(
        "SELECT id,order_no as orderNo,recipient,status,operator_id as operatorId,warehouse_id as warehouseId,remark,created_at as createdAt,updated_at as updatedAt,confirmed_at as confirmedAt FROM outbound_orders WHERE id = :id");
    statement1.parameters.insert("id", id);
    task.statements.append(statement1);

    // 2.查询Lines
    DatabaseStatement statement2;
    statement2.type = StatementType::Query;
    statement2.sql = QStringLiteral(
        "SELECT id as lineId,order_id as orderId,product_id as productId,quantity,unit_price as unitPrice FROM outbound_details WHERE order_id = :id");
    statement2.parameters.insert("id", id);
    task.statements.append(statement2);

    // 包装lambda回调函数处理查询结果
    pending_.insert(task.requestId, PendingRequest { ownerPtr, [ownerPtr, callback = std::move(callback), this, id](const DatabaseResult& result) {
                                                        // 校验参数
                                                        if (ownerPtr.isNull() || !callback)
                                                            return;

                                                        // 查询失败
                                                        if (!result.isSucceeded()) {
                                                            callback(OutboundOperationResult {
                                                                false,
                                                                std::nullopt,
                                                                mapDatabaseErrorToAppError(result.error) });
                                                            return;
                                                        }
                                                        // 如果查询结果小于2,查询失败
                                                        if (result.statementResults.size() != 2) {
                                                            callback(OutboundOperationResult {
                                                                false,
                                                                std::nullopt,
                                                                AppError::repositoryFailure(QStringLiteral("查询出库订单失败")) });
                                                            return;
                                                        }
                                                        // 查询成功
                                                        OutboundOrder order; // 订单
                                                        const auto& headerResult = result.statementResults[0]; // Header结果
                                                        const auto& linesResult = result.statementResults[1]; // Lines结果

                                                        // 处理Header结果
                                                        if (headerResult.columns.size() != MySqlOutboundRepository::headerColumns) {
                                                            callback(OutboundOperationResult {
                                                                false,
                                                                std::nullopt,
                                                                AppError::repositoryFailure(QStringLiteral("查询出库订单Header失败,列数错误,订单(%1号)").arg(id)) });
                                                            return;
                                                        }
                                                        if (headerResult.rows.size() != 0) {
                                                            for (int i = 0; i < MySqlOutboundRepository::headerColumns; ++i) {
                                                                const QString& colName = headerResult.columns[i];
                                                                if (colName == "id")
                                                                    order.id = headerResult.rows[0].value(i).toUInt();
                                                                if (colName == "orderNo")
                                                                    order.orderNo = headerResult.rows[0].value(i).toString();
                                                                if (colName == "recipient")
                                                                    order.recipient = headerResult.rows[0].value(i).toString();
                                                                if (colName == "status") {
                                                                    order.status = mapDatabaseStatusToEnum(headerResult.rows[0].value(i).toString());
                                                                }
                                                                if (colName == "operatorId")
                                                                    order.operatorId = headerResult.rows[0].value(i).toUInt();
                                                                if (colName == "warehouseId")
                                                                    order.warehouseId = headerResult.rows[0].value(i).toUInt();
                                                                if (colName == "remark")
                                                                    order.remark = headerResult.rows[0].value(i).toString();
                                                                if (colName == "confirmedAt") {
                                                                    // 确认时间可能为空,为空设置为默认的std::nullopt
                                                                    if (!headerResult.rows[0].value(i).isNull())
                                                                        order.confirmedAt = headerResult.rows[0].value(i).toDateTime();
                                                                }
                                                                if (colName == "createdAt")
                                                                    order.createdAt = headerResult.rows[0].value(i).toDateTime();
                                                                if (colName == "updatedAt")
                                                                    order.updatedAt = headerResult.rows[0].value(i).toDateTime();
                                                            }
                                                        }
                                                        // 处理Lines结果
                                                        if (linesResult.columns.size() != MySqlOutboundRepository::linesColumns) {
                                                            callback(OutboundOperationResult {
                                                                false,
                                                                std::nullopt,
                                                                AppError::repositoryFailure(QStringLiteral("查询出库订单Lines失败,列数错误,订单(%1号)").arg(order.id)) });
                                                            return;
                                                        }
                                                        if (linesResult.rows.size() == 0 && headerResult.rows.size() != 0) { // Lines结果为空,Header结果不为空,则查询失败
                                                            callback(OutboundOperationResult {
                                                                false,
                                                                std::nullopt,
                                                                AppError::repositoryFailure(QStringLiteral("查询出库订单Lines失败,Lines结果为空,订单(%1号)数据空").arg(order.id)) });
                                                            return;
                                                        } else if (linesResult.rows.size() != 0 && headerResult.rows.size() == 0) { // Lines结果不为空,Header结果为空,则查询失败
                                                            callback(OutboundOperationResult {
                                                                false,
                                                                std::nullopt,
                                                                AppError::repositoryFailure(QStringLiteral("查询出库订单Lines失败,Lines结果不为空,Header结果为空,订单(%1号)数据空").arg(order.id)) });
                                                            return;
                                                        } else if (linesResult.rows.size() != 0 && headerResult.rows.size() != 0) { // 都不为空,则查询成功
                                                            // 遍历所有line
                                                            for (int i = 0; i < linesResult.rows.size(); ++i) {
                                                                std::optional<OutboundOrderLine> line = mapOutboundOrderLine(linesResult.columns, linesResult.rows[i]);
                                                                if (!line.has_value()) {
                                                                    callback(
                                                                        OutboundOperationResult {
                                                                            false,
                                                                            std::nullopt,
                                                                            AppError::repositoryFailure(QStringLiteral("查询出库订单Lines失败,订单(%1号)第%2行数据转换错误").arg(order.id).arg(i + 1)) });
                                                                    return;
                                                                }
                                                                order.lines.append(line.value());
                                                            }
                                                        }
                                                        // 如果Lines结果为空,Header结果为空,查询任务成功,但查询结果空
                                                        else {
                                                            callback(
                                                                OutboundOperationResult {
                                                                    true,
                                                                    std::nullopt,
                                                                    std::nullopt });
                                                            return;
                                                        }

                                                        // 最后统一回调
                                                        callback(OutboundOperationResult {
                                                            true,
                                                            std::make_optional<OutboundOrder>(order),
                                                            std::nullopt });
                                                    } });
    executor_.submitTask(task); // 最后提交任务
}
// 根据编号查询出库订单
void MySqlOutboundRepository::findByOrderNo(
    const QString& orderNo,
    QObject* owner,
    OperateCallback callback)
{
    // 校验参数
    QPointer<QObject> ownerPtr(owner);
    if (ownerPtr.isNull() || !callback)
        return;
    if (orderNo.trimmed().isEmpty()) {
        callback(OutboundOperationResult {
            false,
            std::nullopt,
            AppError {
                AppErrorCategory::Validation,
                AppErrorCode::InvalidOutboundOrder,
                QStringLiteral("订单编号不能为空") } });
        return;
    }
    // 创建事务型语句(一句查订单Header,一句查Lines)
    DatabaseTask task;
    task.type = DatabaseTaskType::Transaction;
    task.requestId = QUuid::createUuid();

    // 1.查询Header
    DatabaseStatement statement1;
    statement1.type = StatementType::Query;
    statement1.sql = QStringLiteral(
        "SELECT id,order_no as orderNo,recipient,status,operator_id as operatorId,warehouse_id as warehouseId,remark,created_at as createdAt,updated_at as updatedAt,confirmed_at as confirmedAt FROM outbound_orders WHERE order_no = :orderNo");
    statement1.parameters.insert("orderNo", orderNo.trimmed());
    task.statements.append(statement1);

    // 2.查询Lines
    DatabaseStatement statement2;
    statement2.type = StatementType::Query;
    statement2.sql = QStringLiteral(
        "SELECT id as lineId,order_id as orderId,product_id as productId,quantity,unit_price as unitPrice FROM outbound_details WHERE order_id = (SELECT id FROM outbound_orders WHERE order_no = :orderNo)");
    statement2.parameters.insert("orderNo", orderNo.trimmed());
    task.statements.append(statement2);

    // 包装lambda回调函数处理查询结果
    pending_.insert(task.requestId, PendingRequest { ownerPtr, [ownerPtr, callback = std::move(callback), orderNo = orderNo.trimmed(), this](const DatabaseResult& result) {
                                                        // 校验参数
                                                        if (ownerPtr.isNull() || !callback)
                                                            return;
                                                        // 查询失败
                                                        if (!result.isSucceeded()) {
                                                            callback(OutboundOperationResult {
                                                                false,
                                                                std::nullopt,
                                                                mapDatabaseErrorToAppError(result.error) });
                                                            return;
                                                        }
                                                        // 如果查询结果不等于2,查询失败
                                                        if (result.statementResults.size() != 2) {
                                                            callback(OutboundOperationResult {
                                                                false,
                                                                std::nullopt,
                                                                AppError::repositoryFailure(QStringLiteral("查询出库订单失败")) });
                                                            return;
                                                        }
                                                        // 查询成功
                                                        OutboundOrder order;
                                                        const auto& headerResult = result.statementResults[0];
                                                        const auto& linesResult = result.statementResults[1];

                                                        // 处理Header结果
                                                        if (headerResult.columns.size() != MySqlOutboundRepository::headerColumns) {
                                                            callback(OutboundOperationResult {
                                                                false,
                                                                std::nullopt,
                                                                AppError::repositoryFailure(QStringLiteral("查询出库订单Header失败,列数错误,订单(%1号)").arg(orderNo)) });
                                                            return;
                                                        }
                                                        if (headerResult.rows.size() != 0) {
                                                            for (int i = 0; i < MySqlOutboundRepository::headerColumns; ++i) {
                                                                const QString& colName = headerResult.columns[i];
                                                                if (colName == "id")
                                                                    order.id = headerResult.rows[0].value(i).toUInt();
                                                                if (colName == "orderNo")
                                                                    order.orderNo = headerResult.rows[0].value(i).toString();
                                                                if (colName == "recipient")
                                                                    order.recipient = headerResult.rows[0].value(i).toString();
                                                                if (colName == "status") {
                                                                    order.status = mapDatabaseStatusToEnum(headerResult.rows[0].value(i).toString());
                                                                }
                                                                if (colName == "operatorId")
                                                                    order.operatorId = headerResult.rows[0].value(i).toUInt();
                                                                if (colName == "warehouseId")
                                                                    order.warehouseId = headerResult.rows[0].value(i).toUInt();
                                                                if (colName == "remark")
                                                                    order.remark = headerResult.rows[0].value(i).toString();
                                                                if (colName == "confirmedAt") {
                                                                    if (!headerResult.rows[0].value(i).isNull())
                                                                        order.confirmedAt = headerResult.rows[0].value(i).toDateTime();
                                                                }
                                                                if (colName == "createdAt")
                                                                    order.createdAt = headerResult.rows[0].value(i).toDateTime();
                                                                if (colName == "updatedAt")
                                                                    order.updatedAt = headerResult.rows[0].value(i).toDateTime();
                                                            }
                                                        }
                                                        // 处理Lines结果
                                                        if (linesResult.columns.size() != MySqlOutboundRepository::linesColumns) {
                                                            callback(OutboundOperationResult {
                                                                false,
                                                                std::nullopt,
                                                                AppError::repositoryFailure(QStringLiteral("查询出库订单Lines失败,列数错误,订单(%1号)").arg(order.orderNo)) });
                                                            return;
                                                        }
                                                        if (linesResult.rows.size() == 0 && headerResult.rows.size() != 0) {
                                                            callback(OutboundOperationResult {
                                                                false,
                                                                std::nullopt,
                                                                AppError::repositoryFailure(QStringLiteral("查询出库订单Lines失败,Lines结果为空,订单(%1号)数据空").arg(order.orderNo)) });
                                                            return;
                                                        } else if (linesResult.rows.size() != 0 && headerResult.rows.size() == 0) {
                                                            callback(OutboundOperationResult {
                                                                false,
                                                                std::nullopt,
                                                                AppError::repositoryFailure(QStringLiteral("查询出库订单Lines失败,Lines结果不为空,Header结果为空,订单(%1号)数据空").arg(order.orderNo)) });
                                                            return;
                                                        } else if (linesResult.rows.size() != 0 && headerResult.rows.size() != 0) {
                                                            for (int i = 0; i < linesResult.rows.size(); ++i) {
                                                                const auto line = mapOutboundOrderLine(linesResult.columns, linesResult.rows[i]);
                                                                if (!line.has_value()) {
                                                                    callback(
                                                                        OutboundOperationResult {
                                                                            false,
                                                                            std::nullopt,
                                                                            AppError::repositoryFailure(QStringLiteral("查询出库订单Lines失败,订单(%1号)第%2行数据转换错误").arg(order.orderNo).arg(i + 1)) });
                                                                    return;
                                                                }
                                                                order.lines.append(line.value());
                                                            }
                                                        }
                                                        // Lines和Header都为空,查询成功但无数据
                                                        else {
                                                            callback(
                                                                OutboundOperationResult {
                                                                    true,
                                                                    std::nullopt,
                                                                    std::nullopt });
                                                            return;
                                                        }

                                                        // 最后统一回调
                                                        callback(OutboundOperationResult {
                                                            true,
                                                            std::make_optional<OutboundOrder>(order),
                                                            std::nullopt });
                                                    } });
    executor_.submitTask(task);
}
// 分页查询出库订单
void MySqlOutboundRepository::listOrders(
    const OutboundOrderFilter& filter,
    const PageRequest& request,
    QObject* owner,
    PageCallback callback)
{
    QPointer<QObject> ownerPtr(owner);
    if (ownerPtr.isNull() || !callback) {
        return;
    }
    if (request.page <= 0 || request.pageSize <= 0) {
        callback(OutboundPageResult {
            false,
            {},
            AppError::repositoryFailure(QStringLiteral("分页查询出库订单失败,分页参数错误")) });
        return;
    }
    // 创建事务型任务
    DatabaseTask task;
    task.type = DatabaseTaskType::Transaction;
    task.requestId = QUuid::createUuid();
    // from语句
    const QString fromSql = QStringLiteral(R"(
    FROM outbound_orders o
    JOIN warehouses w ON w.id = o.warehouse_id
    JOIN users u ON u.id = o.operator_id
)");
    // 构建where查询条件
    QStringList whereConditions; // 条件
    QVariantMap parametersMap; // 参数映射
    if (!filter.keyword.trimmed().isEmpty()) { // 关键字
        whereConditions << QStringLiteral("(o.order_no LIKE :keyword OR o.recipient LIKE :keyword OR o.remark LIKE :keyword OR u.real_name LIKE :keyword OR w.name LIKE :keyword OR w.code LIKE :keyword OR u.username LIKE :keyword) ");
        parametersMap.insert("keyword", "%" + filter.keyword.trimmed() + "%");
    }
    if (filter.status.has_value()) { // 状态
        whereConditions << QStringLiteral("o.status = :status");
        parametersMap.insert("status", mapEnumToDatabaseStatus(filter.status.value()));
    }
    if (filter.warehouseId.has_value()) { // 仓库id
        whereConditions << QStringLiteral("o.warehouse_id = :warehouseId");
        parametersMap.insert("warehouseId", filter.warehouseId.value());
    }
    QString whereResult = whereConditions.isEmpty() ? QStringLiteral("") : QStringLiteral("WHERE ") + whereConditions.join(QStringLiteral(" AND "));
    // 1.查询总记录数
    DatabaseStatement statement1;
    statement1.type = StatementType::Query;
    statement1.sql = QStringLiteral("SELECT COUNT(*) as total ") + fromSql + whereResult;
    statement1.parameters = parametersMap;
    task.statements.append(statement1);
    // 2.查询分页数据
    DatabaseStatement statement2;
    statement2.type = StatementType::Query;
    statement2.sql = QStringLiteral(R"(
        SELECT
            o.id AS id,
            o.order_no AS orderNo,
            o.recipient,
            o.status,
            o.warehouse_id AS warehouseId,
            w.name AS warehouseName,
            o.operator_id AS operatorId,
            u.real_name AS operatorName,
            COUNT(d.id) AS lineCount,
            COALESCE(SUM(d.quantity), 0) AS totalQuantity,
            o.created_at AS createdAt,
            o.updated_at AS updatedAt,
            o.confirmed_at AS confirmedAt )")
        + fromSql + QStringLiteral(R"(
        LEFT JOIN outbound_details d ON d.order_id = o.id
    )") + whereResult
        + QStringLiteral(R"(
        GROUP BY o.id, o.order_no, o.recipient, o.status, o.warehouse_id, w.name, o.operator_id, u.real_name, o.created_at, o.updated_at, o.confirmed_at
        ORDER BY o.created_at DESC,o.id DESC
        LIMIT :limit OFFSET :offset
    )");
    statement2.parameters = parametersMap;
    statement2.parameters.insert("offset", (request.page - 1) * request.pageSize);
    statement2.parameters.insert("limit", request.pageSize);
    task.statements.append(statement2);

    // 包装lambda回调函数
    pending_.insert(task.requestId, PendingRequest { ownerPtr, [ownerPtr, request, callback = std::move(callback)](const DatabaseResult& result) {
                                                        if (ownerPtr.isNull() || !callback) {
                                                            return;
                                                        }
                                                        if (!result.isSucceeded()) {
                                                            callback(OutboundPageResult {
                                                                false,
                                                                {},
                                                                mapDatabaseErrorToAppError(result.error) });
                                                            return;
                                                        }
                                                        // 如果查询结果小于2,查询失败
                                                        if (result.statementResults.size() != 2) {
                                                            callback(OutboundPageResult {
                                                                false,
                                                                {},
                                                                AppError::repositoryFailure(QStringLiteral("查询出库订单分页数据失败")) });
                                                            return;
                                                        }
                                                        // 1.总记录数
                                                        const auto& countResult = result.statementResults[0];
                                                        if (countResult.rows.isEmpty() || countResult.columns.size() != 1 || countResult.rows.constFirst().isEmpty()) { // 如果获取总记录数的结果空
                                                            callback(OutboundPageResult {
                                                                false,
                                                                {},
                                                                AppError::repositoryFailure(QStringLiteral("查询出库订单分页数据为空/异常")) });
                                                            return;
                                                        }
                                                        int total = countResult.rows.constFirst().value(0).toInt(); // 总记录数
                                                        // 2. 获取分页数据
                                                        const auto& pageResult = result.statementResults[1]; // 分页数据
                                                        QVector<OutboundOrderListItemDto> items; // 分页订单列表
                                                        for (int i = 0; i < pageResult.rows.size(); ++i) {
                                                            const auto& row = pageResult.rows[i];
                                                            OutboundOrderListItemDto dto;
                                                            if (row.size() != pageResult.columns.size()) {
                                                                callback(OutboundPageResult {
                                                                    false, {},
                                                                    AppError::repositoryFailure(QStringLiteral("查询出库订单分页数据列数不匹配")) });
                                                                return;
                                                            }
                                                            for (int j = 0; j < pageResult.columns.size(); ++j) {
                                                                const QString& colName = pageResult.columns[j];
                                                                if (colName == "id")
                                                                    dto.id = row.value(j).toInt();

                                                                if (colName == "orderNo")
                                                                    dto.orderNo = row.value(j).toString();
                                                                if (colName == "recipient")
                                                                    dto.recipient = row.value(j).toString();
                                                                if (colName == "status")
                                                                    dto.status = mapDatabaseStatusToEnum(row.value(j).toString());
                                                                if (colName == "warehouseId")
                                                                    dto.warehouseId = row.value(j).toInt();
                                                                if (colName == "warehouseName")
                                                                    dto.warehouseName = row.value(j).toString();
                                                                if (colName == "operatorId")
                                                                    dto.operatorId = row.value(j).toInt();
                                                                if (colName == "operatorName")
                                                                    dto.operatorName = row.value(j).toString();
                                                                if (colName == "lineCount")
                                                                    dto.lineCount = row.value(j).toInt();
                                                                if (colName == "totalQuantity")
                                                                    dto.totalQuantity = row.value(j).toInt();
                                                                if (colName == "createdAt")
                                                                    dto.createdAt = row.value(j).toDateTime();
                                                                if (colName == "updatedAt")
                                                                    dto.updatedAt = row.value(j).toDateTime();
                                                                if (colName == "confirmedAt") {
                                                                    if (!row.value(j).isNull())
                                                                        dto.confirmedAt = row.value(j).toDateTime();
                                                                }
                                                            }
                                                            // 校验数据是否正常
                                                            if (dto.id == 0 || dto.orderNo.trimmed().isEmpty() || dto.recipient.trimmed().isEmpty() || dto.operatorId == 0 || dto.warehouseId == 0 || dto.lineCount < 0 || dto.totalQuantity <= 0 || dto.operatorName.trimmed().isEmpty() || dto.warehouseName.trimmed().isEmpty()) {
                                                                callback(OutboundPageResult {
                                                                    false,
                                                                    {},
                                                                    AppError::repositoryFailure(QStringLiteral("订单分页数据映射异常")) });
                                                                return;
                                                            }
                                                            items.append(dto);
                                                        }
                                                        // 封装最后的分页结果
                                                        PageResult<OutboundOrderListItemDto> page;
                                                        page.items = std::move(items);
                                                        page.total = total;
                                                        page.page = request.page;
                                                        page.pageSize = request.pageSize;
                                                        callback(OutboundPageResult {
                                                            true,
                                                            page,
                                                            std::nullopt });
                                                    } });

    // 最后提交任务
    executor_.submitTask(task);
}
// 创建草稿订单(注意:需要生成有效的OrderNo和订单ID)
void MySqlOutboundRepository::createDraft(
    const OutboundOrder& order,
    const AuditContext& auditContext,
    QObject* owner,
    OperateCallback callback)
{
    // 检验参数
    QPointer<QObject> ownerPtr(owner);
    if (ownerPtr.isNull() || !callback)
        return;
    if (order.recipient.trimmed().isEmpty() || order.operatorId == 0 || order.warehouseId == 0 || order.lines.isEmpty()) {
        callback(
            OutboundOperationResult {
                false,
                std::nullopt,
                AppError::repositoryFailure(QStringLiteral("创建草稿订单失败,订单参数错误")) });
        return;
    }
    // 生成订单号: OUT-YYYYMMDD-NNNNNN 日期+序号
    const QString prefix = QStringLiteral("OUT-%1-").arg(
        QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd")));
    // 日期中最大订单序号+1,作为新订单的序号
    DatabaseStatement statement_seq;
    statement_seq.type = StatementType::Command;
    statement_seq.sql = QStringLiteral(
        "SET @seq = (SELECT COALESCE(MAX(CAST(SUBSTRING_INDEX(order_no, '-', -1) AS UNSIGNED)), 0) + 1 "
        "FROM outbound_orders WHERE order_no LIKE :prefixPattern)");
    statement_seq.parameters.insert("prefixPattern", prefix + QStringLiteral("%"));
    // 生成并设置订单号
    DatabaseStatement statement_orderno;
    statement_orderno.type = StatementType::Command;
    statement_orderno.sql = QStringLiteral(
        "SET @gen_order_no = CONCAT(:prefix, LPAD(@seq, 6, '0'))");
    statement_orderno.parameters.insert("prefix", prefix);

    // 创建插入outbound_orders表语句
    DatabaseStatement statement1;
    statement1.type = StatementType::Command;
    statement1.sql = QStringLiteral("INSERT INTO outbound_orders(order_no,recipient,status,operator_id,warehouse_id,remark) VALUES(@gen_order_no,:recipient,:status,:operatorId,:warehouseId,:remark)");
    statement1.parameters.insert("recipient", order.recipient);
    statement1.parameters.insert("status", QStringLiteral("draft"));
    statement1.parameters.insert("operatorId", order.operatorId);
    statement1.parameters.insert("warehouseId", order.warehouseId);
    statement1.parameters.insert("remark", order.remark);
    // 设置插入订单的ID
    DatabaseStatement statement2;
    statement2.type = StatementType::Command;
    statement2.sql = QStringLiteral("SET @new_order_id = LAST_INSERT_ID();");

    // 获取生成的订单号
    DatabaseStatement statement_generated;
    statement_generated.type = StatementType::Query;
    statement_generated.sql = QStringLiteral("SELECT @gen_order_no AS generatedOrderNo");
    // 创建事务型任务
    DatabaseTask task;
    task.type = DatabaseTaskType::Transaction;
    task.requestId = QUuid::createUuid();
    task.statements.append(statement_seq);
    task.statements.append(statement_orderno);
    task.statements.append(statement1);
    task.statements.append(statement2);
    task.statements.append(statement_generated);
    // 插入outbound_details表语句
    for (const auto& line : order.lines) {
        DatabaseStatement lineStatement;
        lineStatement.type = StatementType::Command;
        lineStatement.sql = QStringLiteral("INSERT INTO outbound_details(order_id,product_id,quantity,unit_price) SELECT @new_order_id,:productId,:quantity,:unitPrice FROM DUAL WHERE EXISTS (SELECT 1 FROM products WHERE id = :productId)");
        lineStatement.parameters.insert("productId", line.productId);
        lineStatement.parameters.insert("quantity", line.quantity);
        lineStatement.parameters.insert("unitPrice", line.unitPrice);
        if (line.quantity <= 0 || line.unitPrice < 0 || line.productId == 0) {
            callback(
                OutboundOperationResult {
                    false,
                    std::nullopt,
                    AppError::repositoryFailure(QStringLiteral("创建草稿订单失败,订单行参数错误")) });
            return;
        }
        task.statements.append(lineStatement);
    }
    // 审计日志写入语句
    DatabaseStatement statementAudit;
    statementAudit.type = StatementType::Command;
    statementAudit.sql = QStringLiteral(
        "INSERT INTO audit_logs "
        "(operator_id, username, action, target_type, target_id, detail, created_at) "
        "SELECT :operatorId, u.username, 'create', 'outbound', CAST(@new_order_id AS CHAR), "
        "JSON_OBJECT('module', 'outbound', 'orderNo', @gen_order_no, 'recipient', :recipient, 'warehouseId', :warehouseId, 'action', 'create'), "
        "CURRENT_TIMESTAMP(3) "
        "FROM users u "
        "WHERE u.id = :operatorId");
    statementAudit.parameters.insert("operatorId", auditContext.operatorId);
    statementAudit.parameters.insert("recipient", order.recipient);
    statementAudit.parameters.insert("warehouseId", order.warehouseId);
    task.statements.append(statementAudit);
    // 包装lambda函数
    pending_.insert(task.requestId, PendingRequest { ownerPtr, [ownerPtr, callback = std::move(callback), order, this, task](const DatabaseResult& result) {
                                                        if (ownerPtr.isNull() || !callback)
                                                            return;
                                                        if (!result.isSucceeded()) {
                                                            callback(OutboundOperationResult {
                                                                false,
                                                                std::nullopt, mapDatabaseErrorToAppError(result.error, QStringLiteral("createDraft"), result.failedStatementIndex) });
                                                            return;
                                                        }
                                                        // 校验结果
                                                        if (result.statementResults.size() != 6 + order.lines.size()) {
                                                            callback(OutboundOperationResult {
                                                                false,
                                                                std::nullopt,
                                                                AppError::repositoryFailure(QStringLiteral("创建草稿订单失败,订单创建结果数量错误")) });
                                                            return;
                                                        }
                                                        if (result.statementResults[2].lastInsertId.toInt() == 0) {
                                                            callback(OutboundOperationResult {
                                                                false,
                                                                std::nullopt,
                                                                AppError::repositoryFailure(QStringLiteral("创建草稿订单失败,订单ID异常")) });
                                                            return;
                                                        }
                                                        if (result.statementResults[2].affectedRows != 1) {
                                                            callback(OutboundOperationResult {
                                                                false,
                                                                std::nullopt,
                                                                AppError::repositoryFailure(QStringLiteral("创建草稿订单失败,订单ID插入失败")) });
                                                            return;
                                                        }
                                                        for (int i = 5; i < result.statementResults.size() - 1; ++i) {
                                                            const auto& lineResult = result.statementResults[i];
                                                            if (lineResult.affectedRows != 1) {
                                                                const int lineIndex = i - 5;
                                                                const quint32 productId = (lineIndex >= 0 && lineIndex < static_cast<int>(order.lines.size()))
                                                                    ? order.lines[lineIndex].productId
                                                                    : 0;
                                                                const QString errorMsg = productId > 0
                                                                    ? QStringLiteral("创建草稿订单失败,产品ID %1 不存在或已停用").arg(productId)
                                                                    : QStringLiteral("创建草稿订单失败,订单行插入失败");
                                                                callback(OutboundOperationResult {
                                                                    false,
                                                                    std::nullopt,
                                                                    AppError::repositoryFailure(errorMsg) });
                                                                return;
                                                            }
                                                        }
                                                        // 校验审计日志写入
                                                        const auto& auditResult = result.statementResults.last();
                                                        if (auditResult.affectedRows != 1) {
                                                            callback(OutboundOperationResult {
                                                                false,
                                                                std::nullopt,
                                                                AppError::repositoryFailure(QStringLiteral("创建草稿订单失败,审计日志写入异常")) });
                                                            return;
                                                        }
                                                        // 从SELECT结果中读取生成的订单号
                                                        const auto& genResult = result.statementResults[4];
                                                        const QString orderNoStr = (genResult.rows.size() == 1 && genResult.rows[0].length() >= 1)
                                                            ? genResult.rows[0].value(0).toString()
                                                            : QString();
                                                        if (orderNoStr.isEmpty()) {
                                                            callback(OutboundOperationResult {
                                                                false,
                                                                std::nullopt,
                                                                AppError::repositoryFailure(QStringLiteral("创建草稿订单失败,无法获取生成的订单号/订单号为空")) });
                                                            return;
                                                        }
                                                        // 回查订单
                                                        findByOrderNo(orderNoStr, ownerPtr.data(), [ownerPtr, callback = std::move(callback)](const OutboundOperationResult& result) {
                                                            if (ownerPtr.isNull() || !callback)
                                                                return;
                                                            if (!result.success) {
                                                                callback(OutboundOperationResult {
                                                                    false, std::nullopt,
                                                                    result.error.has_value() ? result.error.value() : AppError::repositoryFailure(QStringLiteral("创建草稿订单失败,订单创建回查失败")) });
                                                                return;
                                                            }
                                                            if (result.error.has_value()) {
                                                                callback(OutboundOperationResult {
                                                                    false,
                                                                    std::nullopt,
                                                                    result.error.value() });
                                                                return;
                                                            }

                                                            if (!result.order.has_value()) {
                                                                callback(OutboundOperationResult {
                                                                    false,
                                                                    std::nullopt,
                                                                    AppError {
                                                                        AppErrorCategory::Validation,
                                                                        AppErrorCode::OutboundOrderNotFound,
                                                                        QStringLiteral("创建草稿订单失败,订单创建回查失败,订单数据不存在") } });
                                                                return;
                                                            }
                                                            if (result.order.value().status != OutboundOrderStatus::Draft) {
                                                                callback(OutboundOperationResult {
                                                                    false,
                                                                    std::nullopt,
                                                                    AppError::repositoryFailure(QStringLiteral("创建草稿订单失败,订单创建回查失败,订单状态非草稿")) });
                                                                return;
                                                            }

                                                            for (const auto& line : result.order.value().lines) {
                                                                if (line.orderId != result.order.value().id) {
                                                                    callback(OutboundOperationResult {
                                                                        false,
                                                                        std::nullopt,
                                                                        AppError::repositoryFailure(QStringLiteral("创建草稿订单失败,订单创建回查失败,订单行订单ID异常")) });
                                                                    return;
                                                                }
                                                            }

                                                            callback(OutboundOperationResult {
                                                                true,
                                                                result.order.value(),
                                                                std::nullopt });
                                                        });
                                                    } });
    // 提交任务
    executor_.submitTask(task);
}
// 确认出库订单
// 1. 先预读出库订单(含全部明细),用于构建逐行条件扣减语句
// 2. 将订单状态更新为确认状态(条件守卫: 必须是草稿)
// 3. 按行条件扣减库存: UPDATE ... SET quantity = quantity - :qty WHERE quantity >= :qty
//    (任一明细 affectedRows==0 → 整体回滚,库存不会变成负数)
// 4. 写入库存流水(quantity_delta 为负)
// 5. 写入审计日志
void MySqlOutboundRepository::confirmOrder(
    quint32 id,
    const AuditContext& auditContext,
    QObject* owner,
    OperateCallback callback)
{
    // 校验参数
    QPointer<QObject> ownerPtr(owner);
    if (ownerPtr.isNull() || !callback)
        return;
    if (id == 0) {
        callback(OutboundOperationResult {
            false,
            std::nullopt,
            AppError::repositoryFailure(QStringLiteral("确认出库失败,订单ID无效")) });
        return;
    }
    if (auditContext.operatorId == 0) {
        callback(OutboundOperationResult {
            false,
            std::nullopt,
            AppError::repositoryFailure(QStringLiteral("确认出库失败,操作人ID无效")) });
        return;
    }
    // 预读出库订单(含明细),构建逐行扣减需要明细的productId/quantity和订单的warehouseId
    findById(id, ownerPtr.data(), [this, id, auditContext, ownerPtr, callback = std::move(callback)](const OutboundOperationResult& preResult) {
        if (ownerPtr.isNull() || !callback)
            return;
        if (!preResult.success) {
            callback(OutboundOperationResult {
                false,
                std::nullopt,
                preResult.error.has_value() ? preResult.error.value() : AppError::repositoryFailure(QStringLiteral("确认出库失败,订单查找失败")) });
            return;
        }
        if (preResult.error.has_value()) {
            callback(OutboundOperationResult {
                false,
                std::nullopt,
                preResult.error.value() });
            return;
        }
        if (!preResult.order.has_value()) {
            callback(OutboundOperationResult {
                false,
                std::nullopt,
                AppError {
                    AppErrorCategory::Validation,
                    AppErrorCode::OutboundOrderNotFound,
                    QStringLiteral("确认出库失败,出库订单不存在") } });
            return;
        }
        const OutboundOrder order = preResult.order.value();
        const int lineCount = order.lines.size();
        if (lineCount == 0) {
            callback(OutboundOperationResult {
                false,
                std::nullopt,
                AppError::repositoryFailure(QStringLiteral("确认出库失败,订单没有明细行")) });
            return;
        }
        const quint32 warehouseId = order.warehouseId;

        // 1.生成流水号: MOV-YYYYMMDD-NNNNNN (日期+当日序号)
        const QString prefix = QStringLiteral("MOV-%1-").arg(
            QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd")));

        DatabaseStatement statement_seq; // 获取当前最大序号,movement_no序号每次递增
        statement_seq.type = StatementType::Command;
        statement_seq.sql = QStringLiteral(
            "SET @seq = (SELECT COALESCE(MAX(CAST(SUBSTRING_INDEX(movement_no, '-', -1) AS UNSIGNED)), 0) "
            "FROM stock_movements WHERE movement_no LIKE :prefixPattern)");
        statement_seq.parameters.insert("prefixPattern", prefix + QStringLiteral("%"));

        // 2.创建更新订单状态语句(条件守卫: 必须是草稿,防止重复确认/确认已取消订单)
        DatabaseStatement statement1;
        statement1.type = StatementType::Command;
        statement1.expectedAffectedRows = 1; // 设置期望,如果更新失败直接回滚
        statement1.sql = QStringLiteral(
            "UPDATE outbound_orders "
            "SET status = 'confirmed', "
            "    operator_id = :operatorId, "
            "    confirmed_at = NOW(3) "
            "WHERE id = :id "
            "  AND status = 'draft'");
        statement1.parameters.insert("id", id);
        statement1.parameters.insert("operatorId", auditContext.operatorId);

        // 3.按行条件扣减库存(逐行独立 UPDATE,每个语句带预期影响行数守卫)
        //    条件 quantity >= :qty 保证并发/多行场景下库存不会变成负数;
        //    任一行 affectedRows==0(无库存记录或库存不足)都会使事务回滚(不支持部分出库)
        DatabaseTask task;
        task.type = DatabaseTaskType::Transaction;
        task.requestId = QUuid::createUuid();
        task.statements.append(statement_seq);
        task.statements.append(statement1);
        for (const auto& line : order.lines) {
            DatabaseStatement deductStatement;
            deductStatement.type = StatementType::Command;
            deductStatement.expectedAffectedRows = 1; // 库存不足/无记录时整体回滚
            deductStatement.sql = QStringLiteral(
                "UPDATE stock_balance "
                "SET quantity = quantity - :qty, updated_at = NOW(3) "
                "WHERE product_id = :productId "
                "  AND warehouse_id = :warehouseId "
                "  AND quantity >= :qty");
            deductStatement.parameters.insert("qty", line.quantity);
            deductStatement.parameters.insert("productId", line.productId);
            deductStatement.parameters.insert("warehouseId", warehouseId);
            task.statements.append(deductStatement);
        }

        // 4.创建写入库存流水记录语句(quantity_delta 为负,表示出库)
        DatabaseStatement statement_movement;
        statement_movement.type = StatementType::Command;
        statement_movement.sql = QStringLiteral(
            "INSERT INTO stock_movements "
            "(movement_no, product_id, warehouse_id, movement_type, quantity_delta, "
            " source_type, source_id, source_line_id, movement_role, operator_id, reason, created_at) "
            "SELECT CONCAT(:prefix, LPAD(@seq+ROW_NUMBER() OVER(ORDER BY i.id), 6, '0')) AS movement_no, i.product_id, o.warehouse_id, 'outbound', -i.quantity, "
            "       'outbound', :id, i.id, 'normal', :operatorId, '出库确认', NOW(3) "
            "FROM outbound_details i "
            "JOIN outbound_orders o ON o.id = i.order_id "
            "WHERE o.id = :id");
        statement_movement.parameters.insert("operatorId", auditContext.operatorId);
        statement_movement.parameters.insert("id", id);
        statement_movement.parameters.insert("prefix", prefix);
        task.statements.append(statement_movement);

        // 5.创建审计日志写入语句
        DatabaseStatement statement4;
        statement4.type = StatementType::Command;
        statement4.expectedAffectedRows = 1; // 如果写入失败直接回滚
        statement4.sql = QStringLiteral(
            "INSERT INTO audit_logs "
            "(operator_id,username,action,target_type,target_id,detail,created_at) "
            "SELECT :operatorId,u.username,'confirm' ,'outbound',:targetId, "
            "JSON_OBJECT("
            "'module','outbound',"
            "'operatorId',:operatorId,"
            "'userName',u.username,"
            "'orderNo',o.order_no,"
            "'orderId',o.id,"
            "'action','confirm',"
            "'fromStatus','draft',"
            "'toStatus','confirmed',"
            "'warehouseId',o.warehouse_id,"
            "'recipient',o.recipient),"
            "CURRENT_TIMESTAMP(3) "
            "FROM users u "
            "JOIN outbound_orders o ON o.id = :targetId "
            "WHERE u.id = :operatorId");
        statement4.parameters.insert("operatorId", auditContext.operatorId);
        statement4.parameters.insert("targetId", id);

        // 提交任务,在回调中校验结果
        pending_.insert(task.requestId, PendingRequest { ownerPtr, [this, id, ownerPtr, lineCount, callback = std::move(callback)](const DatabaseResult& result) {
                                                            if (ownerPtr.isNull() || !callback)
                                                                return;

                                                            // 事务执行失败(含 expectedAffectedRows 未命中导致的回滚)
                                                            if (!result.isSucceeded()) {
                                                                callback(OutboundOperationResult {
                                                                    false,
                                                                    std::nullopt,
                                                                    mapDatabaseErrorToAppError(result.error, QStringLiteral("confirmOrder"), result.failedStatementIndex, lineCount) });
                                                                return;
                                                            }
                                                            const int expectedSize = 4 + lineCount;
                                                            if (result.statementResults.size() != expectedSize) {
                                                                callback(OutboundOperationResult {
                                                                    false,
                                                                    std::nullopt,
                                                                    AppError::repositoryFailure(QStringLiteral("确认出库失败,事务结果数量异常")) });
                                                                return;
                                                            }

                                                            // 校验 statement1: 更新订单状态(成功路径上由 expectedAffectedRows 守卫,必然命中)
                                                            const auto& result1 = result.statementResults[1];
                                                            if (result1.affectedRows != 1) {
                                                                callback(OutboundOperationResult {
                                                                    false,
                                                                    std::nullopt,
                                                                    AppError::repositoryFailure(QStringLiteral("确认出库失败,更新订单状态影响行数异常")) });
                                                                return;
                                                            }

                                                            // 校验逐行扣减(语句[2..2+N-1],成功路径上由 expectedAffectedRows 守卫)
                                                            for (int i = 0; i < lineCount; ++i) {
                                                                const auto& deductResult = result.statementResults[2 + i];
                                                                if (deductResult.affectedRows != 1) {
                                                                    callback(OutboundOperationResult {
                                                                        false,
                                                                        std::nullopt,
                                                                        AppError::repositoryFailure(QStringLiteral("确认出库失败,库存扣减影响行数异常")) });
                                                                    return;
                                                                }
                                                            }

                                                            // 校验库存流水(语句[2+N])
                                                            const auto& movementResult = result.statementResults[2 + lineCount];
                                                            if (movementResult.affectedRows < 1) {
                                                                callback(OutboundOperationResult {
                                                                    false,
                                                                    std::nullopt,
                                                                    AppError::repositoryFailure(QStringLiteral("确认出库失败,写入库存流水影响行数异常")) });
                                                                return;
                                                            }

                                                            // 校验审计日志(语句[3+N])
                                                            const auto& auditResult = result.statementResults[3 + lineCount];
                                                            if (auditResult.affectedRows != 1) {
                                                                callback(OutboundOperationResult {
                                                                    false,
                                                                    std::nullopt,
                                                                    AppError::repositoryFailure(QStringLiteral("确认出库失败,写入审计日志影响行数异常")) });
                                                                return;
                                                            }

                                                            // 回查出库订单,返回确认后的完整订单
                                                            findById(id, ownerPtr.data(), [ownerPtr, callback = std::move(callback)](const OutboundOperationResult& opResult) {
                                                                if (ownerPtr.isNull() || !callback)
                                                                    return;
                                                                if (!opResult.success) {
                                                                    callback(OutboundOperationResult {
                                                                        false,
                                                                        std::nullopt,
                                                                        opResult.error.has_value() ? opResult.error.value() : AppError::repositoryFailure(QStringLiteral("确认出库失败,订单查找失败")) });
                                                                    return;
                                                                }
                                                                if (opResult.error.has_value()) {
                                                                    callback(OutboundOperationResult {
                                                                        false,
                                                                        std::nullopt,
                                                                        opResult.error.value() });
                                                                    return;
                                                                }
                                                                if (!opResult.order.has_value()) {
                                                                    callback(OutboundOperationResult {
                                                                        false,
                                                                        std::nullopt,
                                                                        AppError {
                                                                            AppErrorCategory::Validation,
                                                                            AppErrorCode::OutboundOrderNotFound,
                                                                            QStringLiteral("确认出库失败,出库订单不存在") } });
                                                                    return;
                                                                }
                                                                const auto& order = opResult.order.value();
                                                                if (order.status != OutboundOrderStatus::Confirmed) {
                                                                    callback(OutboundOperationResult {
                                                                        false,
                                                                        std::nullopt,
                                                                        AppError::repositoryFailure(QStringLiteral("确认出库失败,订单状态异常")) });
                                                                    return;
                                                                }
                                                                callback(OutboundOperationResult {
                                                                    true,
                                                                    order,
                                                                    std::nullopt });
                                                            });
                                                        } });

        executor_.submitTask(task);
    });
}
// 将订单头映射为出库订单头详情Dto
std::optional<OutboundOrderDetailDto> MySqlOutboundRepository::mapOutboundOrderDetailHeader(
    const QStringList& columns,
    const QVariantList& row)
{
    OutboundOrderDetailDto header;
    if (columns.size() != row.size())
        return std::nullopt;
    for (int i = 0; i < columns.size(); ++i) {
        const QString& colName = columns[i];
        if (colName == "id") {
            header.id = row[i].toUInt();
        } else if (colName == "orderNo") {
            header.orderNo = row[i].toString();
        } else if (colName == "recipient") {
            header.recipient = row[i].toString();
        } else if (colName == "status") {
            header.status = mapDatabaseStatusToEnum(row[i].toString());
        } else if (colName == "operatorId") {
            header.operatorId = row[i].toUInt();
        } else if (colName == "operatorName") {
            header.operatorName = row[i].toString();
        } else if (colName == "warehouseId") {
            header.warehouseId = row[i].toUInt();
        } else if (colName == "warehouseName") {
            header.warehouseName = row[i].toString();
        } else if (colName == "remark") {
            header.remark = row[i].toString();
        } else if (colName == "createdAt") {
            header.createdAt = row[i].toDateTime();
        } else if (colName == "updatedAt") {
            header.updatedAt = row[i].toDateTime();
        } else if (colName == "confirmedAt") {
            if (!row[i].isNull())
                header.confirmedAt = row[i].toDateTime();
        }
    }
    if (header.id == 0 || header.orderNo.trimmed().isEmpty() || header.operatorId == 0 || header.warehouseId == 0)
        return std::nullopt;
    return header;
}
// 将订单行映射为出库订单行详情Dto
std::optional<OutboundOrderDetailLineDto> MySqlOutboundRepository::mapOutboundOrderDetailLine(
    const QStringList& columns,
    const QVariantList& row)
{
    if (columns.size() != row.size())
        return std::nullopt;
    OutboundOrderDetailLineDto line;
    for (int i = 0; i < columns.size(); ++i) {
        const QString& colName = columns[i];
        if (colName == "productId") {
            line.productId = row[i].toUInt();
        } else if (colName == "productCode") {
            line.productCode = row[i].toString();
        } else if (colName == "productName") {
            line.productName = row[i].toString();
        } else if (colName == "quantity") {
            line.quantity = row[i].toInt();
        } else if (colName == "unitPrice") {
            line.unitPrice = row[i].toDouble();
        } else if (colName == "subtotal") {
            line.subtotal = row[i].toDouble();
        }
    }
    if (line.productId == 0 || line.productCode.trimmed().isEmpty() || line.productName.trimmed().isEmpty() || line.quantity <= 0 || line.unitPrice < 0.0 || line.subtotal < 0.0)
        return std::nullopt;
    return std::make_optional<OutboundOrderDetailLineDto>(line);
}
// 获取订单详情
void MySqlOutboundRepository::getOrderDetail(
    quint32 id,
    QObject* owner,
    DetailCallback callback)
{
    QPointer<QObject> ownerPtr(owner);
    if (ownerPtr.isNull() || !callback)
        return;
    if (id == 0) {
        callback(OutboundOrderDetailResult {
            false,
            std::nullopt,
            AppError::repositoryFailure(QStringLiteral("获取订单详情失败,订单id无效")) });
        return;
    }
    // 1.查询表头信息
    DatabaseStatement statement1;
    statement1.type = StatementType::Query;
    statement1.sql = QStringLiteral(R"(SELECT 
o.id as id,
o.order_no as orderNo, 
o.recipient, 
o.status, 
o.operator_id as operatorId,
u.real_name as operatorName,
o.warehouse_id as warehouseId,
w.name as warehouseName,
o.remark,
o.created_at as createdAt,
o.updated_at as updatedAt, 
o.confirmed_at as confirmedAt
FROM outbound_orders o 
JOIN warehouses w 
ON o.warehouse_id = w.id
JOIN users u ON  o.operator_id = u.id WHERE o.id = :id)");
    statement1.parameters.insert("id", id);
    // 2.查询订单行信息
    DatabaseStatement statement2;
    statement2.type = StatementType::Query;
    statement2.sql = QStringLiteral(R"(SELECT
    p.id as productId ,
    p.code as productCode,
    p.name as productName,
    i.quantity as quantity,
    i.unit_price as unitPrice,
    i.unit_price * i.quantity as subtotal 
FROM outbound_details i JOIN products p ON p.id = i.product_id WHERE i.order_id = :id ORDER BY i.id ASC)");
    statement2.parameters.insert("id", id);
    // 构建任务
    DatabaseTask task;
    task.type = DatabaseTaskType::Transaction;
    task.requestId = QUuid::createUuid();
    task.statements.append(statement1);
    task.statements.append(statement2);
    pending_.insert(task.requestId, PendingRequest { ownerPtr, [ownerPtr, callback = std::move(callback)](const DatabaseResult& result) {
                                                        if (ownerPtr.isNull() || !callback)
                                                            return;
                                                        if (!result.isSucceeded()) {
                                                            callback(OutboundOrderDetailResult {
                                                                false,
                                                                std::nullopt,
                                                                mapDatabaseErrorToAppError(result.error) });
                                                            return;
                                                        }
                                                        if (result.statementResults.size() != 2) {
                                                            callback(OutboundOrderDetailResult {
                                                                false,
                                                                std::nullopt,
                                                                AppError::repositoryFailure(QStringLiteral("获取订单详情失败,查询结果异常")) });
                                                            return;
                                                        }
                                                        const auto& headerResult = result.statementResults[0];
                                                        if (headerResult.rows.isEmpty()) {
                                                            callback(OutboundOrderDetailResult {
                                                                false,
                                                                std::nullopt,
                                                                AppError {
                                                                    AppErrorCategory::Database,
                                                                    AppErrorCode::OutboundOrderNotFound,
                                                                    QStringLiteral("出库订单不存在") } });
                                                            return;
                                                        }
                                                        if (headerResult.columns.size() != detailHeaderColumns) {
                                                            callback(OutboundOrderDetailResult {
                                                                false,
                                                                std::nullopt,
                                                                AppError::repositoryFailure(QStringLiteral("获取订单详情失败,查询结果异常")) });
                                                            return;
                                                        }
                                                        auto detailHeader = mapOutboundOrderDetailHeader(headerResult.columns, headerResult.rows[0]);
                                                        if (!detailHeader.has_value()) {
                                                            callback(OutboundOrderDetailResult {
                                                                false,
                                                                std::nullopt,
                                                                AppError::repositoryFailure(QStringLiteral("获取订单详情失败,订单头映射异常")) });
                                                            return;
                                                        }
                                                        const auto& lineResult = result.statementResults[1];
                                                        if (lineResult.rows.isEmpty() || lineResult.columns.size() != detailLineColumns) {
                                                            callback(OutboundOrderDetailResult {
                                                                false,
                                                                std::nullopt,
                                                                AppError::repositoryFailure(QStringLiteral("获取订单详情失败,订单详情行查询异常")) });
                                                            return;
                                                        }
                                                        for (const auto& row : lineResult.rows) {
                                                            const auto detailLine = mapOutboundOrderDetailLine(lineResult.columns, row);
                                                            if (!detailLine.has_value()) {
                                                                callback(OutboundOrderDetailResult {
                                                                    false,
                                                                    std::nullopt,
                                                                    AppError::repositoryFailure(QStringLiteral("获取订单详情失败,订单详情行映射异常")) });
                                                                return;
                                                            }
                                                            detailHeader->detailLines.append(detailLine.value());
                                                            detailHeader->totalQuantity += detailLine->quantity;
                                                            detailHeader->totalAmount += detailLine->subtotal;
                                                        }
                                                        if (detailHeader->detailLines.isEmpty()) {
                                                            callback(OutboundOrderDetailResult {
                                                                false,
                                                                std::nullopt,
                                                                AppError {
                                                                    AppErrorCategory::Validation,
                                                                    AppErrorCode::InvalidOutboundOrder,
                                                                    QStringLiteral("出库订单明细为空，订单数据异常") } });
                                                            return;
                                                        }
                                                        detailHeader->lineCount = detailHeader->detailLines.size();
                                                        callback(OutboundOrderDetailResult {
                                                            true,
                                                            detailHeader.value(),
                                                            std::nullopt });
                                                    } });
    executor_.submitTask(task);
}