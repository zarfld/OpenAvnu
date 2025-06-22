cmake_minimum_required(VERSION 2.8)
project(grandmaster_tests)
enable_testing()

set(CPPUTEST_DIR "${CMAKE_CURRENT_LIST_DIR}/../../thirdparty/cpputest")
include_directories(${CPPUTEST_DIR}/include ${CPPUTEST_DIR}/include/Platforms/Gcc)
include_directories(../platform/Linux ../../common ../include ../platform/platTCAL/GNU ../util ../platform/generic)

link_directories(${CPPUTEST_DIR}/src/CppUTest ${CPPUTEST_DIR}/src/CppUTestExt)

add_executable(grandmaster_tests
    AllTests.cpp
    grandmaster_tests.cpp
    ../platform/Linux/openavb_grandmaster_osal.c
    ../../common/avb_gptp.c)

target_link_libraries(grandmaster_tests CppUTest CppUTestExt pthread)

add_test(grandmaster_tests grandmaster_tests)
