qt_add_library(WmsAuthServices STATIC
    AuthServices.cpp
    AuthServices.h
    IAuthRepository.h
    FakeAuthRepository.h
)
target_link_libraries(WmsAuthServices PUBLIC
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
