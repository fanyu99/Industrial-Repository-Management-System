cmake_minimum_required(VERSION 3.16)
project(WmsCores
    LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Sql)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(ConnectionPool)
add_subdirectory(database)
