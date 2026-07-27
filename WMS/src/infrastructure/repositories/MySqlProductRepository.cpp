#include "MySqlProductRepository.h"
// TODO: createProduct等
MySqlProductRepository::MySqlProductRepository(
    DatabaseExecutor& executor,
    QObject* parent)
    : executor_(executor)
    , QObject(parent)
{
    // 连接任务执行器的taskFinished信号
    connect(&executor_, &DatabaseExecutor::taskFinished, this, &MySqlProductRepository::onTaskFinished);
}
// 通过ID查找产品
// 1. 校验参数
// 2. 创建查询语句和任务
// 3. 提交任务到执行器Executor(仅放入队列中)
// 4. 注册lambda回调,等待任务执行完成发送完成信号(taskFinished)后调用lambda
// 5. 顶层调用者收到ProductOperationResult结果后,根据结果进行callback
void MySqlProductRepository::findById(
    quint32 id,
    QObject* owner,
    OperateCallback callback)
{
    // 校验参数
    QPointer<QObject> ownerPtr(owner);
    if (ownerPtr.isNull() || !callback) {
        return;
    }
    if (id == 0) {
        callback(ProductOperationResult {
            false,
            std::nullopt,
            AppError { AppErrorCategory::Validation, AppErrorCode::InvalidProduct, "产品ID无效" } });
        return;
    }
    // 创建查询语句
    DatabaseStatement statement;
    statement.type = StatementType::Query;
    statement.sql = QStringLiteral(" SELECT id, code, name, category_id, unit_id, specification, safety_stock, is_active as active FROM products WHERE id = :id LIMIT 1 ");
    statement.parameters.insert("id", id); // 绑定ID
    // 创建数据库任务并提交到执行器Executor
    DatabaseTask task;
    task.requestId = QUuid::createUuid(); // 提交任务并获取任务id

    task.type = DatabaseTaskType::Single;
    task.statements.append(statement);
    // 将callback存入到pending上下文
    // 上下文添加处理结果回调(将任务执行完成后的结果进行映射处理为产品信息,并调用另一个回调函数进行返回这个结果),最顶层的调用者就收到了ProductOperationResult结果
    pending_.insert(task.requestId, PendingRequest { ownerPtr, [ownerPtr, callback = std::move(callback)](const DatabaseResult& result) {
                                                        // 校验结果
                                                        if (ownerPtr.isNull() || !callback) {
                                                            return;
                                                        }
                                                        if (!result.isSucceeded()) {
                                                            callback(ProductOperationResult {
                                                                false,
                                                                std::nullopt,
                                                                mapDatabaseErrorToAppError(result.error) });
                                                            return;
                                                        }
                                                        // 获取查询结果
                                                        if (result.statementResults.isEmpty()) {
                                                            callback(ProductOperationResult {
                                                                false,
                                                                std::nullopt,
                                                                AppError::repositoryFailure(QStringLiteral("数据库结果为空")) });
                                                            return;
                                                        }
                                                        const auto& statementResult = result.statementResults.constFirst();
                                                        // 如果没有找到结果
                                                        if (statementResult.rows.isEmpty()) {
                                                            callback(ProductOperationResult {
                                                                true,
                                                                std::nullopt,
                                                                std::nullopt });
                                                            return;
                                                        }
                                                        // 将结果映射到Product对象
                                                        const auto& row = statementResult.rows.constFirst();

                                                        auto product = mapProductRow(statementResult.columns, row);
                                                        // 如果映射失败
                                                        if (!product.has_value()) {
                                                            callback(ProductOperationResult {
                                                                false,
                                                                std::nullopt,
                                                                AppError::repositoryFailure(QStringLiteral("产品映射失败")) });
                                                            return;
                                                        }
                                                        // 映射成功,调用回调函数返回产品相关信息
                                                        callback(ProductOperationResult { true, product.value(), std::nullopt });
                                                    } });
    // 提交任务到执行器Executor
    executor_.submitTask(task);
}

