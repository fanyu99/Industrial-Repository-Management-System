#pragma once
#include "AppError.h"
#include "AuthenticatedUser.h"
#include "DatabaseExecutor.h"
#include "DatabaseTypes.h"
#include "IAuthRepository.h"
#include <QHash>
#include <QObject>
class MySqlAuthRepository : public QObject, public IAuthRepository {
    Q_OBJECT
public:
    explicit MySqlAuthRepository(DatabaseExecutor& executor, QObject* parent = nullptr);
    ~MySqlAuthRepository() = default;
    // 通过用户名查询认证凭证记录
    void findCredentialByUserName(const QString& userName,
        QObject* owner,
        FindCredentialCallback callback) override;
    // 通过用户ID查询认证凭证记录
    void findCredentialByUserId(quint32 userId,
        QObject* owner,
        FindCredentialCallback callback) override;
    // 将数据库错误映射为应用错误
    AppError mapDatabaseErrorToAppError(DatabaseError error) const;
    // 将数据库结果映射为认证凭证记录
    std::optional<AuthCredentialRecord> mapStatementResultToFindCredentialResult(const QStringList& columns, const QVariantList& row) const;

private slots:
    // 处理数据库任务完成信号
    void onTaskFinished(const DatabaseResult& result);

private:
    DatabaseExecutor& executor_;
    QHash<QUuid, PendingRequest> pending_;
};