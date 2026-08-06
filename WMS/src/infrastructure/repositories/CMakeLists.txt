qt_add_library(WmsRepositories STATIC
    MySqlProductRepository.cpp
    MySqlProductRepository.h
    MySqlAuthRepository.cpp
    MySqlAuthRepository.h
    MySqlInboundRepository.cpp
    MySqlInboundRepository.h
)
target_link_libraries(
    WmsRepositories
    PRIVATE
    WmsAuthServices
    WmsProductServices
    WmsDatabase
    WmsInboundServices
)
target_include_directories(WmsRepositories PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
