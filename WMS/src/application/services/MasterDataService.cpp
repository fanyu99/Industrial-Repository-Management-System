#include "MasterDataService.h"
#include <QPointer>
MasterDataService::MasterDataService(IMasterDataRepository& repository, SessionManager& sessionManager, QObject* parent)
    : QObject(parent)
    , repository_(repository)
    , sessionManager_(sessionManager)
{
}
// 获取当前用户
std::optional<AuthenticatedUser> MasterDataService::currentUser() const
{
    return sessionManager_.currentUser();
}
// 校验是否有权限
bool MasterDataService::hasPermission(Permission permission) const
{
    return sessionManager_.hasPermission(permission);
}
// 校验用户是否有权限(默认为查看产品)并返回应用错误
std::optional<AppError> MasterDataService::authorize(Permission permission) const
{
    if (!sessionManager_.isAuthenticated()) {
        return AppError {
            AppErrorCategory::Auth,
            AppErrorCode::NotAuthenticated,
            QStringLiteral("用户未登录")
        };
    }
    if (!hasPermission(permission)) {
        return AppError::permissionDenied();
    }
    return std::nullopt;
}
// 列出所有单位
void MasterDataService::listUnits(QObject* owner, bool activeOnly, const UnitListCallback callback) const
{
    // 校验参数
    QPointer<QObject> ownerPtr(owner);
    if (ownerPtr.isNull() || !callback)
        return;
    // 校验是否登录
    if (auto error = authorize(); error.has_value()) {
        callback(
            UnitListResult {
                false,
                std::nullopt,
                error.value() });
        return;
    }
    // 列出单位
    repository_.listUnits(activeOnly, ownerPtr.data(), [ownerPtr,callback = std::move(callback)](const UnitListResult& result) {
        if (ownerPtr.isNull() || !callback)
            return;
        if (!result.success) {
            callback(UnitListResult {
                false,
                std::nullopt,
                result.error.has_value() ? result.error.value() : AppError { AppErrorCategory::Validation, AppErrorCode::UnknownError, QStringLiteral("列出单位失败,未知错误") } });
            return;
        }
        if (result.error.has_value()) {
            callback(UnitListResult {
                false,
                std::nullopt,
                result.error.value() });
            return;
        }
        if (!result.units.has_value()) {
            callback(UnitListResult {
                false,
                std::nullopt,
                AppError {
                    AppErrorCategory::Validation,
                    AppErrorCode::UnknownError,
                    QStringLiteral("列出单位失败,单位查找成功但未返回有效数据") } });
            return;
        }
        callback(UnitListResult {
            true,
            result.units,
            std::nullopt });
    });
}
// 列出所有分类
void MasterDataService::listCategories(QObject* owner, bool activeOnly, const CategoryListCallback callback) const
{
    QPointer<QObject> ownerPtr(owner);
    if (ownerPtr.isNull() || !callback)
        return;
    if (auto error = authorize(); error.has_value()) {
        callback(
            CategoryListResult {
                false,
                std::nullopt,
                error.value() });
        return;
    }
    repository_.listCategories(activeOnly, ownerPtr.data(), [ownerPtr,callback = std::move(callback)](const CategoryListResult& result) {
        if (ownerPtr.isNull() || !callback)
            return;
        if (!result.success) {
            callback(CategoryListResult {
                false,
                std::nullopt,
                result.error.has_value() ? result.error.value() : AppError { AppErrorCategory::Validation, AppErrorCode::UnknownError, QStringLiteral("列出分类失败,未知错误") } });
            return;
        }
        if (result.error.has_value()) {
            callback(CategoryListResult {
                false,
                std::nullopt,
                result.error.value() });
            return;
        }
        if (!result.categories.has_value()) {
            callback(CategoryListResult {
                false,
                std::nullopt,
                AppError {
                    AppErrorCategory::Validation,
                    AppErrorCode::UnknownError,
                    QStringLiteral("列出分类失败,分类查找成功但未返回有效数据") } });
            return;
        }
        callback(CategoryListResult {
            true,
            result.categories,
            std::nullopt });
    });
}