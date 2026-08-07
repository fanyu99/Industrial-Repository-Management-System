#include "MySqlInboundRepository.h"
#include "MySqlAuditLogRepository.h"
#include <QUuid>
#include <QVariantMap>
int MySqlInboundRepository::headerColumns = 10;
int MySqlInboundRepository::linesColumns = 5;
MySqlInboundRepository::MySqlInboundRepository(
    DatabaseExecutor& executor,
    QObject* parent)
    : executor_ { executor }
    , QObject(parent)
{
    // 连接任务执行器的taskFinished信号
    connect(&executor_, &DatabaseExecutor::taskFinished, this, &MySqlInboundRepository::onTaskFinished);
}
// 映射数据库错误到应用错误
AppError MySqlInboundRepository::mapDatabaseErrorToAppError(
    const DatabaseError& error,
    const QString& operationContext,
    int failedStatementIndex)
{
    // 有具体操作上下文时,进行精确错误映射
    if (!operationContext.isEmpty() && failedStatementIndex >= 0) {
        // confirmOrder: 确认订单
        if (operationContext == QStringLiteral("confirmOrder")) {
            // 语句4: INSERT INTO stock_movements (movement_no 唯一键冲突)
            if (failedStatementIndex == 3 && error.nativeErrorCode == QStringLiteral("1062")) {
                return AppError::databaseFailure(QStringLiteral("库存流水编号冲突，请重试"));
            }
            if (failedStatementIndex == 1 && error.code == DatabaseErrorCode::None) {
                return AppError{
                    AppErrorCategory::Validation,
                        AppErrorCode::InvalidInput,
                        QStringLiteral("确认订单失败,订单不存在或状态不是草稿")
                };
            }
        }
        // createDraft: 创建草稿订单
        if (operationContext == QStringLiteral("createDraft")) {
            // 语句2: INSERT INTO inbound_orders (order_no 唯一键冲突)
            if (failedStatementIndex == 2 && error.nativeErrorCode == QStringLiteral("1062")) {
                return AppError::databaseFailure(QStringLiteral("订单编号冲突，请重试"));
            }
        }
    }

    // 通用错误码映射
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
InboundOrderStatus MySqlInboundRepository::mapDatabaseStatusToEnum(const QString& status)
{
    if (status == "draft")
        return InboundOrderStatus::Draft;
    if (status == "confirmed")
        return InboundOrderStatus::Confirmed;
    if (status == "cancelled")
        return InboundOrderStatus::Cancelled;
    return InboundOrderStatus::Draft; // 默认草稿
}
// 映射枚举值到数据库订单状态
QString MySqlInboundRepository::mapEnumToDatabaseStatus(InboundOrderStatus status)
{
    if (status == InboundOrderStatus::Draft)
        return QString("draft");
    if (status == InboundOrderStatus::Confirmed)
        return QString("confirmed");
    if (status == InboundOrderStatus::Cancelled)
        return QString("cancelled");
    return QString("draft"); // 默认草稿
}
// 映射订单行(明细)到订单行结构体
std::optional<InboundOrderLine> MySqlInboundRepository::mapInboundOrderLine(
    const QStringList& columns,
    const QVariantList& row)
{
    if (columns.size() != MySqlInboundRepository::linesColumns)
        return std::nullopt;
    InboundOrderLine line;
    for (int i = 0; i < MySqlInboundRepository::linesColumns; ++i) {
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
void MySqlInboundRepository::onTaskFinished(const DatabaseResult& result)
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
// 根据ID查询入库订单
void MySqlInboundRepository::findById(
    quint32 id,
    QObject* owner,
    OperateCallback callback)
{
    // 校验参数
    QPointer<QObject> ownerPtr(owner);
    if (ownerPtr.isNull() || !callback)
        return;
    if (id == 0) {
        callback(InboundOperationResult {
            false,
            std::nullopt,
            AppError {
                AppErrorCategory::Validation,
                AppErrorCode::InvalidInboundOrder,
                QStringLiteral("订单ID无效") } }

        );
        return;
    }
    // 创建事务型语句(一句查订单Header,一句查Lines)
    DatabaseTask task;
    task.type = DatabaseTaskType::Transaction;
    task.requestId = QUuid::createUuid();
    // 创建查询语句

    // 1.查询Header
    DatabaseStatement statement1;
    statement1.type = StatementType::Query;
    statement1.sql = QStringLiteral(
        "SELECT id,order_no as orderNo,supplier,status,operator_id as operatorId,warehouse_id as warehouseId,remark,created_at as createdAt,updated_at as updatedAt,confirmed_at as confirmedAt FROM inbound_orders WHERE id = :id");
    statement1.parameters.insert("id", id);
    task.statements.append(statement1);

    // 2.查询Lines
    DatabaseStatement statement2;
    statement2.type = StatementType::Query;
    statement2.sql = QStringLiteral(
        "SELECT id as lineId,order_id as orderId,product_id as productId,quantity,unit_price as unitPrice FROM inbound_details WHERE order_id = :id");
    statement2.parameters.insert("id", id);
    task.statements.append(statement2);

    // 包装lambda回调函数处理查询结果
    // 提交任务到执行器
    pending_.insert(task.requestId, PendingRequest { ownerPtr, [ownerPtr, callback = std::move(callback), this, id](const DatabaseResult& result) {
                                                        // 校验参数
                                                        if (ownerPtr.isNull() || !callback)
                                                            return;

                                                        // 查询失败
                                                        if (!result.isSucceeded()) {
                                                            callback(InboundOperationResult {
                                                                false,
                                                                std::nullopt,
                                                                mapDatabaseErrorToAppError(result.error) });
                                                            return;
                                                        }
                                                        // 如果查询结果小于2,查询失败
                                                        if (result.statementResults.size() != 2) {
                                                            callback(InboundOperationResult {
                                                                false,
                                                                std::nullopt,
                                                                AppError::repositoryFailure(QStringLiteral("查询入库订单失败")) });
                                                            return;
                                                        }
                                                        // 查询成功
                                                        InboundOrder order; // 订单
                                                        const auto& headerResult = result.statementResults[0]; // Header结果
                                                        const auto& linesResult = result.statementResults[1]; // Lines结果

                                                        // 拼接成InboundOrder
                                                        // 处理Header结果
                                                        if (headerResult.columns.size() != MySqlInboundRepository::headerColumns) {
                                                            callback(InboundOperationResult {
                                                                false,
                                                                std::nullopt,
                                                                AppError::repositoryFailure(QStringLiteral("查询入库订单Header失败,列数错误,订单(%1号)").arg(id)) });
                                                            return;
                                                        }
                                                        if (headerResult.rows.size() != 0) { // 处理Header结果,如果Header结果为空,不做处理
                                                            for (int i = 0; i < MySqlInboundRepository::headerColumns; ++i) {
                                                                const QString& colName = headerResult.columns[i];
                                                                if (colName == "id")
                                                                    order.id = headerResult.rows[0].value(i).toUInt();
                                                                if (colName == "orderNo")
                                                                    order.orderNo = headerResult.rows[0].value(i).toString();
                                                                if (colName == "supplier")
                                                                    order.supplier = headerResult.rows[0].value(i).toString();
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
                                                        if (linesResult.columns.size() != MySqlInboundRepository::linesColumns) {
                                                            callback(InboundOperationResult {
                                                                false,
                                                                std::nullopt,
                                                                AppError::repositoryFailure(QStringLiteral("查询入库订单Lines失败,列数错误,订单(%1号)").arg(order.id)) });
                                                            return;
                                                        }
                                                        if (linesResult.rows.size() == 0 && headerResult.rows.size() != 0) { // 如果Lines结果为空,Header结果不为空,则查询失败
                                                            callback(InboundOperationResult {
                                                                false,
                                                                std::nullopt,
                                                                AppError::repositoryFailure(QStringLiteral("查询入库订单Lines失败,Lines结果为空,订单(%1号)数据空").arg(order.id)) });
                                                            return;
                                                        } else if (linesResult.rows.size() != 0 && headerResult.rows.size() == 0) { // 如果Lines结果不为空,Header结果为空,则查询失败
                                                            callback(InboundOperationResult {
                                                                false,
                                                                std::nullopt,
                                                                AppError::repositoryFailure(QStringLiteral("查询入库订单Lines失败,Lines结果不为空,Header结果为空,订单(%1号)数据空").arg(order.id)) });
                                                            return;
                                                        } else if (linesResult.rows.size() != 0 && headerResult.rows.size() != 0) { // 如果Lines结果不为空,Header结果不为空,则查询成功
                                                            // 遍历所有line
                                                            for (int i = 0; i < linesResult.rows.size(); ++i) {
                                                                std::optional<InboundOrderLine> line = mapInboundOrderLine(linesResult.columns, linesResult.rows[i]);
                                                                if (!line.has_value()) {
                                                                    callback(
                                                                        InboundOperationResult {
                                                                            false,
                                                                            std::nullopt,
                                                                            AppError::repositoryFailure(QStringLiteral("查询入库订单Lines失败,订单(%1号)第%2行数据转换错误").arg(order.id).arg(i + 1)) });
                                                                    return;
                                                                }
                                                                order.lines.append(line.value());
                                                            }
                                                        }
                                                        // 如果Lines结果为空,Header结果为空,查询任务成功,但查询结果空
                                                        else {
                                                            callback(
                                                                InboundOperationResult {
                                                                    true,
                                                                    std::nullopt,
                                                                    std::nullopt });
                                                            return;
                                                        }

                                                        // 最后统一回调
                                                        callback(InboundOperationResult {
                                                            true,
                                                            std::make_optional<InboundOrder>(order),
                                                            std::nullopt });
                                                    } });
    executor_.submitTask(task); // 最后提交任务
}
// 根据编号查询入库订单
void MySqlInboundRepository::findByOrderNo(
    const QString& orderNo,
    QObject* owner,
    OperateCallback callback)
{
    // 校验参数
    QPointer<QObject> ownerPtr(owner);
    if (ownerPtr.isNull() || !callback)
        return;
    if (orderNo.trimmed().isEmpty()) {
        callback(InboundOperationResult {
            false,
            std::nullopt,
            AppError {
                AppErrorCategory::Validation,
                AppErrorCode::InvalidInboundOrder,
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
        "SELECT id,order_no as orderNo,supplier,status,operator_id as operatorId,warehouse_id as warehouseId,remark,created_at as createdAt,updated_at as updatedAt,confirmed_at as confirmedAt FROM inbound_orders WHERE order_no = :orderNo");
    statement1.parameters.insert("orderNo", orderNo.trimmed());
    task.statements.append(statement1);

    // 2.查询Lines
    DatabaseStatement statement2;
    statement2.type = StatementType::Query;
    statement2.sql = QStringLiteral(
        "SELECT id as lineId,order_id as orderId,product_id as productId,quantity,unit_price as unitPrice FROM inbound_details WHERE order_id = (SELECT id FROM inbound_orders WHERE order_no = :orderNo)");
    statement2.parameters.insert("orderNo", orderNo.trimmed());
    task.statements.append(statement2);

    // 包装lambda回调函数处理查询结果
    pending_.insert(task.requestId, PendingRequest { ownerPtr, [ownerPtr, callback = std::move(callback), orderNo = orderNo.trimmed(), this](const DatabaseResult& result) {
                                                        // 校验参数
                                                        if (ownerPtr.isNull() || !callback)
                                                            return;
                                                        // 查询失败
                                                        if (!result.isSucceeded()) {
                                                            callback(InboundOperationResult {
                                                                false,
                                                                std::nullopt,
                                                                mapDatabaseErrorToAppError(result.error) });
                                                            return;
                                                        }
                                                        // 如果查询结果不等于2,查询失败
                                                        if (result.statementResults.size() != 2) {
                                                            callback(InboundOperationResult {
                                                                false,
                                                                std::nullopt,
                                                                AppError::repositoryFailure(QStringLiteral("查询入库订单失败")) });
                                                            return;
                                                        }
                                                        // 查询成功
                                                        InboundOrder order;
                                                        const auto& headerResult = result.statementResults[0];
                                                        const auto& linesResult = result.statementResults[1];

                                                        // 处理Header结果
                                                        if (headerResult.columns.size() != MySqlInboundRepository::headerColumns) {
                                                            callback(InboundOperationResult {
                                                                false,
                                                                std::nullopt,
                                                                AppError::repositoryFailure(QStringLiteral("查询入库订单Header失败,列数错误,订单(%1号)").arg(orderNo)) });
                                                            return;
                                                        }
                                                        if (headerResult.rows.size() != 0) {
                                                            for (int i = 0; i < MySqlInboundRepository::headerColumns; ++i) {
                                                                const QString& colName = headerResult.columns[i];
                                                                if (colName == "id")
                                                                    order.id = headerResult.rows[0].value(i).toUInt();
                                                                if (colName == "orderNo")
                                                                    order.orderNo = headerResult.rows[0].value(i).toString();
                                                                if (colName == "supplier")
                                                                    order.supplier = headerResult.rows[0].value(i).toString();
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
                                                        if (linesResult.columns.size() != MySqlInboundRepository::linesColumns) {
                                                            callback(InboundOperationResult {
                                                                false,
                                                                std::nullopt,
                                                                AppError::repositoryFailure(QStringLiteral("查询入库订单Lines失败,列数错误,订单(%1号)").arg(order.orderNo)) });
                                                            return;
                                                        }
                                                        if (linesResult.rows.size() == 0 && headerResult.rows.size() != 0) {
                                                            callback(InboundOperationResult {
                                                                false,
                                                                std::nullopt,
                                                                AppError::repositoryFailure(QStringLiteral("查询入库订单Lines失败,Lines结果为空,订单(%1号)数据空").arg(order.orderNo)) });
                                                            return;
                                                        } else if (linesResult.rows.size() != 0 && headerResult.rows.size() == 0) {
                                                            callback(InboundOperationResult {
                                                                false,
                                                                std::nullopt,
                                                                AppError::repositoryFailure(QStringLiteral("查询入库订单Lines失败,Lines结果不为空,Header结果为空,订单(%1号)数据空").arg(order.orderNo)) });
                                                            return;
                                                        } else if (linesResult.rows.size() != 0 && headerResult.rows.size() != 0) {
                                                            for (int i = 0; i < linesResult.rows.size(); ++i) {
                                                                const auto line = mapInboundOrderLine(linesResult.columns, linesResult.rows[i]);
                                                                if (!line.has_value()) {
                                                                    callback(
                                                                        InboundOperationResult {
                                                                            false,
                                                                            std::nullopt,
                                                                            AppError::repositoryFailure(QStringLiteral("查询入库订单Lines失败,订单(%1号)第%2行数据转换错误").arg(order.orderNo).arg(i + 1)) });
                                                                    return;
                                                                }
                                                                order.lines.append(line.value());
                                                            }
                                                        }
                                                        // Lines和Header都为空,查询成功但无数据
                                                        else {
                                                            callback(
                                                                InboundOperationResult {
                                                                    true,
                                                                    std::nullopt,
                                                                    std::nullopt });
                                                            return;
                                                        }

                                                        // 最后统一回调
                                                        callback(InboundOperationResult {
                                                            true,
                                                            std::make_optional<InboundOrder>(order),
                                                            std::nullopt });
                                                    } });
    executor_.submitTask(task);
}
// 分页查询入库订单
void MySqlInboundRepository::listOrders(
    const InboundOrderFilter& filter,
    const PageRequest& request,
    QObject* owner,
    PageCallback callback)
{

    QPointer<QObject> ownerPtr(owner);
    if (ownerPtr.isNull() || !callback) {
        return;
    }
    if (request.page <= 0 || request.pageSize <= 0) {
        callback(InboundPageResult {
            false,
            {},
            AppError::repositoryFailure(QStringLiteral("分页查询入库订单失败,分页参数错误")) });
        return;
    }
    // 创建事务型任务
    DatabaseTask task;
    task.type = DatabaseTaskType::Transaction;
    task.requestId = QUuid::createUuid();
    // 构建where查询条件
    QStringList whereConditions; // 条件
    QVariantMap parametersMap; // 参数映射
    if (!filter.keyword.trimmed().isEmpty()) { // 关键字
        whereConditions << QStringLiteral("(o.order_no LIKE :keyword OR o.supplier LIKE :keyword OR o.remark LIKE :keyword)");
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
    statement1.sql = QStringLiteral("SELECT COUNT(*) as total FROM inbound_orders o ") + whereResult;
    statement1.parameters = parametersMap;
    task.statements.append(statement1);
    // 2.查询分页数据
    DatabaseStatement statement2;
    statement2.type = StatementType::Query;
    statement2.sql = QStringLiteral(R"(
        SELECT 
            o.id AS id,
            o.order_no AS orderNo,
            o.supplier,
            o.status,
            o.warehouse_id AS warehouseId,
            w.name AS warehouseName,
            o.operator_id AS operatorId,
            u.real_name AS operatorName,
            COUNT(d.id) AS lineCount,
            COALESCE(SUM(d.quantity), 0) AS totalQuantity,
            o.created_at AS createdAt,
            o.updated_at AS updatedAt,
            o.confirmed_at AS confirmedAt
        FROM inbound_orders o
        JOIN warehouses w ON w.id = o.warehouse_id
        JOIN users u ON u.id = o.operator_id
        LEFT JOIN inbound_details d ON d.order_id = o.id
    )") + whereResult
        + QStringLiteral(R"(
        GROUP BY o.id, o.order_no, o.supplier, o.status, o.warehouse_id, w.name, o.operator_id, u.real_name, o.created_at, o.updated_at, o.confirmed_at
        ORDER BY o.created_at DESC
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
                                                            callback(InboundPageResult {
                                                                false,
                                                                {},
                                                                mapDatabaseErrorToAppError(result.error) });
                                                            return;
                                                        }
                                                        // 如果查询结果小于2,查询失败
                                                        if (result.statementResults.size() != 2) {
                                                            callback(InboundPageResult {
                                                                false,
                                                                {},
                                                                AppError::repositoryFailure(QStringLiteral("查询入库订单分页数据失败")) });
                                                            return;
                                                        }
                                                        // 1.总记录数
                                                        const auto& countResult = result.statementResults[0];
                                                        if (countResult.rows.isEmpty() || countResult.columns.size() != 1 || countResult.rows.constFirst().isEmpty()) { // 如果获取总记录数的结果空
                                                            callback(InboundPageResult {
                                                                false,
                                                                {},
                                                                AppError::repositoryFailure(QStringLiteral("查询入库订单分页数据为空/异常")) });
                                                            return;
                                                        }
                                                        int total = countResult.rows.constFirst().value(0).toInt(); // 总记录数
                                                        // 2. 获取分页数据
                                                        const auto& pageResult = result.statementResults[1]; // 分页数据
                                                        QVector<InboundOrderListItemDto> items; // 分页订单列表
                                                        for (int i = 0; i < pageResult.rows.size(); ++i) {
                                                            const auto& row = pageResult.rows[i];
                                                            InboundOrderListItemDto dto;
                                                            if (row.size() != pageResult.columns.size()) {
                                                                callback(InboundPageResult {
                                                                    false, {},
                                                                    AppError::repositoryFailure(QStringLiteral("查询入库订单分页数据列数不匹配")) });
                                                                return;
                                                            }
                                                            for (int j = 0; j < pageResult.columns.size(); ++j) {
                                                                const QString& colName = pageResult.columns[j];
                                                                if (colName == "id")
                                                                    dto.id = row.value(j).toInt();

                                                                if (colName == "orderNo")
                                                                    dto.orderNo = row.value(j).toString();
                                                                if (colName == "supplier")
                                                                    dto.supplier = row.value(j).toString();
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
                                                            if (dto.id == 0 || dto.orderNo.trimmed().isEmpty() || dto.supplier.trimmed().isEmpty() || dto.operatorId == 0 || dto.warehouseId == 0 || dto.lineCount < 0 || dto.totalQuantity <= 0 || dto.operatorName.trimmed().isEmpty() || dto.warehouseName.trimmed().isEmpty()) {
                                                                callback(InboundPageResult {
                                                                    false,
                                                                    {},
                                                                    AppError::repositoryFailure(QStringLiteral("订单分页数据映射异常")) });
                                                                return;
                                                            }
                                                            items.append(dto);
                                                        }
                                                        // 封装最后的分页结果
                                                        PageResult<InboundOrderListItemDto> page;
                                                        page.items = std::move(items);
                                                        page.total = total;
                                                        page.page = request.page;
                                                        page.pageSize = request.pageSize;
                                                        callback(InboundPageResult {
                                                            true,
                                                            page,
                                                            std::nullopt });
                                                    } });

    // 最后提交任务
    executor_.submitTask(task);
}
// 创建草稿订单(注意:需要生成有效的OrderNo和订单ID)
void MySqlInboundRepository::createDraft(
    const InboundOrder& order,
    const AuditContext& auditContext,
    QObject* owner,
    OperateCallback callback)
{
    // 检验参数
    QPointer<QObject> ownerPtr(owner);
    if (ownerPtr.isNull() || !callback)
        return;
    if (order.supplier.trimmed().isEmpty() || order.operatorId == 0 || order.warehouseId == 0 || order.lines.isEmpty()) {
        callback(
            InboundOperationResult {
                false,
                std::nullopt,
                AppError::repositoryFailure(QStringLiteral("创建草稿订单失败,订单参数错误")) });
        return;
    }
    // 生成订单号: INB-YYYYMMDD-NNNNNN 日期+序号
    const QString prefix = QStringLiteral("INB-%1-").arg(
        QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd")));
    // 日期中最大订单序号+1,作为新订单的序号
    DatabaseStatement statement_seq;
    statement_seq.type = StatementType::Command;
    statement_seq.sql = QStringLiteral(
        "SET @seq = (SELECT COALESCE(MAX(CAST(SUBSTRING_INDEX(order_no, '-', -1) AS UNSIGNED)), 0) + 1 "
        "FROM inbound_orders WHERE order_no LIKE :prefixPattern)");
    statement_seq.parameters.insert("prefixPattern", prefix + QStringLiteral("%"));
    // 生成并设置订单号
    DatabaseStatement statement_orderno;
    statement_orderno.type = StatementType::Command;
    statement_orderno.sql = QStringLiteral(
        "SET @gen_order_no = CONCAT(:prefix, LPAD(@seq, 6, '0'))");
    statement_orderno.parameters.insert("prefix", prefix);

    // 创建插入inbound_orders表语句
    DatabaseStatement statement1;
    statement1.type = StatementType::Command;
    statement1.sql = QStringLiteral("INSERT INTO inbound_orders(order_no,supplier,status,operator_id,warehouse_id,remark) VALUES(@gen_order_no,:supplier,:status,:operatorId,:warehouseId,:remark)");
    statement1.parameters.insert("supplier", order.supplier);
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
    // 插入inbound_details表语句
    for (const auto& line : order.lines) {
        DatabaseStatement lineStatement;
        lineStatement.type = StatementType::Command;
        lineStatement.sql = QStringLiteral("INSERT INTO inbound_details(order_id,product_id,quantity,unit_price) VALUES(@new_order_id,:productId,:quantity,:unitPrice)");
        lineStatement.parameters.insert("productId", line.productId);
        lineStatement.parameters.insert("quantity", line.quantity);
        lineStatement.parameters.insert("unitPrice", line.unitPrice);
        if (line.quantity <= 0 || line.unitPrice < 0 || line.productId == 0) {
            callback(
                InboundOperationResult {
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
        "SELECT :operatorId, u.username, 'create', 'inbound', CAST(@new_order_id AS CHAR), "
        "JSON_OBJECT('module', 'inbound', 'orderNo', @gen_order_no, 'supplier', :supplier, 'warehouseId', :warehouseId, 'action', 'create'), "
        "CURRENT_TIMESTAMP(3) "
        "FROM users u "
        "WHERE u.id = :operatorId");
    statementAudit.parameters.insert("operatorId", auditContext.operatorId);
    statementAudit.parameters.insert("supplier", order.supplier);
    statementAudit.parameters.insert("warehouseId", order.warehouseId);
    task.statements.append(statementAudit);
    // 包装lambda函数
    pending_.insert(task.requestId, PendingRequest { ownerPtr, [ownerPtr, callback = std::move(callback), order, this, task](const DatabaseResult& result) {
                                                        if (ownerPtr.isNull() || !callback)
                                                            return;
                                                        if (!result.isSucceeded()) {
                                                            callback(InboundOperationResult {
                                                                false,
                                                                std::nullopt, mapDatabaseErrorToAppError(result.error, QStringLiteral("createDraft"), result.failedStatementIndex) });
                                                            return;
                                                        }
                                                        // 校验结果
                                                        if (result.statementResults.size() != 6 + order.lines.size()) {
                                                            callback(InboundOperationResult {
                                                                false,
                                                                std::nullopt,
                                                                AppError::repositoryFailure(QStringLiteral("创建草稿订单失败,订单创建结果数量错误")) });
                                                            return;
                                                        }
                                                        if (result.statementResults[2].lastInsertId.toInt() == 0) {
                                                            callback(InboundOperationResult {
                                                                false,
                                                                std::nullopt,
                                                                AppError::repositoryFailure(QStringLiteral("创建草稿订单失败,订单ID异常")) });
                                                            return;
                                                        }
                                                        if (result.statementResults[2].affectedRows != 1) {
                                                            callback(InboundOperationResult {
                                                                false,
                                                                std::nullopt,
                                                                AppError::repositoryFailure(QStringLiteral("创建草稿订单失败,订单ID插入失败")) });
                                                            return;
                                                        }
                                                        for (int i = 5; i < result.statementResults.size() - 1; ++i) {
                                                            const auto& lineResult = result.statementResults[i];
                                                            if (lineResult.affectedRows != 1) {
                                                                callback(InboundOperationResult {
                                                                    false,
                                                                    std::nullopt,
                                                                    AppError::repositoryFailure(QStringLiteral("创建草稿订单失败,订单行插入失败")) });
                                                                return;
                                                            }
                                                        }
                                                        // 校验审计日志写入
                                                        const auto& auditResult = result.statementResults.last();
                                                        if (auditResult.affectedRows != 1) {
                                                            callback(InboundOperationResult {
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
                                                            callback(InboundOperationResult {
                                                                false,
                                                                std::nullopt,
                                                                AppError::repositoryFailure(QStringLiteral("创建草稿订单失败,无法获取生成的订单号/订单号为空")) });
                                                            return;
                                                        }
                                                        // 回查订单
                                                        findByOrderNo(orderNoStr, ownerPtr.data(), [ownerPtr, callback = std::move(callback)](const InboundOperationResult& result) {
                                                            if (ownerPtr.isNull() || !callback)
                                                                return;
                                                            if (!result.success) {
                                                                callback(InboundOperationResult {
                                                                    false, std::nullopt,
                                                                    result.error.has_value() ? result.error.value() : AppError::repositoryFailure(QStringLiteral("创建草稿订单失败,订单创建回查失败")) });
                                                                return;
                                                            }
                                                            if (result.error.has_value()) {
                                                                callback(InboundOperationResult {
                                                                    false,
                                                                    std::nullopt,
                                                                    result.error.value() });
                                                                return;
                                                            }

                                                            if (!result.order.has_value()) {
                                                                callback(InboundOperationResult {
                                                                    false,
                                                                    std::nullopt,
                                                                    AppError {
                                                                        AppErrorCategory::Validation,
                                                                        AppErrorCode::InboundOrderNotFound,
                                                                        QStringLiteral("创建草稿订单失败,订单创建回查失败,订单数据不存在") } });
                                                                return;
                                                            }
                                                            if (result.order.value().status != InboundOrderStatus::Draft) {
                                                                callback(InboundOperationResult {
                                                                    false,
                                                                    std::nullopt,
                                                                    AppError::repositoryFailure(QStringLiteral("创建草稿订单失败,订单创建回查失败,订单状态非草稿")) });
                                                                return;
                                                            }

                                                            for (const auto& line : result.order.value().lines) {
                                                                if (line.orderId != result.order.value().id) {
                                                                    callback(InboundOperationResult {
                                                                        false,
                                                                        std::nullopt,
                                                                        AppError::repositoryFailure(QStringLiteral("创建草稿订单失败,订单创建回查失败,订单行订单ID异常")) });
                                                                    return;
                                                                }
                                                            }

                                                            callback(InboundOperationResult {
                                                                true,
                                                                result.order.value(),
                                                                std::nullopt });
                                                        });
                                                    } });
    // 提交任务
    executor_.submitTask(task);
}
// 确认订单
// 1. 将订单状态更新为确认状态
// 2. 更新库存余额
// 3. 更新库存移动记录
// 4. 更新审计日志
void MySqlInboundRepository::confirmOrder(
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
        callback(InboundOperationResult {
            false,
            std::nullopt,
            AppError::repositoryFailure(QStringLiteral("确认订单失败,订单ID无效")) });
        return;
    }
    if (auditContext.operatorId == 0) {
        callback(InboundOperationResult {
            false,
            std::nullopt,
            AppError::repositoryFailure(QStringLiteral("确认订单失败,操作人ID无效")) });
        return;
    }

    // 1.创建更新订单状态语句
    DatabaseStatement statement1;
    statement1.type = StatementType::Command;
    statement1.expectedAffectedRows = 1; // 设置期望,如果更新失败直接回滚
    statement1.sql = QStringLiteral(
        "UPDATE inbound_orders "
        "SET status = 'confirmed', "
        "    operator_id = :operatorId, "
        "    confirmed_at = NOW(3) "
        "WHERE id = :id "
        "  AND status = 'draft'");
    statement1.parameters.insert("id", id);
    statement1.parameters.insert("operatorId", auditContext.operatorId);

    // 2.创建更新库存余额语句
    DatabaseStatement statement2;
    statement2.type = StatementType::Command;
    statement2.sql = QStringLiteral(
        "INSERT INTO stock_balance (product_id, warehouse_id, quantity) "
        "SELECT i.product_id, o.warehouse_id, SUM(i.quantity) "
        "FROM inbound_details i "
        "JOIN inbound_orders o ON o.id = i.order_id "
        "WHERE o.id = :id "
        "GROUP BY i.product_id, o.warehouse_id "
        "ON DUPLICATE KEY UPDATE quantity = quantity + VALUES(quantity), updated_at = NOW(3)");
    statement2.parameters.insert("id", id);

    // 3.生成流水号: MOV-YYYYMMDD-NNNNNN (日期+当日序号)
    const QString prefix = QStringLiteral("MOV-%1-").arg(
        QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd")));

    DatabaseStatement statement_seq; // 获取当前最大序号,movement_no序号每次递增
    statement_seq.type = StatementType::Command;
    statement_seq.sql = QStringLiteral(
        "SET @seq = (SELECT COALESCE(MAX(CAST(SUBSTRING_INDEX(movement_no, '-', -1) AS UNSIGNED)), 0) "
        "FROM stock_movements WHERE movement_no LIKE :prefixPattern)");
    statement_seq.parameters.insert("prefixPattern", prefix + QStringLiteral("%"));

    // 4.创建更新库存流水记录语句
    DatabaseStatement statement3;
    statement3.type = StatementType::Command;
    statement3.sql = QStringLiteral(
        "INSERT INTO stock_movements "
        "(movement_no, product_id, warehouse_id, movement_type, quantity_delta, "
        " source_type, source_id, source_line_id, movement_role, operator_id, reason, created_at) "
        "SELECT CONCAT(:prefix, LPAD(@seq+ROW_NUMBER() OVER(ORDER BY i.id), 6, '0')) AS movement_no, i.product_id, o.warehouse_id, 'inbound', i.quantity, "
        "       'inbound', :id, i.id, 'normal', :operatorId, '入库确认', NOW(3) "
        "FROM inbound_details i "
        "JOIN inbound_orders o ON o.id = i.order_id "
        "WHERE o.id = :id");
    statement3.parameters.insert("operatorId", auditContext.operatorId);
    statement3.parameters.insert("id", id);
    statement3.parameters.insert("prefix", prefix);

    // 5.创建审计日志写入语句
    DatabaseStatement statement4;
    statement4.type = StatementType::Command;
    statement4.expectedAffectedRows = 1; // 如果写入失败直接回滚
    statement4.sql = QStringLiteral(
        "INSERT INTO audit_logs "
        "(operator_id,username,action,target_type,target_id,detail,created_at) "
        "SELECT :operatorId,u.username,'confirm' ,'inbound',:targetId, "
        "JSON_OBJECT("
        "'module','inbound',"
        "'operatorId',:operatorId,"
        "'userName',u.username,"
        "'orderNo',o.order_no,"
        "'orderId',o.id,"
        "'action','confirm',"
        "'fromStatus','draft',"
        "'toStatus','confirmed',"
        "'warehouseId',o.warehouse_id),"
        "CURRENT_TIMESTAMP(3) "
        "FROM users u "
        "JOIN inbound_orders o ON o.id =:targetId "
        "WHERE u.id = :operatorId");
    statement4.parameters.insert("operatorId", auditContext.operatorId);
    statement4.parameters.insert("targetId", id);

    // 合并为事务任务
    DatabaseTask task;
    task.type = DatabaseTaskType::Transaction;
    task.requestId = QUuid::createUuid();
    task.statements.append(statement_seq);
    task.statements.append(statement1);
    task.statements.append(statement2);
    task.statements.append(statement3);
    task.statements.append(statement4);

    // 提交任务,在回调中校验结果
    pending_.insert(task.requestId, PendingRequest { ownerPtr, [this, id, ownerPtr, callback = std::move(callback)](const DatabaseResult& result) {
                                                        if (ownerPtr.isNull() || !callback)
                                                            return;

                                                        // 事务执行失败
                                                        if (!result.isSucceeded()) {
                                                            callback(InboundOperationResult {
                                                                false,
                                                                std::nullopt,
                                                                mapDatabaseErrorToAppError(result.error, QStringLiteral("confirmOrder"), result.failedStatementIndex) });
                                                            return;
                                                        }
                                                        if (result.statementResults.size() != 5) {
                                                            callback(InboundOperationResult {
                                                                false,
                                                                std::nullopt,
                                                                AppError::repositoryFailure(QStringLiteral("确认订单失败,事务结果数量异常")) });
                                                            return;
                                                        }

                                                        // 校验 statement1: 更新订单状态
                                                        const auto& result1 = result.statementResults[1];
                                                        const qint64 affectedRows = result1.affectedRows;

                                                        // 校验 statement2: 更新库存余额
                                                        const auto& result2 = result.statementResults[2];
                                                        if (result2.affectedRows < 1) {
                                                            callback(InboundOperationResult {
                                                                false,
                                                                std::nullopt,
                                                                AppError::repositoryFailure(QStringLiteral("确认订单失败,更新库存余额影响行数异常")) });
                                                            return;
                                                        }

                                                        // 校验 statement3: 写入库存流水
                                                        const auto& result3 = result.statementResults[3];
                                                        if (result3.affectedRows < 1) {
                                                            callback(InboundOperationResult {
                                                                false,
                                                                std::nullopt,
                                                                AppError::repositoryFailure(QStringLiteral("确认订单失败,写入库存流水影响行数异常")) });
                                                            return;
                                                        }

                                                        // 校验 statement4: 写入审计日志
                                                        const auto& result4 = result.statementResults[4];
                                                        if (result4.affectedRows != 1) {
                                                            callback(InboundOperationResult {
                                                                false,
                                                                std::nullopt,
                                                                AppError::repositoryFailure(QStringLiteral("确认订单失败,写入审计日志影响行数异常")) });
                                                            return;
                                                        }

                                                        // 调用 findById 查询最新订单状态进行最终校验
                                                        findById(id, ownerPtr.data(), [affectedRows, ownerPtr, callback](const InboundOperationResult& opResult) {
                                                            if (ownerPtr.isNull() || !callback)
                                                                return;

                                                            if (!opResult.success) {
                                                                callback(InboundOperationResult {
                                                                    false,
                                                                    std::nullopt,
                                                                    opResult.error.has_value() ? opResult.error.value() : AppError::repositoryFailure(QStringLiteral("确认订单失败,订单查找失败")) });
                                                                return;
                                                            }
                                                            if (opResult.error.has_value()) {
                                                                callback(InboundOperationResult {
                                                                    false,
                                                                    std::nullopt,
                                                                    opResult.error.value() });
                                                                return;
                                                            }
                                                            if (!opResult.order.has_value()) {
                                                                callback(InboundOperationResult {
                                                                    false,
                                                                    std::nullopt,
                                                                    AppError {
                                                                        AppErrorCategory::Validation,
                                                                        AppErrorCode::InboundOrderNotFound,
                                                                        QStringLiteral("确认订单失败,入库订单不存在") } });
                                                                return;
                                                            }

                                                            const auto& order = opResult.order.value();

                                                            // 订单已取消
                                                            if (order.status == InboundOrderStatus::Cancelled) {
                                                                callback(InboundOperationResult {
                                                                    false,
                                                                    std::nullopt,
                                                                    AppError::repositoryFailure(QStringLiteral("确认订单失败,订单已取消")) });
                                                                return;
                                                            }

                                                            // 订单已确认
                                                            if (order.status == InboundOrderStatus::Confirmed) {
                                                                if (affectedRows == 1) {
                                                                    callback(InboundOperationResult {
                                                                        true,
                                                                        order,
                                                                        std::nullopt });
                                                                } else {
                                                                    callback(InboundOperationResult {
                                                                        false,
                                                                        std::nullopt,
                                                                        AppError::repositoryFailure(QStringLiteral("确认订单失败,订单已确认")) });
                                                                }
                                                                return;
                                                            }

                                                            // 订单仍为草稿: 确认失败
                                                            callback(InboundOperationResult {
                                                                false,
                                                                std::nullopt,
                                                                AppError::repositoryFailure(QStringLiteral("确认订单失败,订单状态异常")) });
                                                        });
                                                    } });

    executor_.submitTask(task);
}