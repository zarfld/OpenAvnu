# OpenAvnu Unified Testing Framework
cmake_minimum_required(VERSION 3.15)
project(OpenAvnuTesting)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable testing
enable_testing()

# Testing configuration options
option(OPENAVNU_BUILD_CONFORMANCE_TESTS "Build Avnu Alliance conformance tests" ON)
option(OPENAVNU_BUILD_HARDWARE_TESTS "Build hardware validation tests" ON)
option(OPENAVNU_BUILD_INTEGRATION_TESTS "Build integration tests" ON)
option(OPENAVNU_BUILD_PERFORMANCE_TESTS "Build performance tests" ON)

# Detect CI environment to disable hardware-dependent tests
set(OPENAVNU_CI_BUILD OFF)
if(DEFINED ENV{CI} OR DEFINED ENV{GITHUB_ACTIONS} OR DEFINED ENV{TRAVIS} OR DEFINED ENV{APPVEYOR})
    set(OPENAVNU_CI_BUILD ON)
    message(STATUS "🤖 CI environment detected - hardware tests will be skipped")
endif()

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/gptp/common)
# Note: Intel HAL included only for specific targets that need it

# =============================================================================
# Conformance Testing
# =============================================================================
if(OPENAVNU_BUILD_CONFORMANCE_TESTS)
    message(STATUS "🧪 Building conformance tests")
    add_subdirectory(conformance/avnu_alliance/gptp/clock_quality)
    
    # Register conformance tests with CTest
    add_test(NAME AvnuAllianceGptp 
             COMMAND clock_quality_tests_consolidated
             WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/conformance/avnu_alliance/gptp/clock_quality)
    set_tests_properties(AvnuAllianceGptp PROPERTIES 
                        LABELS "conformance;avnu;gptp"
                        TIMEOUT 60)
else()
    message(STATUS "⏭️  Skipping conformance tests")
endif()

# =============================================================================
# Hardware Testing
# =============================================================================
if(OPENAVNU_BUILD_HARDWARE_TESTS AND OPENAVNU_BUILD_INTEL_HAL AND NOT OPENAVNU_CI_BUILD)
    message(STATUS "🔧 Building hardware tests")
    add_subdirectory(hardware/intel/hal)
    
    # Add comprehensive TSN test (using compatibility layer, not real Intel HAL)
    message(STATUS "✅ Adding comprehensive TSN test with compatibility layer")
    add_executable(comprehensive_tsn_test hardware/comprehensive_tsn_working.c)
    # Note: NOT linking against intel-ethernet-hal-static - using compatibility layer instead
    target_include_directories(comprehensive_tsn_test PRIVATE 
                              ${CMAKE_SOURCE_DIR}/lib/avtp_pipeline)
    if(WIN32)
        # Remove socket dependencies to avoid conflicts
        # target_link_libraries(comprehensive_tsn_test PRIVATE ws2_32 setupapi)
    endif()
        
        # Set output directory
        set_target_properties(comprehensive_tsn_test PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/tests/Debug"
            RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/tests/Release"
            RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/tests/RelWithDebInfo"
        )
    
    # Register hardware tests with CTest
    add_test(NAME IntelHalValidation 
             COMMAND intel_hal_validation_test_consolidated
             WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/hardware/intel/hal)
    set_tests_properties(IntelHalValidation PROPERTIES 
                        LABELS "hardware;intel;hal"
                        TIMEOUT 120)
    
    # Register comprehensive TSN test
    if(TARGET comprehensive_tsn_test)
        add_test(NAME ComprehensiveTSN 
                 COMMAND comprehensive_tsn_test
                 WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
        set_tests_properties(ComprehensiveTSN PROPERTIES 
                            LABELS "hardware;intel;tsn;comprehensive"
                            TIMEOUT 300)
    endif()
elseif(OPENAVNU_CI_BUILD)
    message(STATUS "⏭️  Skipping hardware tests (CI environment - no real hardware available)")
else()
    message(STATUS "⏭️  Skipping hardware tests (requires Intel HAL and real hardware)")
endif()

# =============================================================================
# Integration Testing
# =============================================================================
if(OPENAVNU_BUILD_INTEGRATION_TESTS)
    message(STATUS "🔗 Building integration tests")
    add_subdirectory(integration/phase2)
    add_subdirectory(integration/phase3)
    
    # Add AVTP Profile Validation Test
    add_executable(test_avtp_profile_validation 
                   integration/test_avtp_profile_validation.cpp
                   ../lib/avtp_pipeline/avtp_profile.cpp)
    target_include_directories(test_avtp_profile_validation PRIVATE
                              ${CMAKE_CURRENT_SOURCE_DIR}/../lib/avtp_pipeline/include
                              ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/gptp/common)
    target_compile_features(test_avtp_profile_validation PRIVATE cxx_std_17)
    
    # Register integration tests with CTest
    add_test(NAME Phase2Integration 
             COMMAND test_phase2_backend_integration_consolidated
             WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/integration/phase2)
    set_tests_properties(Phase2Integration PROPERTIES 
                        LABELS "integration;phase2"
                        TIMEOUT 180)
                        
    add_test(NAME Phase3Production 
             COMMAND phase3_production_test_consolidated
             WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/integration/phase3)
    set_tests_properties(Phase3Production PROPERTIES 
                        LABELS "integration;phase3"
                        TIMEOUT 300)
                        
    add_test(NAME AVTPProfileValidation 
             COMMAND test_avtp_profile_validation
             WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    set_tests_properties(AVTPProfileValidation PROPERTIES 
                        LABELS "integration;avtp;profiles"
                        TIMEOUT 60)
else()
    message(STATUS "⏭️  Skipping integration tests")
endif()

# =============================================================================
# Performance Testing
# =============================================================================
if(OPENAVNU_BUILD_PERFORMANCE_TESTS AND NOT OPENAVNU_CI_BUILD)
    message(STATUS "📊 Building performance tests")
    # Performance tests are primarily Python-based
    # Add any C++ performance tests here if needed
    
    # Register Python-based performance tests
    find_program(PYTHON_EXECUTABLE python3 python)
    if(PYTHON_EXECUTABLE)
        add_test(NAME TimestampAnalysisSuite 
                 COMMAND ${PYTHON_EXECUTABLE} automated_test_suite.py --config=config.json
                 WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/performance/timestamp_analysis/automated_suite)
        set_tests_properties(TimestampAnalysisSuite PROPERTIES 
                            LABELS "performance;timestamp;python"
                            TIMEOUT 600)
    else()
        message(WARNING "Python not found - skipping Python-based performance tests")
    endif()
elseif(OPENAVNU_CI_BUILD)
    message(STATUS "⏭️  Skipping performance tests (CI environment - require real hardware)")
else()
    message(STATUS "⏭️  Skipping performance tests")
endif()

# =============================================================================
# Test Result Management
# =============================================================================

# Create results directories
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/results/conformance)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/results/hardware)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/results/integration)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/results/performance)

