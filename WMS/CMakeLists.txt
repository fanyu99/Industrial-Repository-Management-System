# WMS顶层 WMS
cmake_minimum_required(VERSION 3.16)
project(WMS
    VERSION 0.1.0
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Sql Widgets Test Graphs)
qt_standard_project_setup()

add_subdirectory(src)
add_subdirectory(tests)

# 启用CTest测试(必须在顶层进行调用)
enable_testing()
