#pragma once
#include "AppError.h"
#include <QString>
#include <QVector>
#include <optional>
struct UnitDto {
    quint32 id { 0 };
    QString code;
    QString name;
    bool isActive { false };
    // 判断是否有效
    [[nodiscard]] bool isValid() const
    {
        return id != 0 && !code.trimmed().isEmpty() && !name.trimmed().isEmpty();
    }
};
struct CategoryDto {
    quint32 id { 0 };
    QString code;
    QString name;
    bool isActive { false };
    // 判断是否有效
    [[nodiscard]] bool isValid() const
    {
        return id != 0 && !code.trimmed().isEmpty() && !name.trimmed().isEmpty();
    }
};
struct WarehouseDto {
    quint32 id { 0 };
    QString code;
    QString name;
    bool isActive { false };
    [[nodiscard]] bool isValid() const
    {
        return id != 0 && !code.trimmed().isEmpty() && !name.trimmed().isEmpty();
    }
};
// 列出单位结果
struct UnitListResult {
    bool success { false };
    std::optional<QVector<UnitDto>> units; // 单位列表
    std::optional<AppError> error;
};
// 列出分类结果
struct CategoryListResult {
    bool success { false };
    std::optional<QVector<CategoryDto>> categories; // 分类列表
    std::optional<AppError> error;
};
// 列出仓库结果
struct WarehouseListResult {
    bool success { false };
    std::optional<QVector<WarehouseDto>> warehouses; // 仓库列表
    std::optional<AppError> error;
};