# FindPCAP.cmake - Unified PCAP detection for OpenAvnu
# Supports Npcap (preferred), WinPcap (fallback), and libpcap (Linux/macOS)
# This module can be used by OpenAvnu components without modifying external submodules

# Set CMake policy for consistent behavior
cmake_policy(SET CMP0054 NEW)

if(WIN32)
  # Windows: Detect Npcap vs WinPcap vs NPCAP_SDK_DIR
  if(DEFINED ENV{NPCAP_SDK_DIR})
    # GitHub Actions / CI environment variable
    set(PCAP_ROOT_DIR $ENV{NPCAP_SDK_DIR})
    set(PCAP_LIBRARY_NAME "wpcap")
    set(PCAP_TYPE "Npcap")
    set(PCAP_SOURCE "NPCAP_SDK_DIR")
    add_definitions(-DUSING_NPCAP)
    message(STATUS "✅ Found Npcap SDK (CI): ${PCAP_ROOT_DIR}")
  elseif(DEFINED ENV{NPCAP_DIR})
    # Standard Npcap SDK installation
    set(PCAP_ROOT_DIR $ENV{NPCAP_DIR})
    set(PCAP_LIBRARY_NAME "wpcap")
    set(PCAP_TYPE "Npcap")
    set(PCAP_SOURCE "NPCAP_DIR")
    add_definitions(-DUSING_NPCAP)
    message(STATUS "✅ Found Npcap SDK: ${PCAP_ROOT_DIR}")
  elseif(DEFINED ENV{WPCAP_DIR})
    # Legacy WinPcap SDK (fallback for older systems)
    set(PCAP_ROOT_DIR $ENV{WPCAP_DIR})
    set(PCAP_LIBRARY_NAME "wpcap")
    set(PCAP_TYPE "WinPcap")
    set(PCAP_SOURCE "WPCAP_DIR")
    add_definitions(-DUSING_WINPCAP)
    message(STATUS "⚠️  Found legacy WinPcap SDK: ${PCAP_ROOT_DIR}")
    message(WARNING "WinPcap is deprecated. Consider upgrading to Npcap for better Windows 10/11 support.")
  else()
    # Try to find Npcap in standard installation locations
    set(NPCAP_STANDARD_PATHS
      "C:/npcap-sdk"
      "C:/Program Files/Npcap/SDK"
      "C:/Program Files (x86)/Npcap/SDK"
    )
    
    foreach(NPCAP_PATH ${NPCAP_STANDARD_PATHS})
      if(EXISTS "${NPCAP_PATH}/Include" AND EXISTS "${NPCAP_PATH}/Lib")
        set(PCAP_ROOT_DIR "${NPCAP_PATH}")
        set(PCAP_LIBRARY_NAME "wpcap")
        set(PCAP_TYPE "Npcap")
        set(PCAP_SOURCE "Standard Path")
        add_definitions(-DUSING_NPCAP)
        message(STATUS "✅ Found Npcap SDK at standard location: ${PCAP_ROOT_DIR}")
        break()
      endif()
    endforeach()
    
    if(NOT DEFINED PCAP_ROOT_DIR)
      message(FATAL_ERROR 
        "❌ No PCAP library found for Windows!\n"
        "Please install one of the following:\n"
        "  • Npcap SDK (recommended): https://npcap.com/dist/npcap-sdk-1.13.zip\n"
        "    Set NPCAP_DIR or NPCAP_SDK_DIR environment variable\n"
        "  • WinPcap Developer Pack (legacy): https://www.winpcap.org/install/bin/WpdPack_4_1_2.zip\n"
        "    Set WPCAP_DIR environment variable\n"
      )
    endif()
  endif()

  # Set up Windows include and library directories
  set(PCAP_INCLUDE_DIRS "${PCAP_ROOT_DIR}/Include")
  
  # Architecture-specific library paths
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(PCAP_LIBRARY_DIRS "${PCAP_ROOT_DIR}/Lib/x64")
  elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(PCAP_LIBRARY_DIRS "${PCAP_ROOT_DIR}/Lib")
  else()
    message(FATAL_ERROR "Unsupported architecture for PCAP on Windows")
  endif()
  
  # Verify library directory exists
  if(NOT EXISTS "${PCAP_LIBRARY_DIRS}")
    message(FATAL_ERROR "PCAP library directory not found: ${PCAP_LIBRARY_DIRS}")
  endif()
  
  set(PCAP_LIBRARIES ${PCAP_LIBRARY_NAME})
  
  # Additional Windows libraries typically needed with PCAP
  list(APPEND PCAP_LIBRARIES "Iphlpapi" "Ws2_32")
  
  # Common Windows definitions for PCAP
  add_definitions(-D_CRT_SECURE_NO_WARNINGS -DHAVE_REMOTE)
  
