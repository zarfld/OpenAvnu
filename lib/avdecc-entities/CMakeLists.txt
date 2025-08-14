# AVDECC Entities - Production Build System
# Clean, organized build for top-tier AVDECC implementations

cmake_minimum_required(VERSION 3.16)

# Build options for AVDECC entities
option(BUILD_AVDECC_PRODUCTION "Build production AVDECC entities" ON)
option(BUILD_AVDECC_SPECIALIZED "Build specialized AVDECC entities" OFF)
option(BUILD_AVDECC_EXPERIMENTAL "Build experimental AVDECC entities" OFF)

# Find required dependencies
find_package(PkgConfig QUIET)

# Check for PCAP support (required for most implementations)
if(WIN32)
    # Windows: Look for Npcap or WinPCAP
    find_path(PCAP_INCLUDE_DIR pcap.h
        HINTS 
            $ENV{NPCAP_DIR}/Include
            $ENV{NPCAP_SDK_DIR}/Include  
            $ENV{WPCAP_DIR}/Include
        PATHS
            "C:/npcap-sdk/Include"
            "C:/WpdPack/Include"
    )
    
    find_library(PCAP_LIBRARY
        NAMES wpcap packet
        HINTS
            $ENV{NPCAP_DIR}/Lib/x64
            $ENV{NPCAP_SDK_DIR}/Lib/x64
            $ENV{WPCAP_DIR}/Lib/x64  
        PATHS
            "C:/npcap-sdk/Lib/x64"
            "C:/WpdPack/Lib/x64"
    )
else()
    # Linux/Unix: Standard libpcap
    find_path(PCAP_INCLUDE_DIR pcap.h)
    find_library(PCAP_LIBRARY NAMES pcap)
endif()

if(PCAP_INCLUDE_DIR AND PCAP_LIBRARY)
    set(PCAP_FOUND TRUE)
    message(STATUS "PCAP found: ${PCAP_LIBRARY}")
else()
    set(PCAP_FOUND FALSE)
    message(WARNING "PCAP not found - AVDECC entities will not be built")
endif()

# Production AVDECC Entities (Tier 1)
if(BUILD_AVDECC_PRODUCTION AND PCAP_FOUND)
    
    # Primary: Intel PCAP Responsive Entity ⭐⭐⭐⭐⭐
    add_executable(avdecc_entity_responsive
        production/responsive/intel_pcap_responsive.cpp
    )
    target_include_directories(avdecc_entity_responsive PRIVATE ${PCAP_INCLUDE_DIR})
    target_link_libraries(avdecc_entity_responsive PRIVATE ${PCAP_LIBRARY})
    
    if(WIN32)
        target_link_libraries(avdecc_entity_responsive PRIVATE ws2_32 iphlpapi)
    endif()
    
    # Backup: Hive Compatible Entity ⭐⭐⭐⭐
    add_executable(avdecc_entity_hive_compatible
        production/responsive/hive_compatible.c
    )
    target_include_directories(avdecc_entity_hive_compatible PRIVATE ${PCAP_INCLUDE_DIR})
    target_link_libraries(avdecc_entity_hive_compatible PRIVATE ${PCAP_LIBRARY})
    
    if(WIN32)
        target_link_libraries(avdecc_entity_hive_compatible PRIVATE ws2_32 iphlpapi)
    endif()
    
    # Complete Standards Integration ⭐⭐⭐⭐
    add_executable(avdecc_entity_complete_standards
        production/responsive/complete_standards.cpp
        ../Standards/IEEE/1722/2016/streaming/ieee_1722_2016_streaming.cpp
    )
    target_include_directories(avdecc_entity_complete_standards PRIVATE 
        ${PCAP_INCLUDE_DIR}
        ../Standards
        ../common
    )
    target_link_libraries(avdecc_entity_complete_standards PRIVATE ${PCAP_LIBRARY})
    
    # Add Windows-specific definitions
    target_compile_definitions(avdecc_entity_complete_standards PRIVATE 
        USING_NPCAP
        WIN32_LEAN_AND_MEAN
        NOMINMAX
    )
    
    message(STATUS "✅ complete_standards: Configured with IEEE 1722-2016 implementation")
    
    if(WIN32)
        target_link_libraries(avdecc_entity_complete_standards PRIVATE ws2_32 iphlpapi)
    endif()
    
    message(STATUS "Production AVDECC entities configured:")
    message(STATUS "  - avdecc_entity_responsive (PRIMARY)")
    message(STATUS "  - avdecc_entity_hive_compatible (BACKUP)")
    message(STATUS "  - avdecc_entity_complete_standards (INTEGRATION)")
endif()

# Specialized AVDECC Entities (Tier 2) 
if(BUILD_AVDECC_SPECIALIZED AND PCAP_FOUND)
    
    # Intel AVB Hardware Entity (requires Intel driver)
    add_executable(avdecc_entity_intel_avb
        production/specialized/intel_avb_hardware.cpp
    )
    target_include_directories(avdecc_entity_intel_avb PRIVATE ${PCAP_INCLUDE_DIR})
    target_link_libraries(avdecc_entity_intel_avb PRIVATE ${PCAP_LIBRARY})
    
    if(WIN32)
        target_link_libraries(avdecc_entity_intel_avb PRIVATE ws2_32 iphlpapi)
    endif()
    
    # Raw Ethernet Entity (low-level sockets)
    add_executable(avdecc_entity_raw_ethernet
        production/specialized/raw_ethernet.c
    )
    
    if(WIN32)
        target_link_libraries(avdecc_entity_raw_ethernet PRIVATE ws2_32 iphlpapi)
    endif()
    
    message(STATUS "Specialized AVDECC entities configured:")
    message(STATUS "  - avdecc_entity_intel_avb (Intel hardware)")
    message(STATUS "  - avdecc_entity_raw_ethernet (Raw sockets)")
endif()

# Experimental AVDECC Entities (Development)
if(BUILD_AVDECC_EXPERIMENTAL AND PCAP_FOUND)
    
    # Hive test implementations
    add_executable(avdecc_hive_test
        development/experimental/hive_avdecc_entity_test.cpp
    )
    target_include_directories(avdecc_hive_test PRIVATE ${PCAP_INCLUDE_DIR})
    target_link_libraries(avdecc_hive_test PRIVATE ${PCAP_LIBRARY})
    
    if(WIN32)
        target_link_libraries(avdecc_hive_test PRIVATE ws2_32 iphlpapi)
    endif()
    
    message(STATUS "Experimental AVDECC entities configured for development")
endif()

# Installation
if(BUILD_AVDECC_PRODUCTION)
    install(TARGETS avdecc_entity_responsive
        RUNTIME DESTINATION bin
        COMPONENT AVDECCEntities
    )
    
    if(TARGET avdecc_entity_hive_compatible)
        install(TARGETS avdecc_entity_hive_compatible
            RUNTIME DESTINATION bin
            COMPONENT AVDECCEntities
        )
    endif()
endif()

# Documentation installation
install(FILES docs/DEPLOYMENT_GUIDE.md
    DESTINATION docs/avdecc-entities
    COMPONENT Documentation
)
