# WMS认证模块 WmsAuthDomain 接口
qt_add_library(WmsAuthDomain INTERFACE
    AuthenticatedUser.h
    Permission.h
    Role.h
    AuthCredentialRecord.h
)
target_link_libraries(WmsAuthDomain INTERFACE
    Qt6::Core
)
target_include_directories(WmsAuthDomain INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)