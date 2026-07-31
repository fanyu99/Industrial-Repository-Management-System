qt_add_library(
    WmsPresentation STATIC 
)
add_subdirectory(models)
add_subdirectory(pages)
add_subdirectory(dialogs)
target_link_libraries(WmsPresentation PUBLIC 
WmsModels WmsPages WmsDialogs
)
target_include_directories(
    WmsPresentation
    PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)
