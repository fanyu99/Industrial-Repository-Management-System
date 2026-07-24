// 认证仓库接口 IAuthRepository 取出认证相关数据
//  告诉认证数据应当怎么被拿到
// 根据用户名查出认证凭证记录,或告诉调用方查询失败
#pragma once
#include "AppError.h"
#include "AuthCredentialRecord.h"
#include "AuthenticatedUser.h"
#include "LoginDto.h"
#include <QObject>
#include <QString>
#include <functional>
#include <optional>
struct FindCredentialResult {
    std::optional<AuthCredentialRecord> credential;
    std::optional<AppError> error;
};
// 认证仓库
class IAuthRepository {
public:
    // 通过用户名查询认证凭证记录
    using FindCreadentialCallback = std::function<void(const FindCredentialResult&)>;
    virtual ~IAuthRepository() = default;
    virtual void findCredentialByUserName(const QString& userName, QObject* owner,
        FindCreadentialCallback callback = nullptr)
        = 0; // 根据用户名查询认证凭证并反馈
};
