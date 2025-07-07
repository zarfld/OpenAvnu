cmake_minimum_required(VERSION 3.10)
project(phase2_test)

# Set up include paths for our dependencies  
include_directories(
    ../lib/avtp_pipeline/avtp_open1722
    ../lib/avtp_pipeline/include
    ../lib/avtp_pipeline/platform/Windows
    ../lib/avtp_pipeline/platform/generic
    ../lib/avtp_pipeline/platform/platTCAL/GNU
    ../thirdparty/open1722/include
    ../thirdparty/open1722/src
)

# Add the Open1722 static library path
link_directories(
    ../build/Release
)

# Create the test executable
add_executable(test_phase2_integration test_phase2_integration.c ../lib/avtp_pipeline/avtp_open1722/openavb_avtp_open1722_simple.c)

# Link with Open1722 library
target_link_libraries(test_phase2_integration open1722)

# Compiler definitions for Windows/MSVC
if(WIN32)
    target_compile_definitions(test_phase2_integration PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()
