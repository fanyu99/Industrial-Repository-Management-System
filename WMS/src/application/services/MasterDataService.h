// 仓库基础数据Service : unit,category
#pragma once
#include "IMasterDataRepository.h"
#include "Permission.h"
#include "SessionManager.h"
#include <QObject>
#include <QString>
class MasterDataService : public QObject {
    Q_OBJECT
public:
    using UnitListCallback = IMasterDataRepository::UnitListCallback;
    using CategoryListCallback = IMasterDataRepository::CategoryListCallback;
    using WarehouseListCallback = IMasterDataRepository::WarehouseListCallback;
    explicit MasterDataService(IMasterDataRepository& repository, SessionManager& sessionManager, QObject* parent = nullptr);
    ~MasterDataService() = default;
    // 列出所有单位(是否活跃)
    void listUnits(QObject* owner, bool activeOnly, const UnitListCallback callback) const;
    // 列出所有分类(是否活跃)
    void listCategories(QObject* owner, bool activeOnly, const CategoryListCallback callback) const;
    // 列出所有仓库(是否活跃)
    void listWarehouses(QObject* owner, bool activeOnly, const WarehouseListCallback callback) const;
    // 获取当前用户
    std::optional<AuthenticatedUser> currentUser() const;
    // 校验是否有权限
    [[nodiscard]] bool hasPermission(Permission permission) const;
    // 校验用户是否有权限(默认为查看产品)并返回应用错误
    [[nodiscard]] std::optional<AppError> authorize(Permission permission = Permission::ViewProducts) const;

private:
    IMasterDataRepository& repository_; // 基础数据
    SessionManager& sessionManager_;
};