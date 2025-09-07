cmake_minimum_required (VERSION 3.10)
project (open-avb)

# Add OpenAvnu CMake modules to path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Set policy to handle older CMake versions in submodules
if(POLICY CMP0022)
    cmake_policy(SET CMP0022 NEW)
endif()
if(POLICY CMP0048)
    cmake_policy(SET CMP0048 NEW)
endif()

# Set minimum policy version to handle submodules with older CMake requirements
set(CMAKE_POLICY_DEFAULT_CMP0000 NEW)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

enable_testing()

include(CheckCXXCompilerFlag)

# Set C++11 standard for modern CMake
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.1")
    set(CMAKE_CXX_STANDARD 11)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
else()
    # Legacy C++11 detection for older CMake versions
    CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
    CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
    if(COMPILER_SUPPORTS_CXX11)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    elseif(COMPILER_SUPPORTS_CXX0X)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
    else()
        message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
    endif()
endif()

# ==========================================
# Centralized PCAP Detection for OpenAvnu
# ==========================================
# This provides unified PCAP detection that can be used by OpenAvnu components
# without requiring modifications to external submodules (L-Acoustics AVDECC, etc.)

message(STATUS "[DETECT] Detecting PCAP libraries for OpenAvnu components...")
include(FindPCAP)

# Export PCAP variables for subprojects that need them
set(OPENAVNU_PCAP_AVAILABLE ${PCAP_FOUND})
if(PCAP_FOUND)
    message(STATUS "[OK] PCAP detection complete - Available for OpenAvnu components")
    
    # Set global variables for subprojects
    set(OPENAVNU_PCAP_FOUND ${PCAP_FOUND})
    set(OPENAVNU_PCAP_TYPE ${PCAP_TYPE})
    set(OPENAVNU_PCAP_INCLUDE_DIRS ${PCAP_INCLUDE_DIRS})
    set(OPENAVNU_PCAP_LIBRARIES ${PCAP_LIBRARIES})
    if(WIN32)
        set(OPENAVNU_PCAP_LIBRARY_DIRS ${PCAP_LIBRARY_DIRS})
    endif()
    
    # Set environment variables for submodules that expect them
    if(WIN32 AND PCAP_TYPE STREQUAL "Npcap")
        set(ENV{NPCAP_DIR} "${PCAP_INCLUDE_DIRS}/..")
    elseif(WIN32 AND PCAP_TYPE STREQUAL "WinPcap")
        set(ENV{WPCAP_DIR} "${PCAP_INCLUDE_DIRS}/..")
    endif()
else()
    message(WARNING "[WARNING] PCAP not available - Some components may not build")
    set(OPENAVNU_PCAP_FOUND FALSE)
endif()

# Third-party libraries
add_subdirectory("thirdparty/cpputest")

# Intel Ethernet Hardware Abstraction Layer
option(OPENAVNU_BUILD_INTEL_HAL "Build Intel Ethernet HAL for I210/I219/I225/I226 support" ON)

