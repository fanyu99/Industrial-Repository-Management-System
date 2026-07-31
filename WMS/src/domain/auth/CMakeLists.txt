# WMS认证模块 WmsAuthDomain 接口
qt_add_library(WmsAuthDomain STATIC
    AuthenticatedUser.h
    Permission.h
    Role.h
    AuthCredentialRecord.h
    PasswordHashTypes.h
)
target_link_libraries(WmsAuthDomain PUBLIC
    Qt6::Core
)
target_include_directories(WmsAuthDomain PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)