// 列出产品
void MySqlProductRepository::listProducts(
    const ProductFilter& filter,
    const PageRequest& request,
    QObject* owner,
    PageCallback callback_)
{
    // 校验参数
    QPointer<QObject> ownerPtr(owner);
    if (ownerPtr.isNull() || !callback_) {
        return;
    }
    // 创建事务型语句(一句查总数,一句查分页数据)
    DatabaseTask task;
    task.type = DatabaseTaskType::Transaction;
    task.requestId = QUuid::createUuid();
    // 构建查询语句
    // 构建where条件
    QStringList whereConditions; // 查询条件where
    QVariantMap parametersMap; // 参数映射
    if (!filter.keyword.trimmed().isEmpty()) { // 关键字条件
        whereConditions << QStringLiteral("(p.code LIKE :keyword OR p.name LIKE :keyword)");
        parametersMap.insert("keyword", "%" + filter.keyword.trimmed() + "%");
    }
    if (filter.categoryId.has_value()) { // 分类条件
        whereConditions << QStringLiteral("p.category_id = :categoryId");
        parametersMap.insert("categoryId", filter.categoryId.value());
    }
    if (filter.active.has_value()) { // 状态条件
        whereConditions << QStringLiteral("p.is_active = :active");
        parametersMap.insert("active", filter.active.value());
    }
    // 拼接where条件
    QString whereResult;
    if (!whereConditions.isEmpty()) {
        whereResult = QStringLiteral("WHERE ") + whereConditions.join(" AND ");
    }
    // 1.查询总记录数
    DatabaseStatement statement1;
    statement1.type = StatementType::Query;
    statement1.sql = QStringLiteral("SELECT COUNT(*) as total FROM products p ") + whereResult;
    statement1.parameters = parametersMap;
    task.statements.append(statement1);
    // 2.查询分页数据
    DatabaseStatement statement2;
    statement2.type = StatementType::Query;
    statement2.sql = QStringLiteral("SELECT p.id AS id,p.code AS code ,p.name AS name ,c.name AS category_name,u.name AS unit_name,p.specification AS specification,p.safety_stock AS safety_stock, p.is_active AS active FROM products p JOIN categories c ON c.id = p.category_id JOIN units u ON u.id = p.unit_id ") + whereResult + QStringLiteral(" ORDER BY p.id ASC LIMIT :limit OFFSET :offset ");
    statement2.parameters = parametersMap;
    statement2.parameters.insert("limit", request.pageSize);
    statement2.parameters.insert("offset", (request.page - 1) * request.pageSize);
    task.statements.append(statement2);

    // 包装lambda回调函数,处理查询结果

    // 提交任务到执行器Executor
    pending_.insert(
        task.requestId,
        PendingRequest {
            ownerPtr,
            [ownerPtr, request, callback = std::move(callback_)](const DatabaseResult& result) { // 回调函数处理数据库查询结果
                // 校验参数
                if (ownerPtr.isNull() || !callback) {
                    return;
                }
                // 如果查询失败
                if (!result.isSucceeded()) {
                    callback(ProductPageResult {
                        false,
                        {}, mapDatabaseErrorToAppError(result.error) });
                    return;
                }
                // 如果查询结果小于2,说明查询失败,返回结果
                if (result.statementResults.size() != 2) {
                    callback(ProductPageResult {
                        false,
                        {}, AppError::repositoryFailure(QStringLiteral("查询产品相关信息失败")) });
                    return;
                }
                // 查询成功
                const auto& countResult = result.statementResults[0]; // 获取总记录数
                if (countResult.rows.isEmpty() || countResult.columns.size() != 1 || countResult.rows.constFirst().isEmpty()) { // 如果获取的总记录数的结果空
                    callback(ProductPageResult { false, {}, AppError::repositoryFailure(QStringLiteral("查询产品总数结果为空/异常")) });
                    return;
                }
                int total = countResult.rows.constFirst().value(0).toInt(); // 获取总记录数
                const auto& pageResult = result.statementResults[1]; // 获取分页数据
                QVector<ProductListItemDto> items; // 分页产品列表
                for (int i = 0; i < pageResult.rows.size(); ++i) {
                    const auto& row = pageResult.rows[i];
                    ProductListItemDto dto; // 产品传输DTO
                    // 遍历列,对应列的值赋值
                    if (row.size() != pageResult.columns.size()) {
                        callback(ProductPageResult { false, {}, AppError::repositoryFailure(QStringLiteral("产品分页数据列数不匹配")) });
                        return;
                    }
                    for (int j = 0; j < pageResult.columns.size(); ++j) {
                        const QString& colName = pageResult.columns[j];
                        if (colName == "id") {
                            dto.id = row[j].toUInt();
                        } else if (colName == "code") {
                            dto.code = row[j].toString();
                        } else if (colName == "name") {
                            dto.name = row[j].toString();
                        } else if (colName == "category_name") {
                            dto.categoryName = row[j].toString();
                        } else if (colName == "unit_name") {
                            dto.unitName = row[j].toString();
                        } else if (colName == "specification") {
                            dto.specification = row[j].toString();
                        } else if (colName == "safety_stock") {
                            dto.safetyStock = row[j].toInt();
                        } else if (colName == "active") {
                            dto.active = row[j].toBool();
                        }
                    }
                    if (dto.id == 0 || dto.code.trimmed().isEmpty() || dto.name.trimmed().isEmpty() || dto.categoryName.trimmed().isEmpty() || dto.unitName.trimmed().isEmpty() || dto.safetyStock < 0) {
                        callback(ProductPageResult { false, {}, AppError::repositoryFailure(QStringLiteral("产品分页数据映射异常")) });
                        return;
                    }
                    items.append(dto);
                }
                // 封装最后的分页数据并回调
                PageResult<ProductListItemDto> page;
                page.items = std::move(items);
                page.total = total;
                page.page = request.page;
                page.pageSize = request.pageSize;
                callback(ProductPageResult { true, page, std::nullopt });

            } });

    executor_.submitTask(task); // 最后提交任务
}

