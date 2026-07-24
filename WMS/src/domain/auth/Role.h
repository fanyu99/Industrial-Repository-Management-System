// 用户类型
#pragma once
#include <QString>
#include <optional>
// 用户
enum class Role {
    Admin, // 管理员
    Manager, // 经理
    Operator, // 操作员
};
// 字符串转换为用户
inline std::optional<Role> stringToRole(const QString& role)
{
    if (role == "Admin")
        return Role::Admin;
    if (role == "Manager")
        return Role::Manager;
    if (role == "Operator")
        return Role::Operator;
    return std::nullopt;
}
// 用户转换为字符串
inline QString roleToString(Role role)
{
    switch (role) {
    case Role::Admin:
        return "Admin";
    case Role::Manager:
        return "Manager";
    case Role::Operator:
        return "Operator";
    default:
        return "";
    }
}