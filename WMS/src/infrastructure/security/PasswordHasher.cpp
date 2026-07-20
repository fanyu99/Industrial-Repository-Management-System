#include "PasswordHasher.h"

/* TODO: 完成PasswordHasher类中创建一个工厂用来创建不同算法下的生成器
PasswordHash::hashPassword()用作入口
工厂用于创建和选择具体算法类
*/

// 校验策略是否有效
bool PasswordHashPolicy::isValid() const
{
    // 校验策略是否有效
    // -1表示无效,0表示默认值,但是无效
    return this->algorithmVersion > 0 && this->iterations > 0 && !this->hashName.isEmpty() && this->saltLength > 0 && this->hashLength > 0;
}
// 校验记录是否有效
bool PasswordHashRecord::isValid() const
{
    // 校验记录是否有效
    // -1表示无效,0表示默认值,但是无效
    return this->algorithmVersion > 0 && this->iterations > 0 && !this->hashName.isEmpty() && !this->salt.isEmpty() && !this->hash.isEmpty();
}

// 获取密码hash记录(返回记录和错误信息)
QPair<PasswordHashRecord, PasswordHashError> PasswordHasher::hashPassword(const QString& password, const PasswordHashPolicy& policy) const
{
    PasswordHashRecord record;
    // 校验策略是否有效
    if (!policy.isValid()) {
        return { record, PasswordHashError::InvalidPolicy };
    }
    // 校验密码是否有效
    if (password.isEmpty())
        return { record, PasswordHashError::InvalidPassword };

    // 设置记录
    record.hashName = policy.hashName;
    record.algorithmVersion = policy.algorithmVersion;
    record.iterations = policy.iterations;
    record.salt = generateSalt(policy);
    // 获取hash值
    record.hash = QPasswordDigestor::deriveKeyPbkdf2(QCryptographicHash::Sha256, password.toUtf8(), record.salt, record.iterations, policy.hashLength);
    return { record, record.hash.isEmpty() ? PasswordHashError::HashGenerateError : PasswordHashError::None };
}

// 校验密码是否匹配
PasswordVerifyError PasswordHasher::verifyPassword(const QString& inputPassword, const PasswordHashRecord& record) const
{
    if (!record.isValid())
        return PasswordVerifyError::InvalidRecord;
    if (inputPassword.isEmpty())
        return PasswordVerifyError::WrongPassword;

    QByteArray inputPasswordHash = QPasswordDigestor::deriveKeyPbkdf2(QCryptographicHash::Sha256, inputPassword.toUtf8(), record.salt, record.iterations, record.hash.length());

    if (inputPasswordHash.isEmpty())
        return PasswordVerifyError::HashGenerateError;
    quint8 result = 0;
    const quint8* p1 = reinterpret_cast<const quint8*>(inputPasswordHash.constData());
    const quint8* p2 = reinterpret_cast<const quint8*>(record.hash.constData());
    for (int i = 0; i < inputPasswordHash.size(); ++i)
        result |= p1[i] ^ p2[i];
    return result == 0 ? PasswordVerifyError::None : PasswordVerifyError::WrongPassword;
}

// 判断旧记录是否需要升级重hash
PasswordRehashResult PasswordHasher::needRehash(const PasswordHashRecord& oldRecord, const PasswordHashPolicy& nowPolicy) const
{
    if (!oldRecord.isValid())
        return PasswordRehashResult::InvalidRecord;
    if (!nowPolicy.isValid())
        return PasswordRehashResult::InvalidPolicy;

    if (oldRecord.algorithmVersion != nowPolicy.algorithmVersion)
        return PasswordRehashResult::Yes;
    if (oldRecord.iterations != nowPolicy.iterations)
        return PasswordRehashResult::Yes;
    if (oldRecord.hashName != nowPolicy.hashName)
        return PasswordRehashResult::Yes;
    if (oldRecord.hash.length() != nowPolicy.hashLength)
        return PasswordRehashResult::Yes;
    if (oldRecord.salt.length() != nowPolicy.saltLength)
        return PasswordRehashResult::Yes;

    return PasswordRehashResult::No;
}

// 生成随机盐
QByteArray PasswordHasher::generateSalt(const PasswordHashPolicy& policy) const
{
    // TODO: 生成随机盐
    QByteArray salt(policy.saltLength, Qt::Uninitialized);
    auto generator = QRandomGenerator::securelySeeded();
    for (int i = 0; i < policy.saltLength; ++i) {
        salt[i] = static_cast<char>(generator.bounded(256));
    }
    return salt;
}
