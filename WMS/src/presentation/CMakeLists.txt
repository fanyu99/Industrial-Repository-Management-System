qt_add_library(
    WmsPresentation STATIC 
)
add_subdirectory(models)
target_link_libraries(WmsPresentation PUBLIC 
WmsModels
)
target_include_directories(
    WmsPresentation
    PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)
