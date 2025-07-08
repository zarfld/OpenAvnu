# OpenAvnu Clock Quality Demo
cmake_minimum_required(VERSION 3.15)
project(clock_quality_demo)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty/gptp/common)

# Source files
set(CLOCK_QUALITY_SOURCES
    ../../thirdparty/gptp/common/gptp_clock_quality.cpp
    ../../thirdparty/gptp/common/gptp_profile.cpp
    ../../thirdparty/gptp/common/gptp_clock_quality_config.cpp
    gptp_log_stub.cpp
)

# Create main demo executable
add_executable(clock_quality_demo clock_quality_demo.cpp ${CLOCK_QUALITY_SOURCES})

# Create configuration demo executable
add_executable(config_demo config_demo.cpp ${CLOCK_QUALITY_SOURCES})

# Windows-specific settings
if(WIN32)
    target_compile_definitions(clock_quality_demo PRIVATE 
        NOMINMAX 
        WIN32_LEAN_AND_MEAN
        _CRT_SECURE_NO_WARNINGS
    )
    target_compile_definitions(config_demo PRIVATE 
        NOMINMAX 
        WIN32_LEAN_AND_MEAN
        _CRT_SECURE_NO_WARNINGS
    )
endif()

# Copy configuration file to build directory
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/clock_quality_config.ini 
               ${CMAKE_CURRENT_BINARY_DIR}/clock_quality_config.ini COPYONLY)

# Compiler-specific options
if(MSVC)
    target_compile_options(clock_quality_demo PRIVATE /W3)
else()
    target_compile_options(clock_quality_demo PRIVATE -Wall -Wextra)
endif()

# Install target
install(TARGETS clock_quality_demo DESTINATION bin)
