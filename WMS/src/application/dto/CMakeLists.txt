qt_add_library(
    WmsDto INTERFACE
    ProductDto.h
    LoginDto.h
    ProductRequests.h
    InboundDto.h
    InboundRequests.h
    OutboundDto.h
    OutboundRequests.h
    MasterDataDto.h
    AuditContext.h
)
target_link_libraries(WmsDto INTERFACE WmsDomain)
target_include_directories(WmsDto INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../errors
)
