#pragma once
#include "AppError.h"
#include "AuthCredentialRecord.h"
#include "IAuthRepository.h"
#include <QPointer>
#include <QVector>
#include <optional>
#include <utility>
class FakeAuthRepository : public IAuthRepository {
public:
    QVector<AuthCredentialRecord> records;
    std::optional<AppError> nextFindByNameError;
    std::optional<AppError> nextFindByUserIdError;
    QString lastRequestedUserName;
    quint32 lastRequestedUserId { 0 };
    bool deferFindByName { false };
    bool deferFindByUserId { false };
    void addRecord(const AuthCredentialRecord& record)
    {
        records.push_back(record);
    }
    [[nodiscard]] bool hasPendingFindByName() const noexcept
    {
        return static_cast<bool>(pendingFindByNameCallback_);
    }
    [[nodiscard]] bool hasPendingFindByUserId() const noexcept
    {
        return static_cast<bool>(pendingFindByUserIdCallback_);
    }
    void completePendingFindByNameSuccess()
    {
        if (!pendingFindByNameCallback_) {
            return;
        }
        if (pendingFindByNameOwner_.isNull()) {
            resetPendingFindByName();
            return;
        }
        if (nextFindByNameError.has_value()) {
            const auto error = nextFindByNameError;
            nextFindByNameError.reset();
            auto callback = std::move(pendingFindByNameCallback_);
            resetPendingFindByName();
            callback(FindCredentialResult { false, std::nullopt, error });
            return;
        }
        const QString userName = pendingFindByNameUserName_;
        auto callback = std::move(pendingFindByNameCallback_);
        resetPendingFindByName();
        callback(performFindByName(userName));
    }
    void completePendingFindByNameError(const AppError& error)
    {
        completePendingFindByName(FindCredentialResult { false, std::nullopt, error });
    }
    void completePendingFindByUserIdSuccess()
    {
        if (!pendingFindByUserIdCallback_) {
            return;
        }
        if (pendingFindByUserIdOwner_.isNull()) {
            resetPendingFindByUserId();
            return;
        }
        if (nextFindByUserIdError.has_value()) {
            const auto error = nextFindByUserIdError;
            nextFindByUserIdError.reset();
            auto callback = std::move(pendingFindByUserIdCallback_);
            resetPendingFindByUserId();
            callback(FindCredentialResult { false, std::nullopt, error });
            return;
        }
        const quint32 userId = pendingFindByUserIdUserId_;
        auto callback = std::move(pendingFindByUserIdCallback_);
        resetPendingFindByUserId();
        callback(performFindByUserId(userId));
    }
    void completePendingFindByUserIdError(const AppError& error)
    {
        completePendingFindByUserId(FindCredentialResult { false, std::nullopt, error });
    }
    void clear()
    {
        records.clear();
        lastRequestedUserName.clear();
        lastRequestedUserId = 0;
        nextFindByNameError.reset();
        nextFindByUserIdError.reset();
        deferFindByName = false;
        deferFindByUserId = false;
        resetPendingFindByName();
        resetPendingFindByUserId();
    }
    void findCredentialByUserName(const QString& userName,
        QObject* owner,
        FindCredentialCallback callback) override
    {
        QPointer<QObject> ownerPtr(owner);
        if (ownerPtr.isNull() || !callback) {
            return;
        }
        lastRequestedUserName = userName;
        if (deferFindByName) {
            pendingFindByNameCallback_ = std::move(callback);
            pendingFindByNameUserName_ = userName;
            pendingFindByNameOwner_ = ownerPtr;
            return;
        }
        if (nextFindByNameError.has_value()) {
            const auto error = nextFindByNameError;
            nextFindByNameError.reset();
            callback(FindCredentialResult { false, std::nullopt, error });
            return;
        }
        callback(performFindByName(userName));
    }
    void findCredentialByUserId(quint32 userId,
        QObject* owner,
        FindCredentialCallback callback) override
    {
        QPointer<QObject> ownerPtr(owner);
        if (ownerPtr.isNull() || !callback) {
            return;
        }
        lastRequestedUserId = userId;
        if (deferFindByUserId) {
            pendingFindByUserIdCallback_ = std::move(callback);
            pendingFindByUserIdUserId_ = userId;
            pendingFindByUserIdOwner_ = ownerPtr;
            return;
        }
        if (nextFindByUserIdError.has_value()) {
            const auto error = nextFindByUserIdError;
            nextFindByUserIdError.reset();
            callback(FindCredentialResult { false, std::nullopt, error });
            return;
        }
        callback(performFindByUserId(userId));
    }

private:
    FindCredentialCallback pendingFindByNameCallback_;
    QString pendingFindByNameUserName_;
    QPointer<QObject> pendingFindByNameOwner_;
    FindCredentialCallback pendingFindByUserIdCallback_;
    quint32 pendingFindByUserIdUserId_ { 0 };
    QPointer<QObject> pendingFindByUserIdOwner_;
    void resetPendingFindByName() noexcept
    {
        pendingFindByNameCallback_ = nullptr;
        pendingFindByNameUserName_.clear();
        pendingFindByNameOwner_.clear();
    }
    void resetPendingFindByUserId() noexcept
    {
        pendingFindByUserIdCallback_ = nullptr;
        pendingFindByUserIdUserId_ = 0;
        pendingFindByUserIdOwner_.clear();
    }
    void completePendingFindByName(const FindCredentialResult& result)
    {
        if (!pendingFindByNameCallback_) {
            return;
        }
        if (pendingFindByNameOwner_.isNull()) {
            resetPendingFindByName();
            return;
        }
        auto callback = std::move(pendingFindByNameCallback_);
        resetPendingFindByName();
        callback(result);
    }
    void completePendingFindByUserId(const FindCredentialResult& result)
    {
        if (!pendingFindByUserIdCallback_) {
            return;
        }
        if (pendingFindByUserIdOwner_.isNull()) {
            resetPendingFindByUserId();
            return;
        }
        auto callback = std::move(pendingFindByUserIdCallback_);
        resetPendingFindByUserId();
        callback(result);
    }
    FindCredentialResult performFindByName(const QString& userName) const
    {
        for (const auto& record : records) {
            if (record.userName == userName) {
                return FindCredentialResult { true, record, std::nullopt };
            }
        }
        return FindCredentialResult { true, std::nullopt, std::nullopt };
    }
    FindCredentialResult performFindByUserId(quint32 userId) const
    {
        for (const auto& record : records) {
            if (record.userId == userId) {
                return FindCredentialResult { true, record, std::nullopt };
            }
        }
        return FindCredentialResult { true, std::nullopt, std::nullopt };
    }
};