qt_add_library(
    WmsDto STATIC
    ProductDto.h
    LoginDto.h
    ProductRequests.h
    InboundDto.h
    InboundRequests.h
)
target_link_libraries(WmsDto PRIVATE WmsDomain)
target_include_directories(WmsDto INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
