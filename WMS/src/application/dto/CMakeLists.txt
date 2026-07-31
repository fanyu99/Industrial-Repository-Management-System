qt_add_library(
    WmsDto INTERFACE
    ProductDto.h
    LoginDto.h
    ProductRequests.h
)
target_include_directories(WmsDto INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
