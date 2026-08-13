# 服务层
# 认证服务
qt_add_library(WmsAuthServices STATIC
    AuthServices.cpp
    AuthServices.h
    IAuthRepository.h
    FakeAuthRepository.h
)

# 产品服务
qt_add_library(WmsProductServices STATIC
    ProductService.h
    ProductService.cpp
    IProductRepository.h
    FakeProductRepository.h
)

# 入库服务
qt_add_library(WmsInboundServices STATIC
    InboundService.h
    InboundService.cpp
    IInboundRepository.h
    FakeInboundRepository.h
)

# 出库服务
qt_add_library(WmsOutboundServices STATIC
    OutboundService.h
    OutboundService.cpp
    IOutboundRepository.h
    FakeOutboundRepository.h
)

# 基础数据服务
qt_add_library(WmsMasterDataServices STATIC
    MasterDataService.cpp
    MasterDataService.h
    IMasterDataRepository.h
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
target_link_libraries(
    WmsInboundServices PUBLIC
    WmsCore
    WmsDomain
    WmsSecurity
)
target_link_libraries(
    WmsOutboundServices PUBLIC
    WmsCore
    WmsDomain
    WmsSecurity
    WmsDto
)
target_link_libraries(
    WmsMasterDataServices PUBLIC
    WmsCore
    WmsDomain
    WmsSecurity
    WmsDto
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
target_include_directories(WmsInboundServices PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../errors
    ${CMAKE_CURRENT_SOURCE_DIR}/../dto
)
target_include_directories(WmsOutboundServices PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../errors
    ${CMAKE_CURRENT_SOURCE_DIR}/../dto
)
target_include_directories(WmsMasterDataServices PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../errors
    ${CMAKE_CURRENT_SOURCE_DIR}/../dto
)