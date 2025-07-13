# CMakeLists.txt for Integration Testing
cmake_minimum_required(VERSION 3.10)

# Phase 2 Backend Integration Prototype
add_executable(phase2_backend_integration_prototype
    phase2_backend_integration_prototype.c
)

# Windows-specific libraries
if(WIN32)
    target_link_libraries(phase2_backend_integration_prototype 
        kernel32   # For QueryPerformanceCounter
        winmm      # For GetTickCount  
    )
endif()

# Compiler flags
target_compile_definitions(phase2_backend_integration_prototype PRIVATE
    _WIN32_WINNT=0x0601  # Windows 7+
)

# =============================================================================
# AVTP Pipeline Phase 3 Production Integration Test
# PURPOSE: Final validation of AVTP modernization before production deployment
# SCOPE: End-to-end integration testing with Open1722, enhanced mapping, Intel HAL
# DEPENDENCIES: Phase 1 & 2 complete, Intel I219 HAL implementation
# =============================================================================

# Create the AVTP Phase 3 production integration test executable
add_executable(avtp_phase3_production_integration_test 
    avtp_phase3_production_test.c
)

# Set output directory with descriptive naming
set_target_properties(avtp_phase3_production_integration_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    OUTPUT_NAME "avtp_phase3_production_integration_test"
)

# Include directories for AVTP components
target_include_directories(avtp_phase3_production_integration_test PRIVATE
    ${CMAKE_SOURCE_DIR}/lib/avtp_pipeline/include
    ${CMAKE_SOURCE_DIR}/lib/avtp_pipeline/avtp_open1722
    ${CMAKE_SOURCE_DIR}/lib/common_pub
    ${CMAKE_SOURCE_DIR}/thirdparty/gptp/common
)

# Conditional compilation for Open1722
if(OPENAVNU_BUILD_OPEN1722)
    target_compile_definitions(avtp_phase3_production_integration_test PRIVATE OPENAVNU_USE_OPEN1722)
    target_include_directories(avtp_phase3_production_integration_test PRIVATE
        ${CMAKE_SOURCE_DIR}/thirdparty/open1722/include
    )
    
    # Link with Open1722 library if available
    if(TARGET openavnu-avtp-open1722)
        target_link_libraries(avtp_phase3_production_integration_test openavnu-avtp-open1722)
    endif()
endif()

# Platform-specific libraries
if(WIN32)
    target_link_libraries(avtp_phase3_production_integration_test 
        kernel32   # For QueryPerformanceCounter
        winmm      # For GetTickCount
    )
else()
    target_link_libraries(avtp_phase3_production_integration_test m)
endif()

# Compiler flags for AVTP test
target_compile_definitions(avtp_phase3_production_integration_test PRIVATE
    _WIN32_WINNT=0x0601  # Windows 7+
)

# =============================================================================
# Test Registration
# =============================================================================

# Test registration
if(ENABLE_TESTING)
    add_test(NAME phase2_backend_integration_prototype
        COMMAND phase2_backend_integration_prototype
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    
    set_tests_properties(phase2_backend_integration_prototype PROPERTIES
        LABELS "integration;phase2;backend;prototype"
        TIMEOUT 120  # 2 minutes max
    )
    
    add_test(NAME avtp_phase3_production_integration_test
        COMMAND avtp_phase3_production_integration_test
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    
    set_tests_properties(avtp_phase3_production_integration_test PROPERTIES
        LABELS "integration;phase3;avtp;production;modernization"
        TIMEOUT 300  # 5 minutes max for comprehensive production test
    )
endif()

# Custom target for backend integration testing
add_custom_target(test_phase2_backend_integration
    COMMAND phase2_backend_integration_prototype
    DEPENDS phase2_backend_integration_prototype
    COMMENT "Running Phase 2 backend integration prototype"
    VERBATIM
)

# Custom target for AVTP Phase 3 production integration testing
add_custom_target(test_avtp_phase3_production_integration
    COMMAND avtp_phase3_production_integration_test
    DEPENDS avtp_phase3_production_integration_test
    COMMENT "Running AVTP Phase 3 Production Integration Test Suite"
    VERBATIM
)

message(STATUS "🎵 AVTP Phase 3 Production Integration Test configured")
message(STATUS "   Purpose: Final validation of AVTP modernization before production deployment")
message(STATUS "   Executable: ${CMAKE_CURRENT_BINARY_DIR}/avtp_phase3_production_integration_test")
message(STATUS "   Test Coverage: Open1722 integration, Enhanced mapping, Intel HAL, Performance benchmarks")