// 通过编码查找产品
void MySqlProductRepository::findByCode(
    const QString& code,
    QObject* owner,
    OperateCallback callback)
{
    // 校验参数
    QPointer<QObject> ownerPtr(owner);
    if (ownerPtr.isNull() || !callback) {
        return;
    }

    auto normalCode = code.trimmed();
    if (normalCode.isEmpty()) {
        callback(ProductOperationResult {
            false,
            std::nullopt,
            AppError {
                AppErrorCategory::Validation,
                AppErrorCode::InvalidProduct,
                QStringLiteral("产品编码无效") } });
        return;
    }

    // 包装任务
    DatabaseStatement statement;
    statement.type = StatementType::Query;
    statement.sql = QStringLiteral(" SELECT id, code, name, category_id, unit_id, specification, safety_stock, is_active as active FROM products WHERE code = :code LIMIT 1 ");
    statement.parameters.insert("code", normalCode);

    DatabaseTask task;
    task.requestId = QUuid::createUuid();
    task.type = DatabaseTaskType::Single;
    task.statements.append(statement);
    // 提交任务并获取任务id
    pending_.insert(
        task.requestId,
        PendingRequest {
            ownerPtr,
            [ownerPtr, callback = std::move(callback)](const DatabaseResult& result) {
                // 校验结果
                if (ownerPtr.isNull() || !callback) {
                    return;
                }
                if (!result.isSucceeded()) {
                    callback(ProductOperationResult {
                        false,
                        std::nullopt,
                        mapDatabaseErrorToAppError(result.error) });
                    return;
                }
                if (result.statementResults.isEmpty()) {
                    callback(ProductOperationResult {
                        false,
                        std::nullopt,
                        AppError::repositoryFailure(QStringLiteral("数据库结果为空")) });
                    return;
                }
                // 获取查询结果
                const auto& statementResult = result.statementResults.constFirst();
                // 如果没有找到结果
                if (statementResult.rows.isEmpty()) {
                    callback(ProductOperationResult {
                        true,
                        std::nullopt,
                        std::nullopt });
                    return;
                }
                // 将结果映射到Product对象
                const auto& row = statementResult.rows.constFirst();
                auto product = mapProductRow(statementResult.columns, row);
                // 如果映射失败
                if (!product.has_value()) {
                    callback(ProductOperationResult {
                        false,
                        std::nullopt,
                        AppError::repositoryFailure(QStringLiteral("产品映射失败")) });
                    return;
                }
                // 映射成功,调用回调函数返回产品相关信息
                callback(ProductOperationResult { true, product.value(), std::nullopt });
            } });
    // 提交任务到执行器Executor
    executor_.submitTask(task);
}
//  创建产品
void MySqlProductRepository::createProduct(
    const Product& product,
    QObject* owner,
    OperateCallback callback_)
{
    // 校验参数
    QPointer<QObject> ownerPtr(owner);
    if (ownerPtr.isNull() || !callback_) {
        return;
    }
    if (!product.isValid()) {

        callback_(
            ProductOperationResult { false,
                std::nullopt,
                AppError {
                    AppErrorCategory::Validation,
                    AppErrorCode::InvalidProduct,
                    QStringLiteral("产品信息无效") } });
        return;
    }
    // 创建语句
    DatabaseStatement statement;
    statement.type = StatementType::Command;
    statement.sql = QStringLiteral("INSERT INTO products(code, name, category_id, unit_id, specification, safety_stock, is_active) VALUES (:code, :name, :category_id, :unit_id, :specification, :safety_stock, :is_active)");
    statement.parameters.insert("code", product.code);
    statement.parameters.insert("name", product.name);
    statement.parameters.insert("category_id", product.categoryId);
    statement.parameters.insert("unit_id", product.unitId);
    statement.parameters.insert("specification", product.specification);
    statement.parameters.insert("safety_stock", product.safetyStock);
    statement.parameters.insert("is_active", product.active);
    // 包装任务
    DatabaseTask task;
    task.requestId = QUuid::createUuid();
    task.type = DatabaseTaskType::Single;
    task.statements.append(statement);
    // 添加异步任务到队列
    pending_.insert(
        task.requestId,
        PendingRequest {
            ownerPtr,
            [this, ownerPtr, callback = std::move(callback_)](const DatabaseResult& result) {
                // 校验参数
                if (ownerPtr.isNull() || !callback) {
                    return;
                }
                // 如果语句失败
                if (!result.isSucceeded()) {
                    callback(ProductOperationResult {
                        false,
                        std::nullopt,
                        mapDatabaseErrorToAppError(result.error) });
                    return;
                }
                // 如果语句成功,但没有结果,创建失败
                if (result.statementResults.isEmpty()) {
                    callback(ProductOperationResult {
                        false,
                        std::nullopt,
                        AppError::repositoryFailure(QStringLiteral("数据库插入结果为空,创建失败")) });
                    return;
                }

                const auto& insertResult = result.statementResults.constFirst();
                // 如果影响行数异常,创建失败
                if (insertResult.affectedRows != 1) {
                    callback(ProductOperationResult {
                        false,
                        std::nullopt,
                        AppError::repositoryFailure(QStringLiteral("数据库产品创建影响行数异常")) });
                    return;
                }
                // 如果插入ID异常,创建失败
                const auto lastInsertId = insertResult.lastInsertId.toUInt();
                if (lastInsertId == 0) {
                    callback(ProductOperationResult {
                        false,
                        std::nullopt,
                        AppError::repositoryFailure(QStringLiteral("数据库产品创建失败")) });
                    return;
                }
                // 通过查找ID,获取产品信息并回调至创建成功回调函数
                findById(
                    lastInsertId,
                    ownerPtr.data(),
                    [ownerPtr, callback](const ProductOperationResult& findResult) {
                        if (ownerPtr.isNull() || !callback) {
                            return;
                        }

                        if (findResult.error.has_value()) {
                            callback(findResult);
                            return;
                        }

                        if (!findResult.product.has_value()) {
                            callback(ProductOperationResult {
                                false,
                                std::nullopt,
                                AppError::repositoryFailure(QStringLiteral("数据库产品创建成功但读取失败")) });
                            return;
                        }
                        // 查找成功最后回调创建结果
                        callback(ProductOperationResult {
                            true,
                            findResult.product.value(),
                            std::nullopt });
                    });
            } });
    // 提交任务
    executor_.submitTask(task);
}