# Custom test targets for easy execution
add_custom_target(test_conformance
    COMMAND ${CMAKE_CTEST_COMMAND} -L conformance --output-on-failure
    DEPENDS clock_quality_tests_consolidated
    COMMENT "Running conformance tests"
)

add_custom_target(test_hardware
    COMMAND ${CMAKE_CTEST_COMMAND} -L hardware --output-on-failure
    DEPENDS intel_hal_validation_test_consolidated
    COMMENT "Running hardware tests"
)

add_custom_target(test_integration
    COMMAND ${CMAKE_CTEST_COMMAND} -L integration --output-on-failure
    DEPENDS test_phase2_backend_integration_consolidated phase3_production_test_consolidated
    COMMENT "Running integration tests"
)

add_custom_target(test_performance
    COMMAND ${CMAKE_CTEST_COMMAND} -L performance --output-on-failure
    COMMENT "Running performance tests"
)

add_custom_target(test_all
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    COMMENT "Running all tests"
)

# =============================================================================
# Documentation and Reporting
# =============================================================================

# TODO: Generate test documentation
# configure_file(
#     ${CMAKE_CURRENT_SOURCE_DIR}/docs/test_config.h.in
#     ${CMAKE_CURRENT_BINARY_DIR}/test_config.h
# )

# Summary reporting
message(STATUS "")
message(STATUS "📋 OpenAvnu Testing Framework Configuration Summary:")
message(STATUS "   Conformance Tests: ${OPENAVNU_BUILD_CONFORMANCE_TESTS}")
message(STATUS "   Hardware Tests:    ${OPENAVNU_BUILD_HARDWARE_TESTS}")
message(STATUS "   Integration Tests: ${OPENAVNU_BUILD_INTEGRATION_TESTS}")
message(STATUS "   Performance Tests: ${OPENAVNU_BUILD_PERFORMANCE_TESTS}")
message(STATUS "")
message(STATUS "🎯 Quick Test Commands:")
message(STATUS "   make test_conformance  # Run conformance tests")
message(STATUS "   make test_hardware     # Run hardware tests")
message(STATUS "   make test_integration  # Run integration tests")
message(STATUS "   make test_performance  # Run performance tests")
message(STATUS "   make test_all          # Run all tests")
message(STATUS "")

# =============================================================================
# Performance Testing
# =============================================================================
if(OPENAVNU_BUILD_PERFORMANCE_TESTS)
    message(STATUS "📊 Building performance tests")
    add_subdirectory(performance)
endif()
