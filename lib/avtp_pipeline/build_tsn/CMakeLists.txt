cmake_minimum_required(VERSION 3.10)

# Intel TSN Integration for AVTP Pipeline
project(intel_tsn_integration)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Add include directories
include_directories(../intel_avb/lib)
include_directories(include)

# Define the TSN integration library
add_library(intel_tsn_integration STATIC
    intel_tsn_integration.c
    intel_tsn_integration.h
)

# Link with Intel AVB library if available
if(TARGET intel_avb)
    target_link_libraries(intel_tsn_integration intel_avb)
else()
    # Fallback to direct library path
    find_library(INTEL_AVB_LIB intel_avb PATHS ../intel_avb/lib)
    if(INTEL_AVB_LIB)
        target_link_libraries(intel_tsn_integration ${INTEL_AVB_LIB})
    endif()
endif()

# Add test executable
add_executable(test_intel_tsn_integration
    test_intel_tsn_integration.c
)

# Link test with TSN integration library
target_link_libraries(test_intel_tsn_integration intel_tsn_integration)

# Add install targets
install(TARGETS intel_tsn_integration
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES intel_tsn_integration.h
    DESTINATION include
)

# Add to parent project if building as part of larger project
if(CMAKE_PROJECT_NAME STREQUAL "openavnu" OR CMAKE_PROJECT_NAME STREQUAL "avtp_pipeline")
    set_target_properties(intel_tsn_integration PROPERTIES FOLDER "avtp_pipeline")
    set_target_properties(test_intel_tsn_integration PROPERTIES FOLDER "avtp_pipeline/tests")
endif()

# Compiler-specific options
if(WIN32)
    target_compile_definitions(intel_tsn_integration PRIVATE _WIN32_WINNT=0x0601)
    target_link_libraries(intel_tsn_integration ws2_32 iphlpapi)
    target_link_libraries(test_intel_tsn_integration ws2_32 iphlpapi)
endif()

# Debug options
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(intel_tsn_integration PRIVATE DEBUG=1)
    target_compile_definitions(test_intel_tsn_integration PRIVATE DEBUG=1)
endif()

message(STATUS "Intel TSN Integration configured")
message(STATUS "  Library: intel_tsn_integration")  
message(STATUS "  Test: test_intel_tsn_integration")
message(STATUS "  Platform: ${CMAKE_SYSTEM_NAME}")
