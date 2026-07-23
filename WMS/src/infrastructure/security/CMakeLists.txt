# WMS安全模块 PasswordHasher
qt_add_library(PasswordHasher
    PasswordHasher.cpp
    PasswordHasher.h
)
target_link_libraries(PasswordHasher PRIVATE Qt6::Core Qt6::Network)
target_include_directories(PasswordHasher PUBLIC
${CMAKE_CURRENT_SOURCE_DIR})
