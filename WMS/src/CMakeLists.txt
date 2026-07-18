# WMS WmsApp
qt_add_executable(WmsApp
    main.cpp
)
add_subdirectory(Core)

target_link_libraries(WmsApp PRIVATE
    WmsCore
)
