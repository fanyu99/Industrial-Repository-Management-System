# OptionLoadTracker 公共刷新状态类型
qt_add_library(WmsOptionLoadTracker INTERFACE
    OptionLoadTracker.h
)

# ProductEditDialog
qt_add_library(WmsProductEditDialog STATIC
    ProductEditDialog.cpp ProductEditDialog.h
)

# InboundEditDialog
qt_add_library(WmsInboundEditDialog STATIC
    InboundEditDialog.cpp InboundEditDialog.h
)

# InboundOrderDetailDialog
qt_add_library(WmsInboundOrderDetailDialog STATIC
    InboundOrderDetailDialog.cpp InboundOrderDetailDialog.h
)

# OutboundEditDialog
qt_add_library(WmsOutboundEditDialog STATIC
    OutboundEditDialog.cpp OutboundEditDialog.h
)

# OutboundOrderDetailDialog
qt_add_library(WmsOutboundOrderDetailDialog STATIC
    OutboundOrderDetailDialog.cpp OutboundOrderDetailDialog.h
)

# 总库
qt_add_library(WmsDialogs INTERFACE)

target_link_libraries(WmsProductEditDialog PUBLIC
    Qt6::Widgets WmsDto WmsDomain
)

target_link_libraries(WmsInboundEditDialog PUBLIC
    Qt6::Widgets WmsDto
    WmsDomain
    WmsOptionLoadTracker
)
target_link_libraries(
    WmsInboundOrderDetailDialog PUBLIC
    WmsDomain
    WmsDto
    Qt6::Widgets
)

target_link_libraries(WmsOutboundEditDialog PUBLIC
    Qt6::Widgets WmsDto
    WmsDomain
    WmsOptionLoadTracker
)
target_link_libraries(
    WmsOutboundOrderDetailDialog PUBLIC
    WmsDomain
    WmsDto
    Qt6::Widgets
)

target_link_libraries(WmsDialogs
    INTERFACE
    WmsProductEditDialog
    WmsInboundEditDialog
    WmsInboundOrderDetailDialog
    WmsOutboundEditDialog
    WmsOutboundOrderDetailDialog
    WmsOptionLoadTracker
)

target_include_directories(WmsOptionLoadTracker INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(WmsProductEditDialog PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(WmsInboundOrderDetailDialog PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(WmsInboundEditDialog PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(WmsOutboundEditDialog PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(WmsOutboundOrderDetailDialog PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)