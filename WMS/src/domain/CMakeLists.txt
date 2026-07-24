qt_add_library(
    WmsDomain STATIC
)
add_subdirectory(auth)
target_link_libraries(WmsDomain PUBLIC
    WmsAuthDomain
)
target_include_directories(WmsDomain PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)