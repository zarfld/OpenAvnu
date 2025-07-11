cmake_minimum_required(VERSION 3.15)
project(phase2_test_consolidated)

# Set up include paths for our dependencies  
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/avtp_pipeline/avtp_open1722
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/avtp_pipeline/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/avtp_pipeline/platform/Windows
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/avtp_pipeline/platform/generic
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/avtp_pipeline/platform/platTCAL/GNU
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../thirdparty/open1722/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../thirdparty/open1722/src
)

# Create the test executable with unique name
add_executable(test_phase2_integration_consolidated 
    test_phase2_integration.c 
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/avtp_pipeline/avtp_open1722/openavb_avtp_open1722_simple.c
)

# Link with Open1722 library if available
if(TARGET open1722)
    target_link_libraries(test_phase2_integration_consolidated open1722)
else()
    message(WARNING "Open1722 library not found - phase2 integration test may not link properly")
endif()

# Compiler definitions for Windows/MSVC
if(WIN32)
    target_compile_definitions(test_phase2_integration_consolidated PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()
