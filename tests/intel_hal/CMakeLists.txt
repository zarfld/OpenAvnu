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

# Create the validation test executable
add_executable(intel_hal_validation_test intel_hal_validation_test.c)

# Set include directories
target_include_directories(intel_hal_validation_test PRIVATE 
    ${INTEL_HAL_INCLUDE_DIR}
)

# Link to Intel HAL target (not library path)
if(TARGET intel-ethernet-hal-static)
    target_link_libraries(intel_hal_validation_test 
        intel-ethernet-hal-static
        wsock32
        ws2_32
        iphlpapi
    )
    message(STATUS "  - Intel HAL Target: intel-ethernet-hal-static (static library)")
elseif(TARGET intel-ethernet-hal)
    target_link_libraries(intel_hal_validation_test 
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
    target_compile_definitions(intel_hal_validation_test PRIVATE 
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
    )
endif()

# Set output directory
set_target_properties(intel_hal_validation_test PROPERTIES
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

# Add as a test if CTest is enabled
if(CMAKE_TESTING_ENABLED OR BUILD_TESTING)
    enable_testing()
    add_test(NAME IntelHALValidation COMMAND intel_hal_validation_test)
    set_tests_properties(IntelHALValidation PROPERTIES
        TIMEOUT 60
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
    )
endif()
