# WMS WmsApp
qt_add_executable(WmsApp
    main.cpp
)
add_subdirectory(infrastructure)
add_subdirectory(domain)
add_subdirectory(application)
add_subdirectory(infrastructure/repositories)
target_link_libraries(WmsApp PRIVATE
    WmsCore
    WmsApplication
    WmsRepositories
)
