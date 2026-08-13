qt_add_library(
    WmsDomain INTERFACE
)
add_subdirectory(auth)
add_subdirectory(product)
add_subdirectory(inbound)
add_subdirectory(outbound)
target_link_libraries(WmsDomain INTERFACE
    WmsAuthDomain
    WmsProductDomain
    WmsInboundDomain
    WmsOutboundDomain
)
target_include_directories(WmsDomain INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/auth
    ${CMAKE_CURRENT_SOURCE_DIR}/product
    ${CMAKE_CURRENT_SOURCE_DIR}/inbound
    ${CMAKE_CURRENT_SOURCE_DIR}/outbound
)