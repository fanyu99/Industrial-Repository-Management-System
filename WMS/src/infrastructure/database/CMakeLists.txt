# WMS核心的数据库模块 WmsDatabase
qt_add_library(WmsDatabase STATIC
    DatabaseExecutor.cpp
    DatabaseExecutor.h
    DatabaseTypes.h
    DatabaseWorker.cpp
    DatabaseWorker.h
)
target_link_libraries(WmsDatabase PRIVATE Qt6::Core Qt6::Sql)
target_include_directories(WmsDatabase PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})