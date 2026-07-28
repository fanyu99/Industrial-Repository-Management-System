qt_add_library(
    WmsApplication STATIC
)
add_subdirectory(services)
add_subdirectory(dto)

target_link_libraries(WmsApplication PUBLIC
    WmsAuthServices
    WmsProductServices
    WmsDto
)
target_include_directories(WmsApplication PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
