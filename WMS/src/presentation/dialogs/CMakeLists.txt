# ProductEditDialog
qt_add_library(WmsProductEditDialog STATIC
    ProductEditDialog.cpp ProductEditDialog.h
)

# InboundEditDialog
qt_add_library(WmsInboundEditDialog STATIC
    InboundEditDialog.cpp InboundEditDialog.h
)

# 总库
qt_add_library(WmsDialogs INTERFACE)

target_link_libraries(WmsProductEditDialog PUBLIC
    Qt6::Widgets WmsDto WmsDomain
)
target_link_libraries(WmsInboundEditDialog PUBLIC
    Qt6::Widgets WmsDto
    WmsDomain
)

target_link_libraries(WmsDialogs
    INTERFACE
    WmsProductEditDialog
    WmsInboundEditDialog
)

target_include_directories(WmsProductEditDialog PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(WmsInboundEditDialog PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)