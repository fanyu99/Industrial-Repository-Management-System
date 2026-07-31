qt_add_library(WmsRepositories STATIC
    MySqlProductRepository.cpp
    MySqlProductRepository.h
    MySqlAuthRepository.cpp
    MySqlAuthRepository.h
)
target_link_libraries(
    WmsRepositories
    PRIVATE
    WmsProductServices
    WmsDatabase
)
target_include_directories(WmsRepositories PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
