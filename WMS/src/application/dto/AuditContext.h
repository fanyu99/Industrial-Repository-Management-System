#pragma once
#include <QString>

struct AuditContext {
    QString userName;
    quint32 operatorId { 0 };
};