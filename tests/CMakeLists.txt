# TSN Test Suite CMake Configuration
cmake_minimum_required(VERSION 3.16)

message(STATUS "🔧 Processing TSN tests directory")

# Define TSN test targets if Intel HAL is available
if(INTEL_HAL_FOUND OR TARGET intel-ethernet-hal-static)
    message(STATUS "✅ TSN Tests: Intel HAL found, creating comprehensive TSN test")
    
    # Comprehensive TSN test - full validation
    add_executable(comprehensive_tsn_test
        comprehensive_tsn_test.c
    )
    
    # Link against Intel HAL
    if(TARGET intel-ethernet-hal-static)
        target_link_libraries(comprehensive_tsn_test 
            PRIVATE intel-ethernet-hal-static
        )
    endif()
    
    # Windows-specific libraries
    if(WIN32)
        target_link_libraries(comprehensive_tsn_test 
            PRIVATE ws2_32 setupapi
        )
    endif()
    
    # Include directories
    target_include_directories(comprehensive_tsn_test 
        PRIVATE ${CMAKE_SOURCE_DIR}/thirdparty/intel-ethernet-hal/include
    )
    
    # Set output directory
    set_target_properties(comprehensive_tsn_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/tests/Debug"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/tests/Release"
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/tests/RelWithDebInfo"
    )
    
    # Add to default build
    set_target_properties(comprehensive_tsn_test PROPERTIES
        FOLDER "Tests/TSN"
    )
    
    message(STATUS "TSN Tests: comprehensive_tsn_test target created")
else()
    message(WARNING "Intel HAL not available - TSN tests disabled")
endif()

# Simple TSN test - minimal validation (always available)
add_executable(corrected_tsn_test
    corrected_tsn_test.c
)

if(TARGET intel-ethernet-hal-static)
    target_link_libraries(corrected_tsn_test 
        PRIVATE intel-ethernet-hal-static
    )
endif()

if(WIN32)
    target_link_libraries(corrected_tsn_test 
        PRIVATE ws2_32 setupapi
    )
endif()

target_include_directories(corrected_tsn_test 
    PRIVATE ${CMAKE_SOURCE_DIR}/thirdparty/intel-ethernet-hal/include
)

set_target_properties(corrected_tsn_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/tests/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/tests/Release" 
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/tests/RelWithDebInfo"
)

# Unified TSN Architecture Test - demonstrates proper Intel HAL -> intel_avb integration
add_executable(unified_tsn_test
    unified_tsn_test.c
)

# Note: intel_avb is not built as a static library, so we just use the headers
target_include_directories(unified_tsn_test 
    PRIVATE ${CMAKE_SOURCE_DIR}/lib/intel_avb/lib
)

set_target_properties(unified_tsn_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/tests/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/tests/Release" 
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/tests/RelWithDebInfo"
)