else()
  # Linux/macOS: Use pkg-config or find_library
  find_package(PkgConfig QUIET)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PCAP libpcap)
    if(PCAP_FOUND)
      set(PCAP_TYPE "libpcap")
      set(PCAP_SOURCE "pkg-config")
      message(STATUS "✅ Found libpcap via pkg-config: ${PCAP_LIBRARIES}")
    endif()
  endif()
  
  # Fallback to manual detection
  if(NOT PCAP_FOUND)
    find_library(PCAP_LIBRARIES 
      NAMES pcap
      PATHS /usr/lib /usr/local/lib /opt/local/lib
    )
    find_path(PCAP_INCLUDE_DIRS 
      NAMES pcap.h
      PATHS /usr/include /usr/local/include /opt/local/include
    )
    
    if(PCAP_LIBRARIES AND PCAP_INCLUDE_DIRS)
      set(PCAP_FOUND TRUE)
      set(PCAP_TYPE "libpcap")
      set(PCAP_SOURCE "find_library")
      message(STATUS "✅ Found libpcap: ${PCAP_LIBRARIES}")
    endif()
  endif()
  
  if(NOT PCAP_FOUND)
    message(FATAL_ERROR 
      "❌ libpcap not found!\n"
      "Please install libpcap development package:\n"
      "  • Ubuntu/Debian: sudo apt-get install libpcap-dev\n"
      "  • CentOS/RHEL: sudo yum install libpcap-devel\n"
      "  • macOS: brew install libpcap\n"
    )
  endif()
endif()

# Final validation and summary
if(PCAP_LIBRARIES AND PCAP_INCLUDE_DIRS)
  set(PCAP_FOUND TRUE)
  message(STATUS "📦 PCAP Summary:")
  message(STATUS "   Type: ${PCAP_TYPE}")
  message(STATUS "   Source: ${PCAP_SOURCE}")
  message(STATUS "   Include: ${PCAP_INCLUDE_DIRS}")
  message(STATUS "   Libraries: ${PCAP_LIBRARIES}")
  if(WIN32)
    message(STATUS "   Library Dir: ${PCAP_LIBRARY_DIRS}")
  endif()
else()
  set(PCAP_FOUND FALSE)
  message(FATAL_ERROR "❌ PCAP configuration failed")
endif()

# Export variables for parent project
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  # We are at the top level, set global variables
  set(OPENAVNU_PCAP_FOUND ${PCAP_FOUND})
  set(OPENAVNU_PCAP_TYPE ${PCAP_TYPE})
  set(OPENAVNU_PCAP_INCLUDE_DIRS ${PCAP_INCLUDE_DIRS})
  set(OPENAVNU_PCAP_LIBRARIES ${PCAP_LIBRARIES})
  if(WIN32)
    set(OPENAVNU_PCAP_LIBRARY_DIRS ${PCAP_LIBRARY_DIRS})
  endif()
else()
  # We are included by another project, use PARENT_SCOPE
  set(OPENAVNU_PCAP_FOUND ${PCAP_FOUND} PARENT_SCOPE)
  set(OPENAVNU_PCAP_TYPE ${PCAP_TYPE} PARENT_SCOPE)
  set(OPENAVNU_PCAP_INCLUDE_DIRS ${PCAP_INCLUDE_DIRS} PARENT_SCOPE)
  set(OPENAVNU_PCAP_LIBRARIES ${PCAP_LIBRARIES} PARENT_SCOPE)
  if(WIN32)
    set(OPENAVNU_PCAP_LIBRARY_DIRS ${PCAP_LIBRARY_DIRS} PARENT_SCOPE)
  endif()
endif()

# Create PCAP target for easy linking
if(NOT TARGET OpenAvnu::PCAP)
  add_library(OpenAvnu::PCAP INTERFACE IMPORTED)
  set_target_properties(OpenAvnu::PCAP PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${PCAP_INCLUDE_DIRS}"
    INTERFACE_LINK_LIBRARIES "${PCAP_LIBRARIES}"
  )
  if(WIN32)
    set_target_properties(OpenAvnu::PCAP PROPERTIES
      INTERFACE_LINK_DIRECTORIES "${PCAP_LIBRARY_DIRS}"
    )
  endif()
endif()
