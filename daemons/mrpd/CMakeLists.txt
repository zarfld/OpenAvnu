cmake_minimum_required (VERSION 3.10) 
project (mrpd)
enable_testing()

include_directories( . "../common" )
file(GLOB MRPD_SRC "mrp.c" "mvrp.c" "mmrp.c" "msrp.c" "../common/parse.c" "../common/eui64set.c" "../common/mrpd_intel_hal.c")

if(APPLE)
  add_executable (mrpd ${MRPD_SRC}  "mrpd.c")
elseif(UNIX)
  add_executable (mrpd ${MRPD_SRC}  "mrpd.c")
  target_link_libraries(mrpd pthread)
  
  # Intel HAL integration for Linux
  if(OPENAVNU_BUILD_INTEL_HAL)
    target_link_libraries(mrpd intel-ethernet-hal-static)
    target_include_directories(mrpd PRIVATE 
      ${CMAKE_SOURCE_DIR}/thirdparty/intel-ethernet-hal/include)
    target_compile_definitions(mrpd PRIVATE OPENAVNU_BUILD_INTEL_HAL)
    message(STATUS "MRPD: Intel HAL integration enabled for Linux")
  endif()
elseif(WIN32)
  # Flexible PCAP library detection - supports both WinPcap and Npcap
  if(DEFINED ENV{WPCAP_DIR})
    set(PCAP_ROOT $ENV{WPCAP_DIR})
    message(STATUS "MRPD: Using WinPcap from: ${PCAP_ROOT}")
  elseif(DEFINED ENV{NPCAP_SDK_DIR})
    set(PCAP_ROOT $ENV{NPCAP_SDK_DIR})
    message(STATUS "MRPD: Using Npcap SDK from: ${PCAP_ROOT}")
  else()
    message(FATAL_ERROR "MRPD: Neither WPCAP_DIR nor NPCAP_SDK_DIR environment variable is set. Please install WinPcap or Npcap SDK.")
  endif()

  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    link_directories(${PCAP_ROOT}/Lib/x64)
    message(STATUS "MRPD: Using 64-bit PCAP libraries from: ${PCAP_ROOT}/Lib/x64")
  else()
    link_directories(${PCAP_ROOT}/Lib)
    message(STATUS "MRPD: Using 32-bit PCAP libraries from: ${PCAP_ROOT}/Lib")
  endif()

  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
  include_directories(include ${PCAP_ROOT}/Include)
  add_executable(mrpd ${MRPD_SRC} "mrpw.c" "que.c")
  target_link_libraries(mrpd wpcap Packet Iphlpapi Ws2_32)
  
  # Intel HAL integration for Windows
  if(OPENAVNU_BUILD_INTEL_HAL)
    target_link_libraries(mrpd intel-ethernet-hal-static)
    target_include_directories(mrpd PRIVATE 
      ${CMAKE_SOURCE_DIR}/thirdparty/intel-ethernet-hal/include)
    target_compile_definitions(mrpd PRIVATE OPENAVNU_BUILD_INTEL_HAL)
    message(STATUS "MRPD: Intel HAL integration enabled for Windows")
  endif()
endif()

add_subdirectory("tests/simple")
