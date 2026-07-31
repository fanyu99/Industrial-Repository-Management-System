qt_add_library(
    WmsDomain INTERFACE
)
add_subdirectory(auth)
add_subdirectory(product)

target_link_libraries(WmsDomain INTERFACE
    WmsAuthDomain
    WmsProductDomain
)
target_include_directories(WmsDomain INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)