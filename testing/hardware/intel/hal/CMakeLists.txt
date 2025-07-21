# Intel HAL Validation Test Suite CMake Configuration

cmake_minimum_required(VERSION 3.10)

# Test configuration
project(IntelHALValidationTest)

# C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Find Intel HAL include directory
find_path(INTEL_HAL_INCLUDE_DIR 
    NAMES intel_ethernet_hal.h
    PATHS 
        "${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty/intel-ethernet-hal/include"
        "${CMAKE_SOURCE_DIR}/thirdparty/intel-ethernet-hal/include"
    NO_DEFAULT_PATH
)

# Create the validation test executable with unique name
add_executable(intel_hal_validation_test_consolidated intel_hal_validation_test.c)

# Set include directories
target_include_directories(intel_hal_validation_test_consolidated PRIVATE 
    ${INTEL_HAL_INCLUDE_DIR}
)

# Link to Intel HAL target (not library path)
if(TARGET intel-ethernet-hal-static)
    target_link_libraries(intel_hal_validation_test_consolidated 
        intel-ethernet-hal-static
        wsock32
        ws2_32
        iphlpapi
    )
    message(STATUS "  - Intel HAL Target: intel-ethernet-hal-static (static library)")
elseif(TARGET intel-ethernet-hal)
    target_link_libraries(intel_hal_validation_test_consolidated 
        intel-ethernet-hal
        wsock32
        ws2_32
        iphlpapi
    )
    message(STATUS "  - Intel HAL Target: intel-ethernet-hal (shared library)")
else()
    message(FATAL_ERROR "Intel HAL target not found. Ensure Intel HAL is built before tests.")
endif()

# Windows-specific settings
if(WIN32)
    target_compile_definitions(intel_hal_validation_test_consolidated PRIVATE 
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
    )
endif()

# Set output directory
set_target_properties(intel_hal_validation_test_consolidated PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/tests"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/tests"
)

# Create tests directory
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/tests")

# Display configuration info
message(STATUS "Intel HAL Validation Test Configuration:")
message(STATUS "  - Intel HAL Include: ${INTEL_HAL_INCLUDE_DIR}")
message(STATUS "  - Output Directory: ${CMAKE_BINARY_DIR}/tests")

# Add as a test if CTest is enabled AND not in CI environment
# Hardware tests require real Intel adapters and will fail on virtual machines
set(IS_CI_BUILD OFF)
if(DEFINED ENV{CI} OR DEFINED ENV{GITHUB_ACTIONS} OR DEFINED ENV{TRAVIS} OR DEFINED ENV{APPVEYOR})
    set(IS_CI_BUILD ON)
endif()

if((CMAKE_TESTING_ENABLED OR BUILD_TESTING) AND NOT IS_CI_BUILD)
    enable_testing()
    add_test(NAME IntelHALValidation COMMAND intel_hal_validation_test_consolidated)
    set_tests_properties(IntelHALValidation PROPERTIES
        TIMEOUT 60
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
    )
    message(STATUS "✅ Intel HAL validation test registered (hardware environment)")
elseif(IS_CI_BUILD)
    message(STATUS "⏭️  Skipping Intel HAL validation test (CI environment - no real hardware)")
endif()
