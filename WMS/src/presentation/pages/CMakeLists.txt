qt_add_library(WmsPages STATIC
    ProductPage.cpp ProductPage.h
)
target_link_libraries(WmsPages PUBLIC
    Qt6::Widgets Qt6::Core
    WmsApplication
    WmsModels
    WmsDialogs
    WmsDomain
    WmsMasterDataServices
)
target_include_directories(WmsPages PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})