// 更新产品
void MySqlProductRepository::updateProduct(
    const Product& product,
    QObject* owner,
    OperateCallback callback_)
{
    // 检查参数
    QPointer<QObject> ownerPtr(owner);
    if (ownerPtr.isNull() || !callback_) {
        return;
    }
    if (!product.isValid() || product.id == 0) {
        callback_(
            ProductOperationResult { false,
                std::nullopt,
                AppError {
                    AppErrorCategory::Validation,
                    AppErrorCode::InvalidProduct,
                    QStringLiteral("产品信息无效") } });
        return;
    }
    // 创建语句
    DatabaseStatement statement;
    statement.type = StatementType::Command;
    statement.sql = QStringLiteral("UPDATE products SET name = :name,category_id = :category_id,code = :code, unit_id = :unit_id, specification = :specification, safety_stock = :safety_stock, is_active = :is_active WHERE id = :id");
    statement.parameters.insert("id", product.id);
    statement.parameters.insert("name", product.name);
    statement.parameters.insert("category_id", product.categoryId);
    statement.parameters.insert("code", product.code);
    statement.parameters.insert("unit_id", product.unitId);
    statement.parameters.insert("specification", product.specification);
    statement.parameters.insert("safety_stock", product.safetyStock);
    statement.parameters.insert("is_active", product.active);
    // 包装任务
    DatabaseTask task;
    task.requestId = QUuid::createUuid();
    task.type = DatabaseTaskType::Single;
    task.statements.append(statement);
    // 加入异步回调函数到pending_中
    pending_.insert(task.requestId, PendingRequest { ownerPtr, [this, ownerPtr, callback = std::move(callback_), product](const DatabaseResult& result) {
                                                        // 校验参数
                                                        if (ownerPtr.isNull() || !callback) {
                                                            return;
                                                        }
                                                        if (!result.isSucceeded()) {
                                                            callback(ProductOperationResult {
                                                                false,
                                                                std::nullopt,
                                                                mapDatabaseErrorToAppError(result.error) });
                                                            return;
                                                        }
                                                        if (result.statementResults.isEmpty()) {
                                                            callback(ProductOperationResult {
                                                                false,
                                                                std::nullopt,
                                                                AppError::repositoryFailure(QStringLiteral("数据库产品更新影响行数异常")) });
                                                            return;
                                                        }
                                                        const auto& updateResult = result.statementResults[0];
                                                        // 如果影响行数异常,更新失败
                                                        if (updateResult.affectedRows > 1) {
                                                            callback(
                                                                ProductOperationResult {
                                                                    false,
                                                                    std::nullopt,
                                                                    AppError::repositoryFailure(QStringLiteral("数据库产品更新影响行数异常")) });
                                                            return;
                                                        }
                                                        findById(product.id, ownerPtr.data(), [callback = std::move(callback), ownerPtr](const ProductOperationResult& findResult) {
                                                            if (ownerPtr.isNull() || !callback) {
                                                                return;
                                                            }
                                                            if (findResult.error.has_value()) {
                                                                callback(findResult);
                                                                return;
                                                            }
                                                            if (!findResult.product.has_value()) {
                                                                callback(ProductOperationResult {
                                                                    false,
                                                                    std::nullopt,
                                                                    AppError {
                                                                        AppErrorCategory::Validation,
                                                                        AppErrorCode::ProductNotFound,
                                                                        QStringLiteral("产品不存在") } });
                                                                return;
                                                            }
                                                            // 查找成功最后回调更新结果
                                                            callback(ProductOperationResult {
                                                                true,
                                                                findResult.product.value(),
                                                                std::nullopt });
                                                        });
                                                    }

                                    });
    // 提交任务
    executor_.submitTask(task);
}

