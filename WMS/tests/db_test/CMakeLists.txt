cmake_minimum_required(VERSION 3.16)
project(db_test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Sql) 

qt_add_executable(db_test main.cpp)
target_link_libraries(db_test PRIVATE Qt6::Sql Qt6::Core)
