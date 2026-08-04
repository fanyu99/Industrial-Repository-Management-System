qt_add_library(WmsInboundDomain STATIC
    InboundOrder.h
)
target_include_directories(WmsInboundDomain PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
