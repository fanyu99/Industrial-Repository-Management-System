#include "MySqlAuditLogRepository.h"

MySqlAuditLogRepository::MySqlAuditLogRepository(QObject* parent)
    : QObject(parent)
{
}

DatabaseStatement MySqlAuditLogRepository::buildInsertStatement(const AuditLogEntry& entry)
{
    DatabaseStatement statement;
    statement.type = StatementType::Command;
    statement.sql = QStringLiteral(
        "INSERT INTO audit_logs "
        "(operator_id, username, action, target_type, target_id, detail, created_at) "
        "SELECT :operatorId, u.username, :action, :targetType, :targetId, :detail, CURRENT_TIMESTAMP(3) "
        "FROM users u "
        "WHERE u.id = :operatorId");
    statement.parameters.insert("operatorId", entry.operatorId);
    statement.parameters.insert("action", entry.action);
    statement.parameters.insert("targetType", entry.targetType);
    statement.parameters.insert("targetId", entry.targetId);
    if (!entry.detail.isEmpty())
        statement.parameters.insert("detail", entry.toJsonStr());
    else
        statement.parameters.insert("detail", QString());
    return statement;
}