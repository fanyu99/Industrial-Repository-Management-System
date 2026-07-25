qt_add_library(
    WmsApplication STATIC
)
add_subdirectory(services)
target_link_libraries(WmsApplication PUBLIC
    WmsAuthServices
    WmsProductServices
)
target_include_directories(WmsApplication PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
