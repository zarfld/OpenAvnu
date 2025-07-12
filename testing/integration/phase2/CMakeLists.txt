cmake_minimum_required(VERSION 3.15)
project(phase2_backend_integration_test)

# Simple backend integration test without AVTP dependencies
add_executable(test_phase2_backend_integration_consolidated 
    test_phase2_backend_integration.c
)

# Phase 2 AVB integration prototype
add_executable(phase2_avb_integration_prototype 
    phase2_avb_integration_prototype.c
)

# Phase 2 User feedback simulation
add_executable(phase2_user_feedback_simulation 
    phase2_user_feedback_simulation.c
)

# Phase 2 User feedback collection tool
add_executable(phase2_user_feedback_collection 
    phase2_user_feedback_collection.c
)

# Windows-specific libraries
if(WIN32)
    target_link_libraries(test_phase2_backend_integration_consolidated 
        kernel32   # For QueryPerformanceCounter
    )
    target_compile_definitions(test_phase2_backend_integration_consolidated PRIVATE
        _WIN32_WINNT=0x0601  # Windows 7+
        _CRT_SECURE_NO_WARNINGS
    )
    
    target_link_libraries(phase2_avb_integration_prototype 
        kernel32   # For QueryPerformanceCounter
    )
    target_compile_definitions(phase2_avb_integration_prototype PRIVATE
        _WIN32_WINNT=0x0601  # Windows 7+
        _CRT_SECURE_NO_WARNINGS
    )
    
    target_link_libraries(phase2_user_feedback_simulation 
        kernel32   # For basic Windows APIs
    )
    target_compile_definitions(phase2_user_feedback_simulation PRIVATE
        _WIN32_WINNT=0x0601  # Windows 7+
        _CRT_SECURE_NO_WARNINGS
    )
    
    target_link_libraries(phase2_user_feedback_collection 
        kernel32   # For basic Windows APIs
    )
    target_compile_definitions(phase2_user_feedback_collection PRIVATE
        _WIN32_WINNT=0x0601  # Windows 7+
        _CRT_SECURE_NO_WARNINGS
    )
endif()

# Test registration
if(ENABLE_TESTING)
    add_test(NAME test_phase2_backend_integration
        COMMAND test_phase2_backend_integration_consolidated
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    
    set_tests_properties(test_phase2_backend_integration PROPERTIES
        LABELS "integration;phase2;backend"
        TIMEOUT 60  # 1 minute max
    )
    
    add_test(NAME phase2_avb_integration_prototype
        COMMAND phase2_avb_integration_prototype
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    
    set_tests_properties(phase2_avb_integration_prototype PROPERTIES
        LABELS "integration;phase2;prototype;avb"
        TIMEOUT 60  # 1 minute max
    )
    
    add_test(NAME phase2_user_feedback_simulation
        COMMAND phase2_user_feedback_simulation
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    
    set_tests_properties(phase2_user_feedback_simulation PROPERTIES
        LABELS "integration;phase2;feedback;simulation"
        TIMEOUT 30  # 30 seconds max
    )
    
    add_test(NAME phase2_user_feedback_collection
        COMMAND phase2_user_feedback_collection
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    
    set_tests_properties(phase2_user_feedback_collection PROPERTIES
        LABELS "integration;phase2;feedback;collection"
        TIMEOUT 60  # 1 minute max
    )
endif()

# Custom target for easy execution
add_custom_target(run_phase2_backend_integration_test
    COMMAND test_phase2_backend_integration_consolidated
    DEPENDS test_phase2_backend_integration_consolidated
    COMMENT "Running Phase 2 backend integration test"
    VERBATIM
)

add_custom_target(run_phase2_avb_prototype
    COMMAND phase2_avb_integration_prototype
    DEPENDS phase2_avb_integration_prototype
    COMMENT "Running Phase 2 AVB integration prototype"
    VERBATIM
)

add_custom_target(run_phase2_user_feedback_simulation
    COMMAND phase2_user_feedback_simulation
    DEPENDS phase2_user_feedback_simulation
    COMMENT "Running Phase 2 User Feedback Simulation"
    VERBATIM
)

add_custom_target(run_phase2_user_feedback_collection
    COMMAND phase2_user_feedback_collection
    DEPENDS phase2_user_feedback_collection
    COMMENT "Running Phase 2 User Feedback Collection Tool"
    VERBATIM
)
