#pragma once
#include "MasterDataDto.h"
#include <QString>
#include <QObject>
#include <functional>
class IMasterDataRepository {
public:
    using UnitListCallback = std::function<void(const UnitListResult&)>; // 单位列出回调
    using CategoryListCallback = std::function<void(const CategoryListResult&)>; // 分类列出回调
    virtual ~IMasterDataRepository() = default;
    // 列出所有单位(是否只展示活跃)
    virtual void listUnits(bool activeOnly, QObject* owner, const UnitListCallback callback) = 0;
    // 列出所有分类(是否只展示活跃)
    virtual void listCategories(bool activeOnly, QObject* owner, const CategoryListCallback callback)  = 0;
};