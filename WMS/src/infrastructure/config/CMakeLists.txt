# 配置管理器 ConfigManager
qt_add_library(ConfigManager
    ConfigManager.cpp
    ConfigManager.h
    IniHelper.cpp
    IniHelper.h
    DatabaseConfigLoader.cpp
    DatabaseConfigLoader.h
)
target_link_libraries(ConfigManager Qt6::Core)
target_include_directories(ConfigManager
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
