# 配置管理器 ConfigManager
qt_add_library(ConfigManager
    ConfigManager.cpp
    ConfigManager.h
    IniHelper.cpp
    IniHelper.h
    DatabaseConfigLoader.cpp
    DatabaseConfigLoader.h
)
target_link_libraries(ConfigManager PUBLIC Qt6::Core WmsDatabase)
target_include_directories(ConfigManager
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)