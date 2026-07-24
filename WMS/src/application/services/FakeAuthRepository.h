#pragma once
#include "AuthCredentialRecord.h"
#include "IAuthRepository.h"
#include <QPointer>
// 用于测试的仓库接口
class FakeAuthRepository : public IAuthRepository {
public:
    std::optional<AuthCredentialRecord> record;
    void findCredentialByUserName(const QString& userName, 
        QObject* owner,
        FindCreadentialCallback callback) override
    {
        QPointer<QObject> ownerPtr(owner);
        if(ownerPtr.isNull())return;
        if (!callback)
            return;
        FindCredentialResult result;
        if (record.has_value() && record->userName == userName) {
            result.credential = record.value();
        } 
        callback(result);
    }
};