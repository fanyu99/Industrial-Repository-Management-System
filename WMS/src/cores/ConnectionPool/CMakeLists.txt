cmake_minimum_required(VERSION 3.16)
project(ConnectionPool
    LANGUAGES CXX)
find_package(Qt6 REQUIRED COMPONENTS Core Sql)
set(CMAKE_AUTOMOC ON)
add_library(ConnectionPool STATIC
    ConnectionPool.cpp
    ConnectionPool.h
    ScopedConnection.cpp
    ScopedConnection.h)
target_link_libraries(ConnectionPool PUBLIC Qt6::Core Qt6::Sql)