if(OPENAVNU_BUILD_INTEL_HAL)
    message(STATUS "Building with Intel Ethernet HAL for hardware timestamping")
    add_subdirectory("thirdparty/intel-ethernet-hal")
    set(OPENAVNU_INTEL_HAL_AVAILABLE TRUE)
    
    # ==========================================
    # Intel HAL Detection and Configuration
    # ==========================================
    # Detect Intel HAL capabilities for feature-based compilation
    # Following repository rules: real hardware access, no simulations
    
    find_package(IntelHAL QUIET)
    if(IntelHAL_FOUND)
        message(STATUS "[OK] Intel HAL found: ${IntelHAL_VERSION}")
        message(STATUS "   📍 Include Directory: ${IntelHAL_INCLUDE_DIRS}")
        message(STATUS "   [LIB] Libraries: ${IntelHAL_LIBRARIES}")
        
        # Export Intel HAL availability to OpenAvnu components
        set(OPENAVNU_INTEL_HAL_FOUND TRUE)
        
        # Feature-based compilation flags based on actual hardware capabilities
        # Reference: Intel I210/I219/I225/I226 Controller Datasheets
        if(INTEL_HAL_HAS_TIMESTAMPING)
            message(STATUS "   [TIME] Hardware timestamping support detected")
            set(OPENAVNU_INTEL_TIMESTAMPING_AVAILABLE TRUE)
        endif()
        
        if(INTEL_HAL_HAS_TSN_TAS)
            message(STATUS "   🚦 TSN Time-Aware Shaper support detected (I225/I226)")
            set(OPENAVNU_INTEL_TSN_TAS_AVAILABLE TRUE)
        endif()
        
        if(INTEL_HAL_HAS_TSN_FP)
            message(STATUS "   ⚡ TSN Frame Preemption support detected (I225/I226)")
            set(OPENAVNU_INTEL_TSN_FP_AVAILABLE TRUE)
        endif()
        
        if(INTEL_HAL_HAS_LAUNCHTIME)
            message(STATUS "   [TARGET] LAUNCHTIME timed transmission support detected")
            set(OPENAVNU_INTEL_LAUNCHTIME_AVAILABLE TRUE)
        endif()
        
    else()
        message(WARNING "Intel HAL not found - advanced features unavailable")
        set(OPENAVNU_INTEL_HAL_FOUND FALSE)
        set(OPENAVNU_INTEL_TIMESTAMPING_AVAILABLE FALSE)
        set(OPENAVNU_INTEL_TSN_TAS_AVAILABLE FALSE)
        set(OPENAVNU_INTEL_TSN_FP_AVAILABLE FALSE)
        set(OPENAVNU_INTEL_LAUNCHTIME_AVAILABLE FALSE)
    endif()
else()
    set(OPENAVNU_INTEL_HAL_AVAILABLE FALSE)
    set(OPENAVNU_INTEL_HAL_FOUND FALSE)
endif()

# Optional gPTP daemon for time synchronization
option(OPENAVNU_BUILD_GPTP "Build gPTP daemon for time synchronization" ON)

if(OPENAVNU_BUILD_GPTP)
    message(STATUS "Building with gPTP daemon support")
    add_subdirectory("thirdparty/gptp")
    
    # ==========================================
    # gPTP Intel HAL Integration
    # ==========================================
    # Configure gPTP to use Intel HAL for hardware timestamping when available
    # Following repository rules: real hardware access, no simulation
    
    if(OPENAVNU_INTEL_HAL_FOUND AND TARGET gptp2)
        message(STATUS "🔗 Configuring gPTP with Intel HAL integration")
        
        # Add Intel HAL compile definitions to gPTP
        target_compile_definitions(gptp2 PRIVATE OPENAVNU_INTEL_HAL=1)
        
        # Link Intel HAL library to gPTP
        target_link_libraries(gptp2 PRIVATE IntelHAL::IntelHAL)
        
        # Feature-based compilation flags for gPTP
        if(OPENAVNU_INTEL_TIMESTAMPING_AVAILABLE)
            target_compile_definitions(gptp2 PRIVATE INTEL_HAL_TIMESTAMPING=1)
            message(STATUS "   [TIME] gPTP will use Intel hardware timestamping")
        endif()
        
        if(OPENAVNU_INTEL_TSN_TAS_AVAILABLE)
            target_compile_definitions(gptp2 PRIVATE INTEL_HAL_TSN_TAS=1)
            message(STATUS "   🚦 gPTP TSN Time-Aware Shaper support enabled")
        endif()
        
    else()
        if(OPENAVNU_BUILD_INTEL_HAL)
            message(STATUS "[WARNING] gPTP will use software timestamping (Intel HAL not found)")
        else()
            message(STATUS "📝 gPTP using software timestamping (Intel HAL disabled)")
        endif()
    endif()
