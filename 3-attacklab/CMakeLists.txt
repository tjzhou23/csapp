cmake_minimum_required(VERSION 3.3)
project(3_buffer736)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
    farm.c)

add_executable(3_buffer736 ${SOURCE_FILES})