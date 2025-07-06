cmake_minimum_required(VERSION 3.10)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

project ( AVB )

# Select the OS abstraction layer implementation
if (WIN32 AND NOT DEFINED OPENAVB_OSAL)
  set(OPENAVB_OSAL Windows)
elseif(NOT DEFINED OPENAVB_OSAL)
  set(OPENAVB_OSAL Linux)
endif()

# point to AVB SRC directory
set ( AVB_SRC_DIR ${CMAKE_SOURCE_DIR} )

# point to HAL directory
set ( AVB_HAL_DIR ${AVB_SRC_DIR}/platform/${OPENAVB_HAL} )

# point to OSAL directory
set ( AVB_OSAL_DIR ${AVB_SRC_DIR}/platform/${OPENAVB_OSAL} )

# point to TCAL directory
set ( AVB_TCAL_DIR ${AVB_SRC_DIR}/platform/platTCAL/${OPENAVB_TCAL} )

# Directory to install binaries to
set ( AVB_INSTALL_DIR ${CMAKE_BINARY_DIR}/bin )

# CORE_TODO: There may be additional common CMakeLists.txt functionality that can be migrated out of the platform specific area.

# CMake is happier when the CMakeLists.txt is in a top level directory working down. Therefore the platform specific CMakeLists.txt
# is included here. The common CMake command uage will take this form below when run from a build directory that is at the same 
# directory level as the repo:
# cmake -DCMAKE_TOOLCHAIN_FILE=../<avb_repo>/platform/<platform name>/<target>.cmake -DCMAKE_BUILD_TYPE=Release ../<avb_repo>
# for example:
# cmake -DCMAKE_TOOLCHAIN_FILE=../avbrepo/platform/Linux/x86_i210_linux.cmake -DCMAKE_BUILD_TYPE=Release ../avbrepo

# Used to hold lists of source files from common code subdirectories
SET (SRC_FILES "")

# Add profile framework subdirectory
add_subdirectory(profile)

# Optional L-Acoustics AVDECC integration for modern AVDECC/MILAN support
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../la_avdecc/CMakeLists.txt")
    option(OPENAVNU_ENABLE_LA_AVDECC "Enable L-Acoustics AVDECC integration" OFF)
    
    if(OPENAVNU_ENABLE_LA_AVDECC)
        message(STATUS "Enabling L-Acoustics AVDECC integration for MILAN support")
        
        # Check CMake version compatibility
        if(CMAKE_VERSION VERSION_LESS "3.29")
            message(WARNING "L-Acoustics AVDECC requires CMake 3.29+, current version is ${CMAKE_VERSION}")
            message(WARNING "L-Acoustics AVDECC integration will be disabled")
            set(OPENAVNU_ENABLE_LA_AVDECC OFF CACHE BOOL "" FORCE)
        else()
            # Configure L-Acoustics build options - build only what we need
            set(BUILD_AVDECC_EXAMPLES OFF CACHE BOOL "Build examples" FORCE)
            set(BUILD_AVDECC_TESTS OFF CACHE BOOL "Build unit tests" FORCE)
            set(BUILD_AVDECC_LIB_SHARED_CXX OFF CACHE BOOL "Build C++ shared library" FORCE)
            set(BUILD_AVDECC_LIB_STATIC_RT_SHARED ON CACHE BOOL "Build static library" FORCE)
            set(BUILD_AVDECC_DOC OFF CACHE BOOL "Build documentation" FORCE)
            set(BUILD_AVDECC_BINDINGS_C OFF CACHE BOOL "Build C bindings" FORCE)
            set(BUILD_AVDECC_BINDINGS_CSHARP OFF CACHE BOOL "Build C# bindings" FORCE)
            set(BUILD_AVDECC_CONTROLLER ON CACHE BOOL "Build controller library" FORCE)
            set(INSTALL_AVDECC_EXAMPLES OFF CACHE BOOL "Install examples" FORCE)
            set(INSTALL_AVDECC_TESTS OFF CACHE BOOL "Install tests" FORCE)
            set(INSTALL_AVDECC_LIB_SHARED_CXX OFF CACHE BOOL "Install shared library" FORCE)
            set(INSTALL_AVDECC_LIB_STATIC OFF CACHE BOOL "Install static library" FORCE)
            set(INSTALL_AVDECC_BINDINGS OFF CACHE BOOL "Install bindings" FORCE)
            set(INSTALL_AVDECC_HEADERS OFF CACHE BOOL "Install headers" FORCE)
            set(INSTALL_AVDECC_DOC OFF CACHE BOOL "Install documentation" FORCE)
            
            # Add L-Acoustics avdecc subdirectory
            add_subdirectory(../la_avdecc la_avdecc EXCLUDE_FROM_ALL)
            
            # Create a flag to indicate L-Acoustics AVDECC is available
            add_compile_definitions(OPENAVNU_HAS_LA_AVDECC=1)
            
            message(STATUS "L-Acoustics AVDECC v4.1.0 integration enabled")
            message(STATUS "Available targets: la_avdecc_static, la_avdecc_controller_static")
            message(STATUS "This provides: IEEE 1722.1-2021, MILAN compliance, Hive compatibility")
        endif()
    endif()
endif()

# Suppress the policy warning when including a CMakeLists.txt file
if(POLICY CMP0011)
  cmake_policy(SET CMP0011 OLD)
endif(POLICY CMP0011)

include(${AVB_OSAL_DIR}/CMakeLists.txt)



