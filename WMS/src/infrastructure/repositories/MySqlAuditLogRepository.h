#pragma once
#include "AuditContext.h"
#include <DatabaseTypes.h>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>
// 审计条目
struct AuditLogEntry {
    quint32 operatorId { 0 };
    QString userName;
    QString action;
    QString targetType;
    QString targetId;
    QJsonObject detail;

    QString toJsonStr() const
    {
        QJsonDocument doc(detail);
        return QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
    }

    void fromJson(const QString& jsonStr)
    {
        QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
        detail = doc.object();
    }
};

class MySqlAuditLogRepository : public QObject {
    Q_OBJECT
public:
    explicit MySqlAuditLogRepository(QObject* parent = nullptr);
    ~MySqlAuditLogRepository() = default;

    static DatabaseStatement buildInsertStatement(const AuditLogEntry& entry);
};