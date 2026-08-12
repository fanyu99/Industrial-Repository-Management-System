# PageNavigator
qt_add_library(WmsPageNavigator STATIC
    PageNavigator.cpp PageNavigator.h
)

# ProductPage
qt_add_library(WmsProductPage STATIC
    ProductPage.cpp ProductPage.h
)

# InboundPage
qt_add_library(WmsInboundPage STATIC
    InboundPage.cpp InboundPage.h
)

# 总库
qt_add_library(WmsPages INTERFACE)

target_link_libraries(WmsPageNavigator PUBLIC Qt6::Widgets Qt6::Core)
target_link_libraries(WmsProductPage PUBLIC
    Qt6::Widgets Qt6::Core
    WmsPageNavigator
    WmsProductServices
    WmsProductTableModel
    WmsProductEditDialog
    WmsDomain
    WmsMasterDataServices
)
target_link_libraries(WmsInboundPage PUBLIC
    Qt6::Widgets Qt6::Core
    WmsPageNavigator
    WmsInboundServices
    WmsInboundTableModel
    WmsInboundEditDialog
    WmsDomain
    WmsMasterDataServices
)
target_link_libraries(WmsPages
    INTERFACE
    WmsPageNavigator
    WmsProductPage
    WmsInboundPage
)
target_include_directories(WmsPageNavigator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(WmsProductPage PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(WmsInboundPage PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)