# Generic Network HAL CMakeLists.txt
# 
# Builds vendor-agnostic network hardware abstraction layer.
# Supports Intel I210/I219/I225/I226 via intel_avb integration.
# Graceful degradation when hardware support unavailable.
#
# Author: OpenAvnu Development Team
# Date: September 3, 2025
# Version: 1.0.0

cmake_minimum_required(VERSION 3.10)

# ============================================================================
# PROJECT SETUP
# ============================================================================

# Set project name and description
set(NETWORK_HAL_PROJECT_NAME "OpenAvnu_Network_HAL")
set(NETWORK_HAL_PROJECT_DESCRIPTION "Generic Network Hardware Abstraction Layer")

# Define library name
set(NETWORK_HAL_LIB_NAME "network_hal")

message(STATUS "Configuring ${NETWORK_HAL_PROJECT_DESCRIPTION}")

# ============================================================================
# FEATURE DETECTION
# ============================================================================

# Intel HAL support detection
set(INTEL_HAL_AVAILABLE FALSE)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../intel_avb/lib/intel.h")
    set(INTEL_HAL_AVAILABLE TRUE)
    message(STATUS "Intel HAL library detected: intel_avb available")
else()
    message(STATUS "Intel HAL library not found: building with stub support")
endif()

# Platform detection
if(WIN32)
    set(NETWORK_HAL_PLATFORM "Windows")
    set(PLATFORM_SOURCES 
        # Windows-specific sources would go here
    )
    set(PLATFORM_LIBS
        ws2_32
        iphlpapi
        setupapi
    )
elseif(UNIX AND NOT APPLE)
    set(NETWORK_HAL_PLATFORM "Linux")
    set(PLATFORM_SOURCES
        # Linux-specific sources would go here
    )
    set(PLATFORM_LIBS
        pthread
        # Additional Linux libs as needed
    )
else()
    set(NETWORK_HAL_PLATFORM "Unknown")
    set(PLATFORM_SOURCES)
    set(PLATFORM_LIBS)
    message(WARNING "Unsupported platform - Generic HAL may have limited functionality")
endif()

message(STATUS "Building for platform: ${NETWORK_HAL_PLATFORM}")

# ============================================================================
# SOURCE FILES
# ============================================================================

# Core Generic HAL sources (always included)
set(NETWORK_HAL_CORE_SOURCES
    network_hal.c
    network_hal.h
)

# gPTP HAL integration sources (always included)
set(GPTP_HAL_INTEGRATION_SOURCES
    gptp_hal_integration.c
    gptp_hal_integration.h
)

# Intel adapter sources (conditional)
set(INTEL_ADAPTER_SOURCES
    intel_adapter.c
)

# Combine sources based on availability
set(NETWORK_HAL_SOURCES
    ${NETWORK_HAL_CORE_SOURCES}
    ${GPTP_HAL_INTEGRATION_SOURCES}
)

# Add Intel adapter if available
if(INTEL_HAL_AVAILABLE)
    list(APPEND NETWORK_HAL_SOURCES ${INTEL_ADAPTER_SOURCES})
    message(STATUS "Including Intel adapter in build")
else()
    message(STATUS "Intel adapter excluded from build (intel_avb not available)")
endif()

message(STATUS "Including gPTP HAL integration in build")

# Add platform-specific sources
if(PLATFORM_SOURCES)
    list(APPEND NETWORK_HAL_SOURCES ${PLATFORM_SOURCES})
    message(STATUS "Including platform sources: ${PLATFORM_SOURCES}")
endif()

# ============================================================================
# LIBRARY DEFINITION
# ============================================================================

# Create static library (preferred for HAL)
add_library(${NETWORK_HAL_LIB_NAME} STATIC ${NETWORK_HAL_SOURCES})

