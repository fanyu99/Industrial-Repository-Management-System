cmake_minimum_required(VERSION 3.16)
project(db_test 
    LANGUAGES CXX)
find_package(Qt6 REQUIRED COMPONENTS Core Sql Widgets Charts)
qt_add_executable(db_test main.cpp)
target_link_libraries(db_test PUBLIC Qt6::Sql Qt6::Core Qt6::Widgets Qt6::Charts)
