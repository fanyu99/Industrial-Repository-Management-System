# WMS核心 WmsCore
qt_add_library(WmsCore STATIC)
add_subdirectory(database)
target_link_libraries(WmsCore PRIVATE
    WmsDatabase)
