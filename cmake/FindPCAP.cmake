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

  # Set up Windows include directories with fallback options
  set(PCAP_POSSIBLE_INCLUDE_DIRS
    "${PCAP_ROOT_DIR}/Include"
    "${PCAP_ROOT_DIR}/include"
    "${PCAP_ROOT_DIR}/inc"
    "${PCAP_ROOT_DIR}"
  )
  
  set(PCAP_INCLUDE_DIRS)
  foreach(INC_DIR ${PCAP_POSSIBLE_INCLUDE_DIRS})
    if(EXISTS "${INC_DIR}")
      # Check if this directory actually contains pcap headers
      if(EXISTS "${INC_DIR}/pcap.h" OR EXISTS "${INC_DIR}/pcap/pcap.h")
        set(PCAP_INCLUDE_DIRS "${INC_DIR}")
        message(STATUS "📁 Using PCAP include directory: ${PCAP_INCLUDE_DIRS}")
        break()
      endif()
    endif()
  endforeach()
  
  # Fallback if no headers found
  if(NOT PCAP_INCLUDE_DIRS)
    set(PCAP_INCLUDE_DIRS "${PCAP_ROOT_DIR}/Include")
    message(WARNING 
      "⚠️  No pcap.h found in standard locations, using default: ${PCAP_INCLUDE_DIRS}\n"
      "Searched paths:\n"
      "  ${PCAP_POSSIBLE_INCLUDE_DIRS}"
    )
  endif()
  
  # Architecture-specific library paths with fallback options
  set(PCAP_POSSIBLE_LIB_DIRS)
  
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    # 64-bit architecture - try multiple possible paths
    list(APPEND PCAP_POSSIBLE_LIB_DIRS 
      "${PCAP_ROOT_DIR}/Lib/x64"
      "${PCAP_ROOT_DIR}/Lib"
      "${PCAP_ROOT_DIR}/lib/x64" 
      "${PCAP_ROOT_DIR}/lib"
      "${PCAP_ROOT_DIR}/libs/x64"
      "${PCAP_ROOT_DIR}/libs"
    )
  elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    # 32-bit architecture - try multiple possible paths
    list(APPEND PCAP_POSSIBLE_LIB_DIRS
      "${PCAP_ROOT_DIR}/Lib"
      "${PCAP_ROOT_DIR}/Lib/x86"
      "${PCAP_ROOT_DIR}/lib"
      "${PCAP_ROOT_DIR}/lib/x86"
      "${PCAP_ROOT_DIR}/libs"
      "${PCAP_ROOT_DIR}/libs/x86"
    )
  else()
    message(FATAL_ERROR "Unsupported architecture for PCAP on Windows")
  endif()
  
  # Find the first existing library directory
  set(PCAP_LIBRARY_DIRS)
  foreach(LIB_DIR ${PCAP_POSSIBLE_LIB_DIRS})
    if(EXISTS "${LIB_DIR}")
      set(PCAP_LIBRARY_DIRS "${LIB_DIR}")
      message(STATUS "📁 Using PCAP library directory: ${PCAP_LIBRARY_DIRS}")
      break()
    endif()
  endforeach()
  
  # Verify we found a valid library directory
  if(NOT PCAP_LIBRARY_DIRS)
    message(STATUS "⚠️  No standard library directory found, trying fallback...")
    # Fallback: use root directory if it contains .lib files directly
    file(GLOB PCAP_LIB_FILES "${PCAP_ROOT_DIR}/*.lib")
    if(PCAP_LIB_FILES)
      set(PCAP_LIBRARY_DIRS "${PCAP_ROOT_DIR}")
      message(STATUS "📁 Using fallback PCAP library directory: ${PCAP_LIBRARY_DIRS}")
    else()
      message(WARNING 
        "⚠️  PCAP library directory structure not recognized!\n"
        "Searched paths:\n"
        "  ${PCAP_POSSIBLE_LIB_DIRS}\n"
        "Falling back to root directory: ${PCAP_ROOT_DIR}"
      )
      set(PCAP_LIBRARY_DIRS "${PCAP_ROOT_DIR}")
    endif()
  endif()
  
  # Try to find the actual library file with fallback names
  set(PCAP_POSSIBLE_LIBRARY_NAMES
    wpcap
    pcap
    npcap
    libwpcap
    libpcap
    libnpcap
  )
  
  set(PCAP_LIBRARIES)
  foreach(LIB_NAME ${PCAP_POSSIBLE_LIBRARY_NAMES})
    find_library(PCAP_LIBRARY_${LIB_NAME}
      NAMES ${LIB_NAME}
      PATHS ${PCAP_LIBRARY_DIRS}
      NO_DEFAULT_PATH
    )
    if(PCAP_LIBRARY_${LIB_NAME})
      set(PCAP_LIBRARIES "${PCAP_LIBRARY_${LIB_NAME}}")
      message(STATUS "📚 Found PCAP library: ${PCAP_LIBRARIES}")
      break()
    endif()
  endforeach()
  
  # Fallback to traditional method if no library found
  if(NOT PCAP_LIBRARIES)
    set(PCAP_LIBRARIES "${PCAP_LIBRARY_DIRS}/wpcap.lib")
    message(WARNING 
      "⚠️  No PCAP library found via find_library, using default: ${PCAP_LIBRARIES}\n"
      "Searched for: ${PCAP_POSSIBLE_LIBRARY_NAMES} in ${PCAP_LIBRARY_DIRS}"
    )
  endif()
  
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