# Set library properties
set_target_properties(${NETWORK_HAL_LIB_NAME} PROPERTIES
    VERSION 1.0.0
    SOVERSION 1
    OUTPUT_NAME ${NETWORK_HAL_LIB_NAME}
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

# ============================================================================
# INCLUDE DIRECTORIES
# ============================================================================

# Public interface (for consumers of the library)
target_include_directories(${NETWORK_HAL_LIB_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

# Private includes (for internal implementation)
target_include_directories(${NETWORK_HAL_LIB_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Intel HAL includes (when available)
if(INTEL_HAL_AVAILABLE)
    target_include_directories(${NETWORK_HAL_LIB_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../intel_avb/lib
        ${CMAKE_CURRENT_SOURCE_DIR}/../../intel_avb/include
    )
endif()

# ============================================================================
# COMPILER DEFINITIONS
# ============================================================================

# Core definitions (always applied)
target_compile_definitions(${NETWORK_HAL_LIB_NAME} PRIVATE
    NETWORK_HAL_VERSION_MAJOR=1
    NETWORK_HAL_VERSION_MINOR=0
    NETWORK_HAL_VERSION_PATCH=0
)

# Intel HAL support flag
if(INTEL_HAL_AVAILABLE)
    target_compile_definitions(${NETWORK_HAL_LIB_NAME} PRIVATE
        OPENAVNU_BUILD_INTEL_HAL=1
    )
    message(STATUS "Enabling Intel HAL compile-time support")
endif()

# Platform-specific definitions
if(WIN32)
    target_compile_definitions(${NETWORK_HAL_LIB_NAME} PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _WIN32_WINNT=0x0601  # Windows 7+
    )
elseif(UNIX)
    target_compile_definitions(${NETWORK_HAL_LIB_NAME} PRIVATE
        _GNU_SOURCE
        _DEFAULT_SOURCE
    )
endif()

# Debug/Release specific definitions
target_compile_definitions(${NETWORK_HAL_LIB_NAME} PRIVATE
    $<$<CONFIG:Debug>:NETWORK_HAL_DEBUG=1>
    $<$<CONFIG:Release>:NETWORK_HAL_RELEASE=1>
)

# ============================================================================
# COMPILER FLAGS
# ============================================================================

# Set C standard (C99 minimum for compatibility)
set_property(TARGET ${NETWORK_HAL_LIB_NAME} PROPERTY C_STANDARD 99)
set_property(TARGET ${NETWORK_HAL_LIB_NAME} PROPERTY C_STANDARD_REQUIRED ON)

# Warning flags (strict for quality)
if(MSVC)
    target_compile_options(${NETWORK_HAL_LIB_NAME} PRIVATE
        /W4         # High warning level
        /WX         # Warnings as errors (enforce clean code)
        /wd4996     # Disable deprecation warnings for standard functions
    )
else()
    target_compile_options(${NETWORK_HAL_LIB_NAME} PRIVATE
        -Wall       # Standard warnings
        -Wextra     # Extra warnings
        -Werror     # Warnings as errors
        -Wno-unused-parameter  # Allow unused parameters in function signatures
        -Wno-missing-field-initializers  # Allow partial struct initialization
    )
endif()

# Optimization flags
target_compile_options(${NETWORK_HAL_LIB_NAME} PRIVATE
    $<$<CONFIG:Debug>:-g -O0>
    $<$<CONFIG:Release>:-O3 -DNDEBUG>
    $<$<CONFIG:RelWithDebInfo>:-g -O2 -DNDEBUG>
)

# ============================================================================
# LINKED LIBRARIES
# ============================================================================

# Platform-specific libraries
if(PLATFORM_LIBS)
    target_link_libraries(${NETWORK_HAL_LIB_NAME} PRIVATE ${PLATFORM_LIBS})
    message(STATUS "Linking platform libraries: ${PLATFORM_LIBS}")
endif()

# Intel HAL library linkage (when available)
if(INTEL_HAL_AVAILABLE AND TARGET intel_avb)
    target_link_libraries(${NETWORK_HAL_LIB_NAME} PRIVATE intel_avb)
    message(STATUS "Linking Intel HAL library: intel_avb")
elseif(INTEL_HAL_AVAILABLE)
    # Static linkage fallback if target not available
    find_library(INTEL_AVB_LIB 
        NAMES intel_avb libintel_avb
        PATHS ${CMAKE_CURRENT_SOURCE_DIR}/../../intel_avb/lib
        NO_DEFAULT_PATH
    )
    
    if(INTEL_AVB_LIB)
        target_link_libraries(${NETWORK_HAL_LIB_NAME} PRIVATE ${INTEL_AVB_LIB})
        message(STATUS "Linking Intel HAL library (static): ${INTEL_AVB_LIB}")
    else()
        message(WARNING "Intel HAL library file not found - Intel features may not work")
    endif()
endif()

# ============================================================================
# INSTALLATION RULES
# ============================================================================

# Install library
install(TARGETS ${NETWORK_HAL_LIB_NAME}
    EXPORT OpenAvnuNetworkHALTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

# Install headers
install(FILES network_hal.h
    DESTINATION include/openavnu
    COMPONENT development
)

# Install CMake config files (for find_package support)
install(EXPORT OpenAvnuNetworkHALTargets
    FILE OpenAvnuNetworkHALTargets.cmake
    NAMESPACE OpenAvnu::
    DESTINATION lib/cmake/OpenAvnuNetworkHAL
)

# Create config file
include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/OpenAvnuNetworkHALConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/OpenAvnuNetworkHALConfig.cmake"
    INSTALL_DESTINATION lib/cmake/OpenAvnuNetworkHAL
)

# Create version file
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/OpenAvnuNetworkHALConfigVersion.cmake"
    VERSION 1.0.0
    COMPATIBILITY SameMajorVersion
)

# Install config and version files
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/OpenAvnuNetworkHALConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/OpenAvnuNetworkHALConfigVersion.cmake"
    DESTINATION lib/cmake/OpenAvnuNetworkHAL
)

# ============================================================================
# TESTING SUPPORT
# ============================================================================

# Build tests if enabled
option(NETWORK_HAL_BUILD_TESTS "Build Network HAL unit tests" OFF)

if(NETWORK_HAL_BUILD_TESTS)
    message(STATUS "Building Network HAL tests")
    
    # Find test framework (prefer CppUTest for OpenAvnu consistency)
    find_package(CppUTest QUIET)
    
    if(CppUTest_FOUND)
        add_subdirectory(tests)
    else()
        message(WARNING "CppUTest not found - tests will not be built")
    endif()
endif()

# ============================================================================
# DOCUMENTATION
# ============================================================================

# Build documentation if enabled
option(NETWORK_HAL_BUILD_DOCS "Build Network HAL documentation" OFF)

if(NETWORK_HAL_BUILD_DOCS)
    find_package(Doxygen QUIET)
    
    if(DOXYGEN_FOUND)
        # Configure Doxygen
        set(DOXYGEN_PROJECT_NAME "OpenAvnu Network HAL")
        set(DOXYGEN_PROJECT_BRIEF "Generic Network Hardware Abstraction Layer")
        set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/docs)
        set(DOXYGEN_INPUT ${CMAKE_CURRENT_SOURCE_DIR})
        set(DOXYGEN_RECURSIVE YES)
        set(DOXYGEN_EXTRACT_ALL YES)
        set(DOXYGEN_GENERATE_HTML YES)
        set(DOXYGEN_GENERATE_LATEX NO)
        
        doxygen_add_docs(network_hal_docs
            ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "Generating Network HAL documentation"
        )
        
        message(STATUS "Documentation target 'network_hal_docs' available")
    else()
        message(WARNING "Doxygen not found - documentation will not be built")
    endif()
endif()

# ============================================================================
# STATUS SUMMARY
# ============================================================================

message(STATUS "")
message(STATUS "=== Network HAL Configuration Summary ===")
message(STATUS "Library Name: ${NETWORK_HAL_LIB_NAME}")
message(STATUS "Platform: ${NETWORK_HAL_PLATFORM}")
message(STATUS "Intel HAL Available: ${INTEL_HAL_AVAILABLE}")
message(STATUS "Sources: ${NETWORK_HAL_SOURCES}")
if(PLATFORM_LIBS)
    message(STATUS "Platform Libraries: ${PLATFORM_LIBS}")
endif()
message(STATUS "Build Tests: ${NETWORK_HAL_BUILD_TESTS}")
message(STATUS "Build Documentation: ${NETWORK_HAL_BUILD_DOCS}")
message(STATUS "==========================================")
message(STATUS "")
