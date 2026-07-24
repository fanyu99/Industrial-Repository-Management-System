# WMS安全模块 WmsSecurity
qt_add_library(WmsSecurity STATIC
    PasswordHasher.cpp
    PasswordHasher.h
    SessionManager.cpp
    SessionManager.h
)
target_link_libraries(WmsSecurity PUBLIC Qt6::Core Qt6::Network WmsDomain)
target_include_directories(WmsSecurity PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)