cmake_minimum_required(VERSION 2.6)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
project(AVB)

# Use the Windows OSAL implementation
set(OPENAVB_OSAL Windows)

if(MSVC)
  # Link against the static runtime so that the resulting binaries do not
  # require the Microsoft DLL runtimes to be installed.
  if(NOT CMAKE_MSVC_RUNTIME_LIBRARY)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  endif()
endif()

message("-- Using Windows toolchain")

# Default build type handling copied from Linux version
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build" FORCE)
else()
  if(NOT (("${CMAKE_BUILD_TYPE}" STREQUAL "Release") OR
          ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug") OR
          ("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo") OR
          ("${CMAKE_BUILD_TYPE}" STREQUAL "MinSizeRel")))
    message(FATAL_ERROR "Unknown CMAKE_BUILD_TYPE")
  endif()
endif()
message("-- Build type is ${CMAKE_BUILD_TYPE}")

# Set a define to signal build to source files
string(TOUPPER "${CMAKE_BUILD_TYPE}_BUILD" BUILD_TYPE_STRING)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D${BUILD_TYPE_STRING}")

# Windows specific compile flags
if(MSVC)
  add_definitions(/W3 /D_CRT_SECURE_NO_WARNINGS)
else()
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
endif()

# WinPcap/Npcap support
if(DEFINED ENV{WPCAP_DIR})
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(PCAP_LIB_DIR "$ENV{WPCAP_DIR}/Lib/x64")
  else()
    set(PCAP_LIB_DIR "$ENV{WPCAP_DIR}/Lib")
  endif()
  set(PCAP_INCLUDE_DIR "$ENV{WPCAP_DIR}/Include")
  include_directories(${PCAP_INCLUDE_DIR})
  link_directories(${PCAP_LIB_DIR})
  set(PLATFORM_LINK_LIBRARIES ${PLATFORM_LINK_LIBRARIES} wpcap Iphlpapi Ws2_32)
endif()

# include original Linux build logic for remaining targets
include(${CMAKE_CURRENT_LIST_DIR}/../Linux/CMakeLists.txt)

