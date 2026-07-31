// ProductRequests.h 产品请求
#pragma once
#include <QString>
// 创建产品请求
struct CreateProductRequest {
    QString code;
    QString name;
    quint32 categoryId { 0 };
    quint32 unitId { 0 };
    QString specification;
    int safetyStock { 0 };
    bool active { true };
    bool isValid() const noexcept
    {
        return !code.trimmed().isEmpty() && !name.trimmed().isEmpty() && categoryId > 0 && unitId > 0 && safetyStock >= 0;
    }
};
// 更新产品请求
struct UpdateProductRequest {
    quint32 id { 0 };
    QString code;
    QString name;
    quint32 categoryId { 0 };
    quint32 unitId { 0 };
    QString specification;
    int safetyStock { 0 };
    bool active { true };
    bool isValid() const noexcept
    {
        return !code.trimmed().isEmpty() && !name.trimmed().isEmpty() && categoryId > 0 && unitId > 0 && safetyStock >= 0;
    }
};