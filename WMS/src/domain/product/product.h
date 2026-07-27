// 产品实体Entity,偏业务
#pragma once
#include <QString>
struct Product {
    quint32 id { 0 }; // id
    QString code; // 编码
    QString name; // 名字
    quint32 categoryId { 0 }; // 类别id
    quint32 unitId { 0 }; // 单位id
    QString specification; // 规格
    int safetyStock { 0 }; // 安全库存
    bool active { true}; // 是否激活
    // 检查产品对象是否合法
    [[nodiscard]] bool isValid() const
    {
        return !code.trimmed().isEmpty() && !name.trimmed().isEmpty() && categoryId > 0 && unitId > 0  && safetyStock >= 0;
    }
};
