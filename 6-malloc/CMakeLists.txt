cmake_minimum_required(VERSION 3.3)
project(6_malloc)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
    clock.c
    clock.h
    config.h
    contracts.h
    fcyc.c
    fcyc.h
    fsecs.c
    fsecs.h
    ftimer.c
    ftimer.h
    mdriver.c
    memlib.c
    memlib.h
#    mm-naive.c
#    mm-textbook.c
    mm.c
    mm.h)

add_definitions( -DDRIVER )

add_executable(6_malloc ${SOURCE_FILES})