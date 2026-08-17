# PageNavigator
qt_add_library(WmsPageNavigator STATIC
    PageNavigator.cpp PageNavigator.h
)
# LoginPage
qt_add_library(WmsLoginPage STATIC
    LoginPage.cpp LoginPage.h
)
# ProductPage
qt_add_library(WmsProductPage STATIC
    ProductPage.cpp ProductPage.h
)

# InboundPage
qt_add_library(WmsInboundPage STATIC
    InboundPage.cpp InboundPage.h
)

# OutboundPage
qt_add_library(WmsOutboundPage STATIC
    OutboundPage.cpp OutboundPage.h
)

# 总库
qt_add_library(WmsPages INTERFACE)

target_link_libraries(WmsPageNavigator PUBLIC Qt6::Widgets Qt6::Core)

target_link_libraries(WmsLoginPage PUBLIC
    Qt6::Widgets Qt6::Core
    WmsAuthServices
)
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
    WmsProductServices
    WmsInboundServices
    WmsInboundTableModel
    WmsInboundEditDialog
    WmsInboundOrderDetailDialog
    WmsDomain
    WmsMasterDataServices
)
target_link_libraries(WmsOutboundPage PUBLIC
    Qt6::Widgets Qt6::Core
    WmsPageNavigator
    WmsProductServices
    WmsOutboundServices
    WmsOutboundTableModel
    WmsOutboundEditDialog
    WmsOutboundOrderDetailDialog
    WmsDomain
    WmsMasterDataServices
)
target_link_libraries(WmsPages
    INTERFACE
    WmsPageNavigator
    WmsLoginPage
    WmsProductPage
    WmsInboundPage
    WmsOutboundPage
)
target_include_directories(WmsPageNavigator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(WmsLoginPage PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/src/infrastructure/config )
target_include_directories(WmsProductPage PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(WmsInboundPage PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(WmsOutboundPage PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
