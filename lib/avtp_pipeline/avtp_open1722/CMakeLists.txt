# CMakeLists.txt for OpenAvnu Open1722 AVTP wrapper
#
# Copyright (c) 2024, OpenAvnu Project
# All rights reserved.

cmake_minimum_required(VERSION 3.10)

# Only build if Open1722 support is enabled
if(OPENAVNU_USE_OPEN1722)

project(openavnu-avtp-open1722)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../..
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../util
    ${CMAKE_CURRENT_SOURCE_DIR}/../qmgr
    ${CMAKE_CURRENT_SOURCE_DIR}/../tl
    ${CMAKE_CURRENT_SOURCE_DIR}/../intf_ctrl
    ${CMAKE_CURRENT_SOURCE_DIR}/../avtp
    ${CMAKE_CURRENT_SOURCE_DIR}/../mediaq
    ${CMAKE_CURRENT_SOURCE_DIR}/../rawsock
    ${OPEN1722_INCLUDE_DIRS}
)

# Platform-specific include paths
if(LINUX)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../platform/Linux)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../platform/platTCAL/GNU)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../platform/platOSAL/GNU)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../platform/platHAL/GNU)
elseif(WIN32)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../platform/Windows)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../platform/generic)
    # For Windows, use GNU platform abstractions as fallback
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../platform/platTCAL/GNU)
    # Note: platOSAL and platHAL don't have implementations yet
endif()

# Source files
set(AVTP_OPEN1722_SOURCES
    openavb_avtp_open1722_simple.c
)

set(AVTP_OPEN1722_HEADERS
    openavb_avtp_open1722_simple.h
    openavb_open1722_msvc_wrapper.h
)

# Test files
set(AVTP_OPEN1722_TEST_SOURCES
    test_open1722_simple.c
)

# Create library target
add_library(openavnu-avtp-open1722 STATIC ${AVTP_OPEN1722_SOURCES})

# Create test executable
add_executable(test-openavnu-avtp-open1722 ${AVTP_OPEN1722_TEST_SOURCES})
target_link_libraries(test-openavnu-avtp-open1722 openavnu-avtp-open1722)

# Link with Open1722 library
target_link_libraries(openavnu-avtp-open1722 ${OPEN1722_LIBRARIES})

# Set include directories for consumers
target_include_directories(openavnu-avtp-open1722 
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${OPEN1722_INCLUDE_DIRS}
)

# Compiler definitions
target_compile_definitions(openavnu-avtp-open1722 
    PUBLIC 
        OPENAVNU_USE_OPEN1722
)

# Install targets
install(TARGETS openavnu-avtp-open1722
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES ${AVTP_OPEN1722_HEADERS}
    DESTINATION include/openavnu/avtp_open1722
)

message(STATUS "Open1722 AVTP wrapper configured")

else()
    message(STATUS "Open1722 AVTP wrapper disabled (OPENAVNU_USE_OPEN1722=OFF)")
endif()
