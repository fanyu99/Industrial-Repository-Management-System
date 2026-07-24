# 配置管理器 ConfigManager
qt_add_library(WmsConfigManager STATIC
    ConfigManager.cpp
    ConfigManager.h
    IniHelper.cpp
    IniHelper.h
    DatabaseConfigLoader.cpp
    DatabaseConfigLoader.h
)
target_link_libraries(WmsConfigManager PUBLIC Qt6::Core WmsDatabase)
target_include_directories(WmsConfigManager
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)