endif()

# Optional L-Acoustics AVDECC library for modern AVDECC support
option(OPENAVNU_BUILD_LA_AVDECC "Build with L-Acoustics AVDECC library for IEEE 1722.1-2021 compliance" ON)

if(OPENAVNU_BUILD_LA_AVDECC)
    message(STATUS "Building with L-Acoustics AVDECC library for modern AVDECC support")
    
    # Pre-configure PCAP compatibility for L-Acoustics AVDECC before including the submodule
    if(WIN32 AND OPENAVNU_PCAP_FOUND)
        message(STATUS "[CONFIG] Configuring L-Acoustics AVDECC with modern PCAP support")
        message(STATUS "[OK] Using ${OPENAVNU_PCAP_TYPE} for L-Acoustics AVDECC")
        
        # L-Acoustics AVDECC expects winpcap in ${CU_ROOT_DIR}/externals/3rdparty/winpcap
        # We need to create the compatibility layer in the submodule directory
        set(LA_AVDECC_WINPCAP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib/la_avdecc/externals/3rdparty/winpcap")
        
        # Create compatibility symlinks/copies if they don't exist
        if(NOT EXISTS "${LA_AVDECC_WINPCAP_DIR}/Include")
            message(STATUS "Creating PCAP compatibility layer for L-Acoustics AVDECC...")
            
            # Create directories
            file(MAKE_DIRECTORY "${LA_AVDECC_WINPCAP_DIR}")
            file(MAKE_DIRECTORY "${LA_AVDECC_WINPCAP_DIR}/Include")
            file(MAKE_DIRECTORY "${LA_AVDECC_WINPCAP_DIR}/Lib")
            file(MAKE_DIRECTORY "${LA_AVDECC_WINPCAP_DIR}/Lib/x64")
            
            # Copy/link include files
            if(EXISTS "${OPENAVNU_PCAP_INCLUDE_DIRS}")
                file(GLOB PCAP_HEADERS "${OPENAVNU_PCAP_INCLUDE_DIRS}/*.h")
                foreach(HEADER ${PCAP_HEADERS})
                    get_filename_component(HEADER_NAME ${HEADER} NAME)
                    file(COPY ${HEADER} DESTINATION "${LA_AVDECC_WINPCAP_DIR}/Include")
                endforeach()
                
                # Copy pcap subdirectory with real headers (some headers expect pcap/pcap.h)
                if(EXISTS "${OPENAVNU_PCAP_INCLUDE_DIRS}/pcap")
                    file(COPY "${OPENAVNU_PCAP_INCLUDE_DIRS}/pcap" DESTINATION "${LA_AVDECC_WINPCAP_DIR}/Include")
                endif()
                
                message(STATUS "[OK] Copied PCAP headers to L-Acoustics AVDECC compatibility layer")
            endif()
            
            # Copy/link library files
            if(EXISTS "${OPENAVNU_PCAP_LIBRARY_DIRS}")
                file(GLOB PCAP_LIBS "${OPENAVNU_PCAP_LIBRARY_DIRS}/*.lib" "${OPENAVNU_PCAP_LIBRARY_DIRS}/*.a")
                foreach(LIB ${PCAP_LIBS})
                    get_filename_component(LIB_NAME ${LIB} NAME)
                    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
                        file(COPY ${LIB} DESTINATION "${LA_AVDECC_WINPCAP_DIR}/Lib/x64")
                    else()
                        file(COPY ${LIB} DESTINATION "${LA_AVDECC_WINPCAP_DIR}/Lib")
                    endif()
                endforeach()
                message(STATUS "[OK] Copied PCAP libraries to L-Acoustics AVDECC compatibility layer")
            endif()
            
            # Create README explaining the compatibility layer
            file(WRITE "${LA_AVDECC_WINPCAP_DIR}/README.md"
"# PCAP Compatibility Layer for L-Acoustics AVDECC

This directory contains a compatibility layer that provides PCAP libraries
to the L-Acoustics AVDECC submodule using OpenAvnu's centralized PCAP detection.

**Source:** ${OPENAVNU_PCAP_TYPE} (${OPENAVNU_PCAP_INCLUDE_DIRS})
**Generated:** ${CMAKE_CURRENT_LIST_FILE}
**Architecture:** ${CMAKE_SIZEOF_VOID_P}-bit

This allows L-Acoustics AVDECC to build with modern Npcap libraries
without requiring modifications to the external submodule.

Note: This directory is auto-generated during CMake configuration and should
be ignored by git. See .gitignore for the appropriate entry.
")
        endif()
        
        # Enable PCAP interface with compatibility layer
        set(BUILD_AVDECC_INTERFACE_PCAP ON CACHE BOOL "Enable PCAP interface with compatibility layer" FORCE)
        
        message(STATUS "[OK] L-Acoustics AVDECC PCAP compatibility layer ready")
        message(STATUS "   WinPcap compatibility path: ${LA_AVDECC_WINPCAP_DIR}")
    else()
        # Linux: PCAP should work directly, or disable if not available
        if(OPENAVNU_PCAP_FOUND)
            set(BUILD_AVDECC_INTERFACE_PCAP ON CACHE BOOL "Enable PCAP interface" FORCE)
        else()
            message(WARNING "[WARNING] PCAP not available - L-Acoustics AVDECC may not build correctly")
            message(STATUS "Attempting to build L-Acoustics AVDECC without PCAP interface")
            set(BUILD_AVDECC_INTERFACE_PCAP OFF CACHE BOOL "Disable PCAP interface - PCAP not available" FORCE)
        endif()
    endif()
    
    # Set other L-Acoustics AVDECC build options
    set(AVDECC_BUILD_CONTROLLER ON CACHE BOOL "Build AVDECC Controller")
    set(AVDECC_BUILD_VIRTUAL ON CACHE BOOL "Build AVDECC Virtual Entity") 
    set(AVDECC_BUILD_EXAMPLES OFF CACHE BOOL "Disable AVDECC examples")
    set(AVDECC_BUILD_TESTS OFF CACHE BOOL "Disable AVDECC tests for now")
    set(AVDECC_BUILD_DOCUMENTATION OFF CACHE BOOL "Disable AVDECC documentation")
    
    # Set C++17 requirement for L-Acoustics AVDECC
    if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.8")
        set(CMAKE_CXX_STANDARD 17)
        set(CMAKE_CXX_STANDARD_REQUIRED ON)
    endif()
    
    message(STATUS "[PKG] L-Acoustics AVDECC configuration:")
    message(STATUS "   PCAP Interface: ${BUILD_AVDECC_INTERFACE_PCAP}")
    message(STATUS "   Controller: ${AVDECC_BUILD_CONTROLLER}")
    message(STATUS "   Virtual Entity: ${AVDECC_BUILD_VIRTUAL}")
    
    # Include the actual submodule
    add_subdirectory("lib/la_avdecc")
    
    # Define flag for conditional compilation in OpenAvnu code
    add_definitions(-DOPENAVNU_HAS_LA_AVDECC)
endif()

# ==========================================
# Generic Network HAL (required for gPTP)
# ==========================================

# Always build the Generic Network HAL for gPTP integration
message(STATUS "[CONFIG] Building Generic Network HAL for gPTP integration")
add_subdirectory("lib/common/hal")

# Optional Milan v1.2 Protocol Implementation for Professional Audio AVB
option(OPENAVNU_BUILD_MILAN_V1_2 "Build Milan v1.2 protocol implementation for professional audio AVB" ON)

if(OPENAVNU_BUILD_MILAN_V1_2)
    message(STATUS "Building with Milan v1.2 protocol implementation")
    message(STATUS "🎵 Milan v1.2 provides professional audio AVB compatibility")
    
    # Include Milan v1.2 CMake configuration
    include(CMakeLists_Milan_v1_2.txt)
    
    # Define flag for conditional compilation in OpenAvnu code
    add_definitions(-DOPENAVNU_HAS_MILAN_V1_2)
    add_definitions(-DMILAN_VERSION_MAJOR=1)
    add_definitions(-DMILAN_VERSION_MINOR=2)
    add_definitions(-DMILAN_VERSION_PATCH=0)
    
    message(STATUS "[PKG] Milan v1.2 configuration:")
    message(STATUS "   Professional Audio AVB Device (PAAD) Entity: ENABLED")
    message(STATUS "   Milan MVU Commands: ENABLED")
    message(STATUS "   Media Clock Management: ENABLED")
    message(STATUS "   Seamless Network Redundancy: ENABLED")
    message(STATUS "   Professional Tool Compatibility: ENABLED")
    message(STATUS "   Compatible with: Hive-AVDECC, L-Acoustics Network Manager")
else()
    message(STATUS "[WARNING] Milan v1.2 protocol implementation disabled")
    message(STATUS "    Professional audio tool compatibility may be limited")
endif()

# Optional Open1722 AVTP library for modernized AVTP implementation
option(OPENAVNU_USE_OPEN1722 "Use Open1722 for AVTP implementation" ON)

if(OPENAVNU_USE_OPEN1722)
    message(STATUS "Building with Open1722 AVTP library support")
    
    # Set Open1722 include directory
    set(OPEN1722_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/open1722/include")
    
    # Create Open1722 library manually for Windows compatibility
    if(WIN32)
        # For Windows, create the Open1722 target manually
        file(GLOB_RECURSE OPEN1722_SOURCES 
            "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/open1722/src/*.c"
        )
        
        add_library(open1722 STATIC ${OPEN1722_SOURCES})
        target_include_directories(open1722 PUBLIC ${OPEN1722_INCLUDE_DIRS})
        
        # MSVC compatibility definitions
        if(MSVC)
            # Use forced include to inject our compatibility header
            target_compile_options(open1722 PRIVATE
                /FI"${CMAKE_CURRENT_SOURCE_DIR}/lib/avtp_pipeline/avtp_open1722/open1722_msvc_compat.h"
                /wd4200  # nonstandard extension used: zero-sized array in struct/union
                /wd4244  # conversion warnings
                /wd4996  # deprecated function warnings
                /Zp1     # force structure packing to 1 byte for compatibility
            )
            target_compile_definitions(open1722 PRIVATE _CRT_SECURE_NO_WARNINGS)
        else()
            target_compile_definitions(open1722 PRIVATE _CRT_SECURE_NO_WARNINGS)
        endif()
        
        set(OPEN1722_LIBRARIES open1722)
        message(STATUS "Created Open1722 static library for Windows with MSVC compatibility")
    else()
        # For Linux/other platforms, use the standard Open1722 build
        add_subdirectory("thirdparty/open1722")
        set(OPEN1722_LIBRARIES open1722)
    endif()
    
    add_definitions(-DOPENAVNU_USE_OPEN1722)
endif()

# ==========================================
# OpenAvnu Examples with Modern PCAP Support
# ==========================================
option(OPENAVNU_BUILD_EXAMPLES "Build OpenAvnu examples with modern PCAP support" ON)

if(OPENAVNU_BUILD_EXAMPLES)
    message(STATUS "Building OpenAvnu examples")
    add_subdirectory("examples")
else()
    message(STATUS "Skipping OpenAvnu examples")
endif()

# OpenAvnu components
add_subdirectory("daemons/common/tests")
add_subdirectory("daemons/mrpd")
add_subdirectory("daemons/maap")
add_subdirectory("daemons/shaper")
add_subdirectory("lib/avtp_pipeline/tests")

# Add IEEE Standards implementation library - Re-enabled after Phase 2B namespace fixes
add_subdirectory("lib/Standards")

# Add Open1722 wrapper if enabled
if(OPENAVNU_USE_OPEN1722)
    add_subdirectory("lib/avtp_pipeline/avtp_open1722")
endif()

# ==========================================
# AVDECC Entity Implementations
# ==========================================

# Production-ready AVDECC entities with clean organization
option(OPENAVNU_BUILD_AVDECC_ENTITIES "Build organized AVDECC entity implementations" ON)

if(OPENAVNU_BUILD_AVDECC_ENTITIES)
    message(STATUS "[TARGET] Building AVDECC entities with organized structure")
    add_subdirectory("lib/avdecc-entities")
    message(STATUS "[OK] AVDECC entities configured:")
    message(STATUS "   Production: intel_pcap_responsive (PRIMARY)")
    message(STATUS "   Production: hive_compatible (BACKUP)")
    message(STATUS "   Production: complete_standards (INTEGRATION)")
else()
    message(STATUS "[SKIP] AVDECC entities disabled")
endif()

# ==========================================
# Testing Framework Configuration
# ==========================================

# Unified Testing Framework (Recommended)
option(OPENAVNU_BUILD_UNIFIED_TESTING "Build unified testing framework with organized directory structure" ON)

# Legacy Testing Support (Deprecated - will be removed in future versions)
option(OPENAVNU_BUILD_LEGACY_TESTS "Build legacy individual test directories (deprecated)" OFF)

if(OPENAVNU_BUILD_UNIFIED_TESTING)
    message(STATUS "🧪 Building with unified testing framework")
    add_subdirectory("testing")
    
    # Disable legacy tests to avoid conflicts
    set(OPENAVNU_BUILD_LEGACY_TESTS OFF CACHE BOOL "Forced OFF when unified testing is enabled" FORCE)
    
elseif(OPENAVNU_BUILD_LEGACY_TESTS)
    message(STATUS "[WARNING] Building with legacy testing structure (deprecated)")
    message(STATUS "🔄 Consider migrating to unified testing framework (OPENAVNU_BUILD_UNIFIED_TESTING=ON)")
    
    # Legacy individual test options
    option(OPENAVNU_BUILD_TEST_MRPD "Build MRPD test" ON)
    option(OPENAVNU_BUILD_TEST_MAAP "Build MAAP test" ON)
    option(OPENAVNU_BUILD_TEST_AVDECC "Build AVDECC test" ON)
    option(OPENAVNU_BUILD_TEST_PCAP "Build PCAP test" ON)
    option(OPENAVNU_BUILD_TEST_GPTP "Build gPTP test" ON)
    option(OPENAVNU_BUILD_TEST_INTEL_HAL "Build Intel HAL test" ON)
    option(OPENAVNU_BUILD_TEST_CLOCK_QUALITY "Build Clock Quality test" ON)

    # MRPD Test
    if(OPENAVNU_BUILD_TEST_MRPD)
        message(STATUS "✅ Adding legacy MRPD test")
        add_subdirectory("daemons/mrpd/tests")
    endif()

    # MAAP Test
    if(OPENAVNU_BUILD_TEST_MAAP)
        message(STATUS "✅ Adding legacy MAAP test")
        add_subdirectory("daemons/maap/tests")
    endif()

    # AVDECC Test
    if(OPENAVNU_BUILD_TEST_AVDECC)
        message(STATUS "✅ Adding legacy AVDECC test")
        add_subdirectory("lib/la_avdecc/tests")
    endif()

    # PCAP Test
    if(OPENAVNU_BUILD_TEST_PCAP)
        message(STATUS "✅ Adding legacy PCAP test")
        add_subdirectory("lib/pcap/tests")
    endif()

    # gPTP Test
    if(OPENAVNU_BUILD_TEST_GPTP)
        message(STATUS "✅ Adding legacy gPTP test")
        add_subdirectory("thirdparty/gptp/tests")
    endif()

    # Intel HAL Test
    if(OPENAVNU_BUILD_TEST_INTEL_HAL)
        message(STATUS "✅ Adding legacy Intel HAL test")
        add_subdirectory("tests/intel_hal")
    endif()

    # TSN Test Suite (always available when Intel HAL is present)
    message(STATUS "🔧 DEBUG: Checking TSN test conditions...")
    message(STATUS "🔧 DEBUG: INTEL_HAL_FOUND = ${INTEL_HAL_FOUND}")
    if(TARGET intel-ethernet-hal-static)
        message(STATUS "🔧 DEBUG: intel-ethernet-hal-static target EXISTS")
    else()
        message(STATUS "🔧 DEBUG: intel-ethernet-hal-static target NOT FOUND")
    endif()
    
    if(INTEL_HAL_FOUND OR TARGET intel-ethernet-hal-static)
        message(STATUS "✅ Adding TSN test suite")
        add_subdirectory("tests")
    else()
        message(STATUS "❌ TSN test suite skipped - Intel HAL not available")
    endif()

    # Clock Quality Test
    if(OPENAVNU_BUILD_TEST_CLOCK_QUALITY)
        message(STATUS "✅ Adding legacy Clock Quality test")
        add_subdirectory("test/clock_quality")
    endif()
    
else()
    message(STATUS "⏭️  All testing disabled (OPENAVNU_BUILD_UNIFIED_TESTING=OFF, OPENAVNU_BUILD_LEGACY_TESTS=OFF)")
endif()

message("
-------------------------------------------------------
OpenAvnu Build information

Current compiler options:
    CC:                                 ${CMAKE_C_COMPILER}
    CXX:                                ${CMAKE_CXX_COMPILER}
    OpenAvnu CFLAGS:                    ${CMAKE_C_FLAGS}
    OpenAvnu CXXFLAGS:                  ${CMAKE_CXX_FLAGS}
    OpenAvnu LDFLAGS:                   ${CMAKE_LD_FLAGS}

Features configured in OpenAvnu root CMakeFile.txt:
    Memory Leak Detection:              ${MEMORY_LEAK_DETECTION}
    Compiling Extensions:               ${EXTENSIONS}
    Support Long Long:                  ${LONGLONG}
    Use OpenAvnu flags:                 ${CMAKE_FLAGS}

    Using Standard C library:           ${STD_C}
    Using Standard C++ library:         ${STD_CPP}
    Using C++11 library:                ${C++11}

-------------------------------------------------------
")

# ==========================================
# Phase 2B Header Rationalization Test
# ==========================================
# Test to validate that the rationalized IEEE 1722.1-2021 headers work correctly
add_executable(test_phase2b_working
    test_phase2b_working.cpp
)

# Use the minimal core library that we know compiles
target_link_libraries(test_phase2b_working PRIVATE
    ieee_1722_1_2021_core
)

# Include directories for the test
target_include_directories(test_phase2b_working PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/Standards/IEEE/1722.1/2021"
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/Standards"
)

# Set C++11 standard for the test
set_target_properties(test_phase2b_working PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
)

# TSN Integration Test (Phase 3)
if(TARGET intel-ethernet-hal-static)
    add_executable(test_intel_tsn_integration
        lib/avtp_pipeline/test_intel_tsn_integration.c
        lib/avtp_pipeline/intel_tsn_integration.c
    )

    target_link_libraries(test_intel_tsn_integration
        PRIVATE
        intel-ethernet-hal-static
        ws2_32
    )

    target_include_directories(test_intel_tsn_integration
        PRIVATE
        ${CMAKE_SOURCE_DIR}/thirdparty/intel-ethernet-hal/include
        ${CMAKE_SOURCE_DIR}/lib/intel_avb/lib
        ${CMAKE_SOURCE_DIR}/lib/intel_avb/include
    )

    set_target_properties(test_intel_tsn_integration PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
    )

    add_dependencies(test_intel_tsn_integration intel-ethernet-hal-static)
    
    # Real TSN Feature Test (tests actual TSN functions)
    add_executable(real_tsn_test
        lib/avtp_pipeline/real_tsn_test.c
    )
    
    target_link_libraries(real_tsn_test 
        PRIVATE 
        intel-ethernet-hal-static
    )
    
    target_include_directories(real_tsn_test 
        PRIVATE 
        ${CMAKE_SOURCE_DIR}/thirdparty/intel-ethernet-hal/include
    )

    set_target_properties(real_tsn_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
    )

    add_dependencies(real_tsn_test intel-ethernet-hal-static)
    
    # Simple TSN Feature Test (Windows-compatible, no network conflicts)
    add_executable(simple_tsn_test
        lib/avtp_pipeline/simple_tsn_test.c
    )
    
    target_link_libraries(simple_tsn_test 
        PRIVATE 
        intel-ethernet-hal-static
    )
    
    target_include_directories(simple_tsn_test 
        PRIVATE 
        ${CMAKE_SOURCE_DIR}/thirdparty/intel-ethernet-hal/include
    )

    set_target_properties(simple_tsn_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
    )

    add_dependencies(simple_tsn_test intel-ethernet-hal-static)
    
    # Minimal TSN test (no includes, direct validation)
    add_executable(minimal_tsn_test
        lib/avtp_pipeline/minimal_tsn_test.c
    )
    
    target_link_libraries(minimal_tsn_test 
        PRIVATE 
        intel-ethernet-hal-static
    )

    set_target_properties(minimal_tsn_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
    )

    add_dependencies(minimal_tsn_test intel-ethernet-hal-static)
    
    # Corrected TSN test (uses actual Intel HAL API)
    add_executable(corrected_tsn_test
        lib/avtp_pipeline/corrected_tsn_test.c
    )
    
    target_link_libraries(corrected_tsn_test 
        PRIVATE 
        intel-ethernet-hal-static
    )

    set_target_properties(corrected_tsn_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
    )

    add_dependencies(corrected_tsn_test intel-ethernet-hal-static)
    
    message(STATUS "TSN Integration Test target added")
    message(STATUS "Real TSN Feature Test target added")
endif()

# Add TSN Fixed Test - Test our fixed TSN functions
add_executable(test_tsn_fixed
    test_tsn_fixed.c
    # Include intel_avb source files needed for TSN functions
    lib/intel_avb/lib/intel.c
    lib/intel_avb/lib/intel_i225.c
    lib/intel_avb/lib/intel_i210.c
    lib/intel_avb/lib/intel_i219.c
    lib/intel_avb/lib/intel_i217.c
    lib/intel_avb/lib/intel_common.c
    lib/intel_avb/lib/intel_windows.c
)

target_include_directories(test_tsn_fixed PRIVATE
    lib/intel_avb/lib
)

target_link_libraries(test_tsn_fixed
    intel-ethernet-hal-static
)

set_target_properties(test_tsn_fixed PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
)

# Only add Intel HAL dependency if Intel HAL is enabled
if(OPENAVNU_BUILD_INTEL_HAL AND TARGET intel-ethernet-hal-static)
    add_dependencies(test_tsn_fixed intel-ethernet-hal-static)
endif()

message(STATUS "✅ TSN Fixed Test target added - tests our fixed TSN functions")

# Test executable for Milan Hardware Integration
add_executable(test_milan_hardware_integration test_milan_hardware_integration.cpp)

# Handle Windows-specific linking
if(WIN32)
    target_link_libraries(test_milan_hardware_integration ws2_32 iphlpapi)
endif()

# Enable building if requested
option(BUILD_MILAN_HARDWARE_TEST "Build Milan Hardware Integration Test" ON)

if(BUILD_MILAN_HARDWARE_TEST)
    message(STATUS "✅ Milan Hardware Integration Test target added - validates hardware integration")
endif()
