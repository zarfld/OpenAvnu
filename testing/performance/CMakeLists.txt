# CMakeLists.txt for Performance Testing
cmake_minimum_required(VERSION 3.10)

# Performance baseline measurement tool
add_executable(phase2_baseline_measurement
    phase2_baseline_measurement.c
)

# Windows-specific libraries
if(WIN32)
    target_link_libraries(phase2_baseline_measurement 
        psapi      # For GetProcessMemoryInfo
        iphlpapi   # For network interface queries
        ws2_32     # For Winsock
    )
endif()

# Compiler flags for performance testing
target_compile_definitions(phase2_baseline_measurement PRIVATE
    _WIN32_WINNT=0x0601  # Windows 7+
)

# Test registration
if(ENABLE_TESTING)
    add_test(NAME phase2_baseline_measurement
        COMMAND phase2_baseline_measurement
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    
    set_tests_properties(phase2_baseline_measurement PROPERTIES
        LABELS "performance;phase2;baseline"
        TIMEOUT 300  # 5 minutes max
    )
endif()

# Custom target for performance measurement
add_custom_target(measure_phase2_baseline
    COMMAND phase2_baseline_measurement
    DEPENDS phase2_baseline_measurement
    COMMENT "Running Phase 2 performance baseline measurement"
    VERBATIM
)
