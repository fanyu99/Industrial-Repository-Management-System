qt_add_library(WmsAuthServices STATIC
    AuthServices.cpp
    AuthServices.h
    IAuthRepository.h
    FakeAuthRepository.h
)
qt_add_library(WmsProductServices STATIC
    ProductService.h
    ProductService.cpp
    IProductRepository.h
    FakeProductRepository.h
)
target_link_libraries(WmsAuthServices PUBLIC
    WmsCore
    WmsDomain
    WmsSecurity
)
target_link_libraries(
    WmsProductServices PUBLIC
    WmsCore
    WmsDomain
    WmsSecurity
)
target_include_directories(WmsAuthServices PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../errors
    ${CMAKE_CURRENT_SOURCE_DIR}/../dto
)
target_include_directories(WmsProductServices PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../errors
    ${CMAKE_CURRENT_SOURCE_DIR}/../dto
)
