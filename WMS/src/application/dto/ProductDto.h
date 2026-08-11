// 产品DTO
#pragma once
#include <QString>
#include <QVector>
#include <optional>

// 产品传输DTO,偏传输,轻量
struct ProductListItemDto {
    quint32 id { 0 };
    QString code;
    QString name;

    quint32 categoryId { 0 };
    QString categoryName;

    quint32 unitId { 0 };
    QString unitName;
    
    QString specification;
    int safetyStock { 0 };
    bool active { false };
};

// 分页模型
constexpr int PAGESIZE = 20;
// 过滤器
struct ProductFilter {
    QString keyword;
    std::optional<quint32> categoryId;
    std::optional<bool> active;
};
// 分页请求
struct PageRequest {
    int page { 1 };
    int pageSize { PAGESIZE };
};
// 获取分页结果
template <typename T>
struct PageResult {
    QVector<T> items;
    int total { 0 };
    int page { 1 };
    int pageSize { PAGESIZE };
};
