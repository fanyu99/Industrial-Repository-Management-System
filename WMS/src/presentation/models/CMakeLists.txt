qt_add_library(
    WmsModels STATIC 
    ProductTableModel.cpp
    ProductTableModel.h
)
target_link_libraries(WmsModels PRIVATE 
Qt6::Core 
Qt6::Widgets  
WmsDto)
target_include_directories(
    WmsModels
    PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)
