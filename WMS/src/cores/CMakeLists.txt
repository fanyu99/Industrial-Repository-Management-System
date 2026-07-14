cmake_minimum_required(VERSION 3.16)
project(Connection
    LANGUAGES CXX)
find_package(Qt6 REQUIRED COMPONENTS Core Sql)
set(CMAKE_AUTOMOC ON)
qt_add_executable(Test ConnectionPool.cpp ConnectionPool.h main.cpp ScopedConnection.cpp ScopedConnection.h)
target_link_libraries(Test PUBLIC Qt6::Sql Qt6::Core)
