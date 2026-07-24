// 用户权限
#pragma once
#include "Role.h"
// 用户权限
enum class Permission {
    ViewDashboard, // 查看板
    ViewProducts, // 查看商品
    CreateProducts, // 创建商品
    EditProducts, // 编辑商品
    DisableProducts, // 禁用商品

    ViewInboundOrders, // 查看入库订单
    CreateInboundOrders, // 创建入库订单
    ConfirmInboundOrders, // 确认入库订单

    ViewOutboundOrders, // 查看出库订单
    CreateOutboundOrders, // 创建出库订单
    ConfirmOutboundOrders, // 确认出库订单

    ViewUsers, // 查看用户
    ManageUsers, // 管理用户

    ViewAuditLogs, // 查看审计日志
    ExportData, // 导出数据
};
// 用户权限检查
inline bool roleHasPermission(Role role, Permission permission)
{
    switch (role) {
        // 管理员权限
    case Role::Admin:
        return true;
        // 经理权限
    case Role::Manager:
        switch (permission) {
        case Permission::ViewDashboard:
        case Permission::ViewProducts:
        case Permission::ViewInboundOrders:
        case Permission::ViewOutboundOrders:
        case Permission::ExportData:
            return true;
        default:
            return false;
        }
        // 操作员权限
    case Role::Operator:
        switch (permission) {
        case Permission::ViewDashboard:
        case Permission::ViewProducts:
        case Permission::ViewInboundOrders:
        case Permission::ViewOutboundOrders:
        case Permission::CreateInboundOrders:
        case Permission::CreateOutboundOrders:
        case Permission::ConfirmInboundOrders:
        case Permission::ConfirmOutboundOrders:
            return true;
        default:
            return false;
        }
    }
    return false;
}