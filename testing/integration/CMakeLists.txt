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
endif()

# Custom target for backend integration testing
add_custom_target(test_phase2_backend_integration
    COMMAND phase2_backend_integration_prototype
    DEPENDS phase2_backend_integration_prototype
    COMMENT "Running Phase 2 backend integration prototype"
    VERBATIM
)
