#include "MySqlMasterDataRepository.h"
#include <QPointer>
#include <QVector>
#include <utility>
// 单位列数
int MySqlMasterDataRepository::UnitColumnCount = 4;
// 分类列数
int MySqlMasterDataRepository::CategoryColumnCount = 4;

MySqlMasterDataRepository::MySqlMasterDataRepository(DatabaseExecutor& executor, QObject* parent)
    : executor_(executor)
    , QObject(parent)
{
    connect(&executor_, &DatabaseExecutor::taskFinished, this, &MySqlMasterDataRepository::onTaskFinished);
}
// 映射数据库错误为应用错误
AppError MySqlMasterDataRepository::mapDatabaseErrorToAppError(const DatabaseError& error)
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

// 任务完成
void MySqlMasterDataRepository::onTaskFinished(const DatabaseResult& result)
{
    auto ite = pending_.find(result.requestId);
    if (ite == pending_.end())
        return;
    auto pending = std::move(ite.value());
    pending_.erase(ite);
    if (pending.owner.isNull())
        return;
    if (pending.handler)
        pending.handler(result);
}

// 列出所有单位(是否仅激活)
void MySqlMasterDataRepository::listUnits(bool activeOnly, QObject* owner, const UnitListCallback callback)
{
    // 校验参数
    QPointer<QObject> ownerPtr(owner);
    if (ownerPtr.isNull() || !callback)
        return;
    // 创建查询语句
    DatabaseStatement statement;
    statement.type = StatementType::Query;
    statement.sql = QStringLiteral("SELECT id,code,name,is_active as isActive FROM units");
    // 如果仅激活,添加where条件
    if (activeOnly)
        statement.sql += QStringLiteral(" WHERE is_active = 1");
    // 包装任务
    DatabaseTask task;
    task.type = DatabaseTaskType::Single;
    task.statements.append(statement);
    task.requestId = QUuid::createUuid();
    // 提交到pending_
    pending_.insert(task.requestId, PendingRequest { ownerPtr, [ownerPtr, callback = std::move(callback), activeOnly](const DatabaseResult& result) {
                                                        // 校验参数
                                                        if (ownerPtr.isNull() || !callback)
                                                            return;
                                                        if (!result.isSucceeded()) {
                                                            callback(UnitListResult {
                                                                false,
                                                                std::nullopt,
                                                                mapDatabaseErrorToAppError(result.error) });
                                                            return;
                                                        }
                                                        if (result.statementResults.size() != 1) {
                                                            callback(UnitListResult {
                                                                false,
                                                                std::nullopt,
                                                                AppError {
                                                                    AppError::repositoryFailure(QStringLiteral("单位查询失败,查询结果异常")) } });
                                                            return;
                                                        }
                                                        QVector<UnitDto> units; // 所有单位
                                                        const auto& statementResult = result.statementResults[0];
                                                        for (const auto& row : statementResult.rows) {
                                                            UnitDto unit;
                                                            // 如果行数据的数量与列数不同
                                                            if (row.size() != statementResult.columns.size() || row.size() != UnitColumnCount) {
                                                                callback(UnitListResult {
                                                                    false,
                                                                    std::nullopt,
                                                                    AppError {
                                                                        AppError::repositoryFailure(QStringLiteral("单位查询失败,单位行数据数量异常")) } });
                                                                return;
                                                            }
                                                            for (int i = 0; i < row.size(); ++i) {
                                                                if (statementResult.columns[i] == "id")
                                                                    unit.id = row[i].toUInt();
                                                                else if (statementResult.columns[i] == "code")
                                                                    unit.code = row[i].toString();
                                                                else if (statementResult.columns[i] == "name")
                                                                    unit.name = row[i].toString();
                                                                else if (statementResult.columns[i] == "isActive")
                                                                    unit.isActive = row[i].toBool();
                                                            }
                                                            // 检查单位数据
                                                            if (unit.id==0 || unit.code.trimmed().isEmpty() || unit.name.trimmed().isEmpty()) {
                                                                callback(UnitListResult {
                                                                    false,
                                                                    std::nullopt,
                                                                    AppError {
                                                                        AppError::repositoryFailure(QStringLiteral("单位查询失败,单位数据异常")) } });
                                                                return;
                                                            }
                                                            // 如果仅激活,但单位未激活,跳过
                                                            if (activeOnly && !unit.isActive)
                                                                continue;
                                                            // 添加到单位列表
                                                            units.append(unit);
                                                        }
                                                        // 回调返回列表数据
                                                        callback(UnitListResult {
                                                            true,
                                                            std::make_optional<QVector<UnitDto>>(units),
                                                            std::nullopt });
                                                    } });
    // 提交任务
    executor_.submitTask(task);
}
// 列出所有分类
void MySqlMasterDataRepository::listCategories(bool activeOnly, QObject* owner, const CategoryListCallback callback)
{
    QPointer<QObject> ownerPtr(owner);
    if (ownerPtr.isNull() || !callback)
        return;
    // 创建查询语句
    DatabaseStatement statement;
    statement.type = StatementType::Query;
    statement.sql = QStringLiteral("SELECT id,code,name,is_active as isActive FROM categories");
    // 如果仅激活,添加where条件
    if (activeOnly)
        statement.sql += QStringLiteral(" WHERE is_active = 1");
    // 包装任务
    DatabaseTask task;
    task.type = DatabaseTaskType::Single;
    task.statements.append(statement);
    task.requestId = QUuid::createUuid();
    // 提交到pending_
    pending_.insert(task.requestId, PendingRequest { ownerPtr, [ownerPtr, callback = std::move(callback), activeOnly](const DatabaseResult& result) {
                                                        // 校验参数
                                                        if (ownerPtr.isNull() || !callback)
                                                            return;
                                                        if (!result.isSucceeded()) {
                                                            callback(CategoryListResult {
                                                                false,
                                                                std::nullopt,
                                                                mapDatabaseErrorToAppError(result.error) });
                                                            return;
                                                        }
                                                        if (result.statementResults.size() != 1) {
                                                            callback(CategoryListResult {
                                                                false,
                                                                std::nullopt,
                                                                AppError {
                                                                    AppError::repositoryFailure(QStringLiteral("分类查询失败,查询结果异常")) } });
                                                            return;
                                                        }
                                                        QVector<CategoryDto> categories; // 所有分类
                                                        const auto& statementResult = result.statementResults[0];
                                                        for (const auto& row : statementResult.rows) {
                                                            CategoryDto category;
                                                            // 如果行数据的数量与列数不同
                                                            if (row.size() != statementResult.columns.size() || row.size() != CategoryColumnCount) {
                                                                callback(CategoryListResult {
                                                                    false,
                                                                    std::nullopt,
                                                                    AppError {
                                                                        AppError::repositoryFailure(QStringLiteral("分类查询失败,分类行数据数量异常")) } });
                                                                return;
                                                            }
                                                            for (int i = 0; i < CategoryColumnCount; ++i) {
                                                                if (statementResult.columns[i] == "id")
                                                                    category.id = row[i].toUInt();
                                                                else if (statementResult.columns[i] == "code")
                                                                    category.code = row[i].toString();
                                                                else if (statementResult.columns[i] == "name")
                                                                    category.name = row[i].toString();
                                                                else if (statementResult.columns[i] == "isActive")
                                                                    category.isActive = row[i].toBool();
                                                            }
                                                            // 检查分类数据
                                                            if (category.id == 0 || category.code.trimmed().isEmpty() || category.name.trimmed().isEmpty()) {
                                                                callback(CategoryListResult {
                                                                    false,
                                                                    std::nullopt,
                                                                    AppError {
                                                                        AppError::repositoryFailure(QStringLiteral("分类查询失败,分类数据异常")) } });
                                                                return;
                                                            }
                                                            // 如果仅激活,但分类未激活,跳过
                                                            if (activeOnly && !category.isActive)
                                                                continue;
                                                            // 添加到分类列表
                                                            categories.append(category);
                                                        }
                                                        // 回调返回列表数据
                                                        callback(CategoryListResult {
                                                            true,
                                                            std::make_optional<QVector<CategoryDto>>(categories),
                                                            std::nullopt });
                                                    } });
    // 提交任务
    executor_.submitTask(task);
}