// 设置产品状态
void MySqlProductRepository::setProductActive(
    quint32 id,
    bool active,
    QObject* owner,
    ActiveCallback callback)
{
    QPointer<QObject> ownerPtr(owner);
    if (ownerPtr.isNull() || !callback) {
        return;
    }
    if (id == 0) {
        callback(AppError{AppErrorCategory::Validation, AppErrorCode::InvalidProduct, QStringLiteral("产品id无效")});
        return;
    }
    // 创建语句
    DatabaseStatement statement;
    statement.type = StatementType::Command;
    statement.sql = QStringLiteral("UPDATE products SET is_active = :is_active WHERE id = :id");
    statement.parameters.insert("id", id);
    statement.parameters.insert("is_active", active);
    // 包装任务
    DatabaseTask task;
    task.requestId = QUuid::createUuid();
    task.type = DatabaseTaskType::Single;
    task.statements.append(statement);
    // 添加到pending_
    pending_.insert(
        task.requestId,
        PendingRequest {
            ownerPtr,
            [this, ownerPtr, id, callback = std::move(callback)](const DatabaseResult& result) {
                if (ownerPtr.isNull() || !callback) {
                    return;
                }

                if (!result.isSucceeded()) {
                    callback(mapDatabaseErrorToAppError(result.error));
                    return;
                }

                if (result.statementResults.isEmpty()) {
                    callback(AppError::repositoryFailure(QStringLiteral("数据库产品状态更新结果为空")));
                    return;
                }

                const auto& updateResult = result.statementResults.constFirst();

                if (updateResult.affectedRows > 1) {
                    callback(AppError::repositoryFailure(QStringLiteral("数据库产品状态更新影响行数异常")));
                    return;
                }

                findById(
                    id,
                    ownerPtr.data(),
                    [ownerPtr, callback](const ProductOperationResult& findResult) {
                        if (ownerPtr.isNull() || !callback) {
                            return;
                        }

                        if (findResult.error.has_value()) {
                            callback(findResult.error.value());
                            return;
                        }

                        if (!findResult.product.has_value()) {
                            callback(AppError {
                                AppErrorCategory::Validation,
                                AppErrorCode::ProductNotFound,
                                QStringLiteral("产品不存在") });
                            return;
                        }

                        callback(std::nullopt);
                    });
            } });

    // 提交任务
    executor_.submitTask(task);
}
// 执行器任务完成后处理结果
void MySqlProductRepository::onTaskFinished(const DatabaseResult& result)
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
// 映射数据库错误到应用错误
AppError MySqlProductRepository::mapDatabaseErrorToAppError(const DatabaseError& error)
{
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
    case DatabaseErrorCode::TransactionFailed:
        return AppError::databaseFailure(QStringLiteral("数据库操作失败"));
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
// 映射产品行到产品
std::optional<Product> MySqlProductRepository::mapProductRow(
    const QStringList& columns,
    const QVariantList& row)
{
    Product product;
    if (columns.size() != row.size()) {
        return std::nullopt;
    }
    for (int i = 0; i < columns.size(); ++i) {
        if (columns[i] == "id") {
            product.id = row[i].toUInt();
        } else if (columns[i] == "code") {
            product.code = row[i].toString();
        } else if (columns[i] == "name") {
            product.name = row[i].toString();
        } else if (columns[i] == "category_id") {
            product.categoryId = row[i].toUInt();
        } else if (columns[i] == "unit_id") {
            product.unitId = row[i].toUInt();
        } else if (columns[i] == "specification") {
            product.specification = row[i].toString();
        } else if (columns[i] == "active") {
            product.active = row[i].toBool();
        } else if (columns[i] == "safety_stock") {
            product.safetyStock = row[i].toInt();
        }
    }
    if (product.code.trimmed().isEmpty() || product.id == 0 || product.name.trimmed().isEmpty() || product.categoryId == 0 || product.unitId == 0 || product.safetyStock < 0) {
        return std::nullopt;
    }
    return product;
}