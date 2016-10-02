cmake_minimum_required(VERSION 3.3)
project(5_tshlab)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
    config.h
    csapp.c
    csapp.h
    driverhdrs.h
    driverlib.c
    driverlib.h
    fork.c
    mycat.c
    myenv.c
    myintp.c
    myints.c
    myspin1.c
    myspin2.c
    mysplit.c
    mysplitp.c
    mytstpp.c
    mytstps.c
    runtrace.c
    sdriver.c
    tsh.c)

add_executable(5_tshlab ${SOURCE_FILES})