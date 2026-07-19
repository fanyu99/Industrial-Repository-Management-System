# WMS安全模块 PasswordHasher
qt_add_library(PasswordHasher
    PasswordHasher.cpp
    PasswordHasher.h
)
target_link_libraries(PasswordHasher Qt6::Core Qt6::Network)
