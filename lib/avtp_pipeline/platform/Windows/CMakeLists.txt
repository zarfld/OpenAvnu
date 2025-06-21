cmake_minimum_required(VERSION 2.6)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
project(AVB)

message("-- Using Windows toolchain")

# Default build type handling copied from Linux version
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build" FORCE)
else()
  if(NOT (("${CMAKE_BUILD_TYPE}" STREQUAL "Release") OR
          ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug") OR
          ("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo") OR
          ("${CMAKE_BUILD_TYPE}" STREQUAL "MinSizeRel")))
    message(FATAL_ERROR "Unknown CMAKE_BUILD_TYPE")
  endif()
endif()
message("-- Build type is ${CMAKE_BUILD_TYPE}")

# Set a define to signal build to source files
string(TOUPPER "${CMAKE_BUILD_TYPE}_BUILD" BUILD_TYPE_STRING)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D${BUILD_TYPE_STRING}")

# Windows specific compile flags
if(MSVC)
  add_definitions(/W3 /D_CRT_SECURE_NO_WARNINGS)
else()
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
endif()

# include original Linux build logic for remaining targets
include(${CMAKE_CURRENT_LIST_DIR}/../Linux/CMakeLists.txt)

