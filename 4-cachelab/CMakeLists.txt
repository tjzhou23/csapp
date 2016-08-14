cmake_minimum_required(VERSION 3.3)
project(4_cachelab)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
    cachelab.c
    cachelab.h
    contracts.h
    test-trans.c
    tracegen.c
    trans.c csim.c)

add_executable(4_cachelab ${SOURCE_FILES})