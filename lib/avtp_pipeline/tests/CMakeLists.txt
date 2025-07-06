cmake_minimum_required(VERSION 3.10)
project(grandmaster_tests)
enable_testing()

set(CPPUTEST_DIR "${CMAKE_CURRENT_LIST_DIR}/../../thirdparty/cpputest")
include_directories(${CPPUTEST_DIR}/include ${CPPUTEST_DIR}/include/Platforms/Gcc)
if(WIN32)
  include_directories(../platform/Windows ../include ../platform/platTCAL/GNU ../util ../platform/generic)
else()
  include_directories(../platform/Linux ../../common ../include ../platform/platTCAL/GNU ../util ../platform/generic)
endif()

link_directories(${CPPUTEST_DIR}/src/CppUTest ${CPPUTEST_DIR}/src/CppUTestExt)

if(WIN32)
  add_executable(grandmaster_tests
      AllTests.cpp
      grandmaster_tests.cpp
      ../platform/Windows/openavb_grandmaster_osal.c)
else()
  add_executable(grandmaster_tests
      AllTests.cpp
      grandmaster_tests.cpp
      ../platform/Linux/openavb_grandmaster_osal.c
      ../../common/avb_gptp.c)
endif()

if(WIN32)
  target_link_libraries(grandmaster_tests CppUTest CppUTestExt)
else()
  target_link_libraries(grandmaster_tests CppUTest CppUTestExt pthread)
endif()

add_test(grandmaster_tests grandmaster_tests)
