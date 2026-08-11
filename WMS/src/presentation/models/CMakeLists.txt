# ProductTableModel
qt_add_library(
    WmsProductTableModel STATIC
    ProductTableModel.cpp
    ProductTableModel.h
)

# InboundTableModel
qt_add_library(
    WmsInboundTableModel STATIC
    InboundTableModel.cpp
    InboundTableModel.h
)

# 总库
qt_add_library(
    WmsModels INTERFACE)

target_link_libraries(WmsProductTableModel PUBLIC
    Qt6::Core
    Qt6::Widgets
    WmsDto
)
target_link_libraries(WmsInboundTableModel PUBLIC
    Qt6::Core
    Qt6::Widgets
    WmsDto
)
target_link_libraries(WmsModels
    INTERFACE
    WmsProductTableModel
    WmsInboundTableModel
)

target_include_directories(
    WmsModels
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(WmsProductTableModel PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(WmsInboundTableModel PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
