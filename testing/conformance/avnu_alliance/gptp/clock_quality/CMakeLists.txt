# Clock Quality Tests - Consolidated Location
cmake_minimum_required(VERSION 3.15)
project(clock_quality_tests_consolidated)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/thirdparty/gptp/common)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Source files for clock quality framework
set(CLOCK_QUALITY_SOURCES
    ${CMAKE_SOURCE_DIR}/thirdparty/gptp/common/gptp_clock_quality.cpp
    ${CMAKE_SOURCE_DIR}/thirdparty/gptp/common/gptp_profile.cpp
    gptp_log_stub.cpp
)

# Test source files (simplified without GTest for now)
set(TEST_SOURCES
    simple_test_main.cpp
    simple_test_framework.cpp
)

# Create test executable with unique name to avoid conflicts
add_executable(clock_quality_tests_consolidated ${TEST_SOURCES} ${CLOCK_QUALITY_SOURCES})

# Windows-specific settings
if(WIN32)
    target_compile_definitions(clock_quality_tests_consolidated PRIVATE 
        NOMINMAX 
        WIN32_LEAN_AND_MEAN
        _CRT_SECURE_NO_WARNINGS
    )
endif()

# Set output directory
set_target_properties(clock_quality_tests_consolidated PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Create alias for compatibility
add_executable(clock_quality_tests_new ALIAS clock_quality_tests_consolidated)
