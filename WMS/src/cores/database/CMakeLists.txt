qt_add_library(WmsDatabase STATIC
    DatabaseExecutor.cpp
    DatabaseExecutor.h
    DatabaseTypes.h
    DatabaseWorker.cpp
    DatabaseWorker.h
)

target_link_libraries(WmsDatabase
    PUBLIC
        Qt6::Core
        Qt6::Sql
)

target_include_directories(WmsDatabase PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(WmsDatabase PUBLIC cxx_std_17)
