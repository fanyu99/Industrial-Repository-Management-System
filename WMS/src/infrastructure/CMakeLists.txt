# WMS核心 WmsCore
qt_add_library(WmsCore STATIC)
add_subdirectory(database)
add_subdirectory(security)
add_subdirectory(config)
target_link_libraries(WmsCore PRIVATE
    WmsDatabase
    ConfigManager
    PasswordHasher
)
