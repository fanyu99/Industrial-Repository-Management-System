qt_add_library(WmsDialogs STATIC 
ProductEditDialog.cpp ProductEditDialog.h
)
target_link_libraries(WmsDialogs PUBLIC 
Qt6::Widgets WmsDto
)
target_include_directories(WmsDialogs PUBLIC 
${CMAKE_CURRENT_SOURCE_DIR}
)
