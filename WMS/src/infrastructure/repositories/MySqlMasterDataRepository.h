#pragma once
#include "DatabaseExecutor.h"
#include "DatabaseTypes.h"
#include "IMasterDataRepository.h"
#include <QHash>
#include <QObject>
#include <QUuid>
class MySqlMasterDataRepository : public QObject, public IMasterDataRepository {
    Q_OBJECT

public:
    explicit MySqlMasterDataRepository(DatabaseExecutor& executor, QObject* parent = nullptr);
    ~MySqlMasterDataRepository() = default;
    // 列出所有单位(是否仅激活)
    void listUnits(bool activeOnly, QObject* owner, const UnitListCallback callback) override;
    // 列出所有分类(是否仅激活)
    void listCategories(bool activeOnly, QObject* owner, const CategoryListCallback callback) override;
    // 列出所有仓库(是否仅激活)
    void listWarehouses(bool activeOnly, QObject* owner, const WarehouseListCallback callback) override;
    // 映射数据库错误为应用错误
    static AppError mapDatabaseErrorToAppError(const DatabaseError& error);

private slots:
    void onTaskFinished(const DatabaseResult& result); // 任务完成调用回调

private:
    static int UnitColumnCount; // 单位列数
    static int CategoryColumnCount; // 分类列数
    static int WarehouseColumnCount; // 仓库列数
    DatabaseExecutor& executor_; // 任务执行器
    QHash<QUuid, PendingRequest> pending_; // 待处理请求
};