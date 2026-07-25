qt_add_library(
    WmsProductDomain STATIC
    product.h
)
target_include_directories(WmsProductDomain PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
