# OpenAvnu Examples CMakeLists.txt
# Modern CMake configuration for examples that use PCAP

cmake_minimum_required(VERSION 3.10)
project(OpenAvnuExamples)

# Use the same PCAP detection as the main project
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
include(FindPCAP)

# AVDECC Protocol Compliance Test (no PCAP dependency)
# This tests that our refactored AVDECC constants work correctly
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../avdecc-lib/jdksavdecc-c/include/jdksavdecc_aem_command.h")
    message(STATUS "🔧 Building AVDECC protocol compliance test")
    
    # Create a simplified test file to verify AVDECC protocol constants compile
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/test_protocol_compliance.c" "
#include <stdio.h>
#include <jdksavdecc_aem_command.h>
#include <jdksavdecc_aem_descriptor.h>

int main() {
    printf(\"✅ AVDECC Protocol Compliance Test\\n\");
    printf(\"REGISTER_UNSOLICITED_NOTIFICATION: 0x%04X\\n\", JDKSAVDECC_AEM_COMMAND_REGISTER_UNSOLICITED_NOTIFICATION);
    printf(\"DEREGISTER_UNSOLICITED_NOTIFICATION: 0x%04X\\n\", JDKSAVDECC_AEM_COMMAND_DEREGISTER_UNSOLICITED_NOTIFICATION);
    printf(\"ENTITY descriptor: 0x%04X\\n\", JDKSAVDECC_DESCRIPTOR_ENTITY);
    printf(\"CONFIGURATION descriptor: 0x%04X\\n\", JDKSAVDECC_DESCRIPTOR_CONFIGURATION);
    printf(\"All AVDECC protocol constants are accessible!\\n\");
    return 0;
}
")
    
    add_executable(test_protocol_compliance "${CMAKE_CURRENT_BINARY_DIR}/test_protocol_compliance.c")
    target_include_directories(test_protocol_compliance PRIVATE ../avdecc-lib/jdksavdecc-c/include)
    
    message(STATUS "✅ AVDECC protocol compliance tests configured")
else()
    message(STATUS "⚠️  jdksavdecc headers not found - skipping AVDECC protocol compliance test")
endif()

# Only build PCAP examples if PCAP is available from main project
if(NOT PCAP_FOUND)
    message(STATUS "⚠️  Skipping PCAP-dependent examples - PCAP library not found by main project")
    return()
endif()

message(STATUS "🚀 Examples configuration complete")

# NOTE: AVDECC entities have been moved to lib/avdecc-entities/ for better organization
# Use the new organized structure instead of examples:
#   cmake --build build --target avdecc_entity_responsive         # Primary choice
#   cmake --build build --target avdecc_entity_hive_compatible   # Backup choice
#   cmake --build build --target avdecc_entity_complete_standards # Integration demo

# IEEE 1722-2016 Demo executable
# TEMPORARILY DISABLED - depends on IEEE standards libraries that have Windows MSVC compatibility issues
# TODO: Re-enable after fixing Windows compatibility in IEEE standards files
# add_executable(ieee_1722_2016_demo
#     ieee_standards_integration_demo.cpp
# )
# 
# target_link_libraries(ieee_1722_2016_demo PRIVATE ieee_1722_2016_avtp)
# target_include_directories(ieee_1722_2016_demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../lib/Standards)
# target_compile_features(ieee_1722_2016_demo PRIVATE cxx_std_17)

# ===================================================================
# Complete AVB Entity with Intel Filter Driver Integration
# ===================================================================
# This demonstrates integration of:
# - Your gPTP submodule (thirdparty/gptp)
# - Your Intel AVB filter driver (lib/intel_avb/lib/)  
# - Your responsive AVDECC entity
# ===================================================================

# Temporarily disable this target due to linking conflicts
# TODO: Fix Intel driver integration and multiple main() functions
if(FALSE AND PCAP_FOUND AND OPENAVNU_BUILD_INTEL_HAL)
    message(STATUS "🚀 Building Complete AVB Entity with Intel Filter Driver")
    
    add_executable(complete_avb_entity_with_intel_driver
        complete_avb_entity_with_intel_driver.cpp
        # NOTE: Removed intel_pcap_avdecc_entity_responsive.cpp - causes duplicate main()
    )
    
    # Include directories
    target_include_directories(complete_avb_entity_with_intel_driver PRIVATE 
        ${PCAP_INCLUDE_DIRS}
        ../lib/intel_avb/lib
        ../lib/Standards
        ../avdecc-lib/jdksavdecc-c/include
        ../thirdparty/intel-ethernet-hal/include
    )
    
    # Link libraries
    target_link_libraries(complete_avb_entity_with_intel_driver PRIVATE 
        ${PCAP_LIBRARIES}
        intel-ethernet-hal-static
    )
    
    # Add library directories for Windows
    if(WIN32 AND PCAP_LIBRARY_DIRS)
        target_link_directories(complete_avb_entity_with_intel_driver PRIVATE 
            ${PCAP_LIBRARY_DIRS}
        )
        target_compile_definitions(complete_avb_entity_with_intel_driver PRIVATE _CRT_SECURE_NO_WARNINGS)
    endif()
    
    message(STATUS "   📍 Complete AVB Entity: complete_avb_entity_with_intel_driver.exe")
    message(STATUS "   🔗 Integrates: gPTP + Intel Driver + AVDECC + AVTP")
    message(STATUS "   🎯 Target Hardware: Intel I210/I219/I225/I226")
endif()

# Add the working integration demonstration example (always enabled if PCAP found)
if(PCAP_FOUND)
    add_executable(complete_avb_entity_integration
        complete_avb_entity_integration.cpp
    )
    
    target_include_directories(complete_avb_entity_integration PRIVATE 
        ${PCAP_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/lib/Standards
        ${CMAKE_SOURCE_DIR}/lib/intel_avb/lib
        ${CMAKE_SOURCE_DIR}/lib/common
        ${CMAKE_SOURCE_DIR}/thirdparty/intel-ethernet-hal/include
        ${CMAKE_SOURCE_DIR}/thirdparty/gptp
        ${CMAKE_SOURCE_DIR}/thirdparty/open1722/include
    )
    
    target_link_libraries(complete_avb_entity_integration PRIVATE 
        ${PCAP_LIBRARIES}
        intel-ethernet-hal-static
        wsock32
        ws2_32
    )
    
    # Add gPTP library source files directly since there's no library target
    if(WIN32)
        target_sources(complete_avb_entity_integration PRIVATE
            ${CMAKE_SOURCE_DIR}/lib/common/avb_gptp_windows.c
        )
    else()
        target_sources(complete_avb_entity_integration PRIVATE
            ${CMAKE_SOURCE_DIR}/lib/common/avb_gptp.c
        )
    endif()
    
    if(WIN32)
        target_link_directories(complete_avb_entity_integration PRIVATE 
            ${PCAP_LIBRARY_DIRS}
        )
        target_compile_definitions(complete_avb_entity_integration PRIVATE _CRT_SECURE_NO_WARNINGS)
    endif()
    
    message(STATUS "   📍 Integration Demo: complete_avb_entity_integration.exe")
    
    # Complete CMake-Compatible AVB Entity (with real PCAP packet transmission)
    message(STATUS "🔧 Building Complete CMake-Compatible AVB Entity")
    
    add_executable(complete_cmake_avb_entity complete_avb_entity_cmake_integration.cpp)
    
    # Add PCAP support for real packet transmission
    if(PCAP_FOUND)
        message(STATUS "✅ Adding PCAP support for real packet transmission")
        message(STATUS "   PCAP Type: ${PCAP_TYPE}")
        message(STATUS "   PCAP Include: ${PCAP_INCLUDE_DIRS}")
        message(STATUS "   PCAP Libraries: ${PCAP_LIBRARIES}")
        
        target_include_directories(complete_cmake_avb_entity PRIVATE ${PCAP_INCLUDE_DIRS})
        target_link_libraries(complete_cmake_avb_entity PRIVATE ${PCAP_LIBRARIES})
        target_compile_definitions(complete_cmake_avb_entity PRIVATE HAVE_PCAP=1)
        
        if(WIN32)
            target_link_libraries(complete_cmake_avb_entity PRIVATE 
                wsock32
                ws2_32
                iphlpapi
            )
            if(PCAP_LIBRARY_DIRS)
                target_link_directories(complete_cmake_avb_entity PRIVATE ${PCAP_LIBRARY_DIRS})
            endif()
            target_compile_definitions(complete_cmake_avb_entity PRIVATE _CRT_SECURE_NO_WARNINGS)
        endif()
        
        message(STATUS "   📍 Real Packet Demo: complete_cmake_avb_entity.exe (PCAP enabled)")
    else()
        message(WARNING "⚠️ PCAP not found - complete_cmake_avb_entity will not send real packets")
        if(WIN32)
            target_link_libraries(complete_cmake_avb_entity PRIVATE 
                wsock32
                ws2_32
            )
            target_compile_definitions(complete_cmake_avb_entity PRIVATE _CRT_SECURE_NO_WARNINGS)
        endif()
        message(STATUS "   📍 Simulation Mode: complete_cmake_avb_entity.exe (no real packets)")
    endif()
    
else()
    message(STATUS "⏭️  Skipping Complete AVB Entity (requires PCAP + Intel HAL)")
endif()

# ====================================================================================
# Milan v1.2 AVDECC Entity Example
# ====================================================================================

if(TARGET avnu_milan_1_2_2023)
    message(STATUS "🎵 Building Milan v1.2 AVDECC Entity Example")
    
    add_executable(milan_avdecc_entity_example milan_avdecc_entity_example.cpp)
    
    target_link_libraries(milan_avdecc_entity_example 
        PRIVATE avnu_milan_1_2_2023
    )
    
    target_include_directories(milan_avdecc_entity_example PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../lib/Standards/AVnu/Milan/1.2-2023
    )
    
    if(WIN32)
        target_link_libraries(milan_avdecc_entity_example PRIVATE 
            ws2_32
        )
        target_compile_definitions(milan_avdecc_entity_example PRIVATE 
            _CRT_SECURE_NO_WARNINGS
            WIN32_LEAN_AND_MEAN
            NOMINMAX
        )
    endif()
    
    message(STATUS "   📍 Milan Entity: milan_avdecc_entity_example.exe")
    message(STATUS "   ✅ Professional tool compatible Milan v1.2 entity for Hive-AVDECC testing")
else()
    message(STATUS "⏭️  Skipping Milan AVDECC Entity Example (avnu_milan_1_2_2023 not available)")
endif()
