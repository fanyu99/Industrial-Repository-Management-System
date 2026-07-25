qt_add_library(
    WmsDomain STATIC
)
add_subdirectory(auth)
add_subdirectory(product)

target_link_libraries(WmsDomain PUBLIC
    WmsAuthDomain
    WmsProductDomain
)
target_include_directories(WmsDomain PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)