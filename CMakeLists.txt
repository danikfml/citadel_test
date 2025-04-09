cmake_minimum_required(VERSION 3.10)
project(myapp)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ZLIB REQUIRED)

add_executable(myapp task3.cpp)

target_link_libraries(myapp ZLIB::ZLIB)
