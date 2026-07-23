#include "PasswordHasher.h"

// 校验策略是否有效
bool PasswordHashPolicy::isValid() const
{
    // 校验策略是否有效
    // -1表示无效,0表示默认值,但是无效
    return this->algorithmVersion <= nowAlgorithmVersion && this->algorithmVersion >= 1 && this->iterations > 0 && this->saltLength >= minSaltLength && this->hashLength >= minHashLength
        && this->hashName == kPbkdf2Sha256Name; // 目前仅支持pbkdf2_hmac_sha256算法
}
// 校验记录是否有效
bool PasswordHashRecord::isValid() const
{
    // 校验记录是否有效
    // -1表示无效,0表示默认值,但是无效
    return this->algorithmVersion <= nowAlgorithmVersion && this->algorithmVersion >= 1 && this->iterations > 0 && this->hash.size() >= minHashLength && this->salt.size() >= minSaltLength
        && this->hashName == kPbkdf2Sha256Name; // 目前仅支持pbkdf2_hmac_sha256算法
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
    // 获取hash值(目前仅支持pbkdf2_hmac_sha256算法)
    record.hash = QPasswordDigestor::deriveKeyPbkdf2(QCryptographicHash::Sha256, password.toUtf8(), record.salt, record.iterations, policy.hashLength);
    return { record, record.hash.isEmpty() ? PasswordHashError::HashGenerateError : PasswordHashError::None };
}

// 校验密码是否匹配,无错误则密码匹配
PasswordVerifyError PasswordHasher::verifyPassword(const QString& inputPassword, const PasswordHashRecord& record) const
{
    if (!record.isValid())
        return PasswordVerifyError::InvalidRecord;
    if (inputPassword.isEmpty())
        return PasswordVerifyError::WrongPassword;

    QByteArray inputPasswordHash = QPasswordDigestor::deriveKeyPbkdf2(QCryptographicHash::Sha256, inputPassword.toUtf8(), record.salt, record.iterations, record.hash.length()); // 目前仅支持pbkdf2_hmac_sha256算法

    if (inputPasswordHash.isEmpty())
        return PasswordVerifyError::HashGenerateError;
    // 进行时序比较
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
    if (oldRecord.algorithmVersion != nowPolicy.algorithmVersion || oldRecord.iterations != nowPolicy.iterations || oldRecord.hashName != nowPolicy.hashName || oldRecord.hash.length() != nowPolicy.hashLength || oldRecord.salt.length() != nowPolicy.saltLength)
        return PasswordRehashResult::Yes;
    else
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
