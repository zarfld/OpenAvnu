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
    
    # Test executable to verify AVDECC constants resolve correctly
    add_executable(test_avdecc_constants test_avdecc_constants.c)
    target_include_directories(test_avdecc_constants PRIVATE ../avdecc-lib/jdksavdecc-c/include)
    
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
    
    # Build the REAL Hive Compatible AVDECC Entity executable using main project's PCAP detection
    if(PCAP_FOUND)
        message(STATUS "✅ Using main project's PCAP detection - building hive_compatible_avdecc_entity.exe")
        message(STATUS "   PCAP Type: ${PCAP_TYPE}")
        message(STATUS "   PCAP Include: ${PCAP_INCLUDE_DIRS}")
        message(STATUS "   PCAP Libraries: ${PCAP_LIBRARIES}")
        
        add_executable(hive_compatible_avdecc_entity hive_compatible_avdecc_entity.c)
        target_include_directories(hive_compatible_avdecc_entity PRIVATE 
            ../avdecc-lib/jdksavdecc-c/include
            ${PCAP_INCLUDE_DIRS}
        )
        target_link_libraries(hive_compatible_avdecc_entity ${PCAP_LIBRARIES})
        
        if(WIN32)
            target_link_libraries(hive_compatible_avdecc_entity ws2_32 iphlpapi)
            if(PCAP_LIBRARY_DIRS)
                target_link_directories(hive_compatible_avdecc_entity PRIVATE ${PCAP_LIBRARY_DIRS})
            endif()
        endif()
        
        message(STATUS "✅ Real AVDECC entity will be built at: hive_compatible_avdecc_entity.exe")
    else()
        message(STATUS "⚠️  PCAP not found by main project - cannot build real hive_compatible_avdecc_entity.exe")
        message(STATUS "   Install Npcap SDK or WinPcap Developer Pack and reconfigure main project")
    endif()
    
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

# Intel PCAP AVDECC Entity (Your working implementation) 
# Build the improved responsive entity that we just fixed
if(PCAP_FOUND)
    message(STATUS "✅ Building your improved Intel PCAP AVDECC Entity")
    add_executable(intel_pcap_avdecc_entity_responsive
        ../lib/Standards/intel_pcap_avdecc_entity_responsive.cpp
    )
    
    target_include_directories(intel_pcap_avdecc_entity_responsive PRIVATE 
        ${PCAP_INCLUDE_DIRS}
        ../lib/Standards
    )
    
    # Use the proper PCAP libraries from the main project's detection
    target_link_libraries(intel_pcap_avdecc_entity_responsive PRIVATE 
        ${PCAP_LIBRARIES}
    )
    
    # Add explicit library directory for linker
    if(WIN32 AND PCAP_LIBRARY_DIRS)
        target_link_directories(intel_pcap_avdecc_entity_responsive PRIVATE 
            ${PCAP_LIBRARY_DIRS}
        )
    endif()
    
    if(WIN32)
        target_compile_definitions(intel_pcap_avdecc_entity_responsive PRIVATE _CRT_SECURE_NO_WARNINGS)
    endif()
    
    message(STATUS "   📍 Your working AVDECC entity: intel_pcap_avdecc_entity_responsive.exe")
    message(STATUS "   🔗 Using PCAP: ${PCAP_TYPE} (${PCAP_LIBRARIES})")
endif()

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

if(PCAP_FOUND AND OPENAVNU_BUILD_INTEL_HAL)
    message(STATUS "🚀 Building Complete AVB Entity with Intel Filter Driver")
    
    add_executable(complete_avb_entity_with_intel_driver
        complete_avb_entity_with_intel_driver.cpp
        ../lib/Standards/intel_pcap_avdecc_entity_responsive.cpp
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
else()
    message(STATUS "⏭️  Skipping Complete AVB Entity (requires PCAP + Intel HAL)")
endif()
