qt_add_library(WmsOutboundDomain STATIC
    OutboundOrder.h
)
target_include_directories(WmsOutboundDomain PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
