cmake_minimum_required(VERSION 3.3)
project(7_proxy)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
#    tiny/cgi-bin/adder.c
#    tiny/csapp.c
#    tiny/csapp.h
#    tiny/tiny.c
    csapp.c
    csapp.h
    proxy.c)

add_executable(proxy ${SOURCE_FILES})