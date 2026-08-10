# 产品repository
qt_add_library(WmsProductRepository STATIC
    MySqlProductRepository.cpp
    MySqlProductRepository.h
)
target_link_libraries(WmsProductRepository PRIVATE
    WmsDatabase
    WmsDomain
    WmsDto
    WmsProductServices
    WmsAuditLogRepository
)
target_include_directories(WmsProductRepository PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 认证repository
qt_add_library(WmsAuthRepository STATIC
    MySqlAuthRepository.cpp
    MySqlAuthRepository.h
)
target_link_libraries(WmsAuthRepository PRIVATE
    WmsDatabase
    WmsDomain
    WmsAuthServices
    WmsAuditLogRepository
)
target_include_directories(WmsAuthRepository PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 入库repository
qt_add_library(WmsInboundRepository STATIC
    MySqlInboundRepository.cpp
    MySqlInboundRepository.h
)
target_link_libraries(WmsInboundRepository PRIVATE
    WmsDatabase
    WmsInboundServices
    WmsAuditLogRepository
    WmsDto
)
target_include_directories(WmsInboundRepository PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 审计日志repository
qt_add_library(WmsAuditLogRepository STATIC
    MySqlAuditLogRepository.h
    MySqlAuditLogRepository.cpp
)
target_link_libraries(WmsAuditLogRepository PRIVATE
    WmsDatabase
    WmsDto
)
target_include_directories(WmsAuditLogRepository PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 基础数据repository
qt_add_library(WmsMasterDataRepository STATIC
    MySqlMasterDataRepository.h
    MySqlMasterDataRepository.cpp
)
target_link_libraries(WmsMasterDataRepository PRIVATE
    WmsDatabase
    WmsDto
    WmsMasterDataServices
)
target_include_directories(WmsMasterDataRepository PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 总repository(聚合)
qt_add_library(WmsRepositories STATIC)
target_link_libraries(
    WmsRepositories
    PRIVATE
    WmsAuthRepository
    WmsProductRepository
    WmsInboundRepository
    WmsAuditLogRepository
    WmsMasterDataRepository
)
target_include_directories(WmsRepositories PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)