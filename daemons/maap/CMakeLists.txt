cmake_minimum_required (VERSION 3.10)
project (maap)

set (ComDir "./common")
set (ComSource
  "${ComDir}/intervals.c"
  "${ComDir}/maap.c"
  "${ComDir}/maap_net.c"
  "${ComDir}/maap_packet.c"
  "${ComDir}/maap_parse.c"
  "${ComDir}/maap_log_queue.c"
  "${ComDir}/maap_intel_hal.c"
)
include_directories( ${ComDir} )

if(UNIX)
  set (OsDir "./linux/src")
  set (OsSource
    "${OsDir}/maap_daemon.c"
    "${OsDir}/maap_log_linux.c"
    "${OsDir}/maap_timer_linux.c"
  )
  include_directories( ${OsDir} )
  add_executable(maap_daemon ${ComSource} ${OsSource})
  target_link_libraries(maap_daemon rt pthread)
  
  # Intel HAL integration
  if(OPENAVNU_BUILD_INTEL_HAL)
    target_link_libraries(maap_daemon intel-ethernet-hal-static)
    target_include_directories(maap_daemon PRIVATE 
      ${CMAKE_SOURCE_DIR}/thirdparty/intel-ethernet-hal/include)
    target_compile_definitions(maap_daemon PRIVATE OPENAVNU_BUILD_INTEL_HAL)
    message(STATUS "MAAP: Intel HAL integration enabled")
  endif()
elseif(WIN32)
  set (OsDir "./windows/src")
  set (OsSource
    "${OsDir}/maap_main.c"
    "${OsDir}/maap_log_windows.c"
    "${OsDir}/maap_timer_windows.c"
  )
  add_definitions(-D_CRT_SECURE_NO_WARNINGS -DHAVE_REMOTE)
  
  # Flexible PCAP library detection - supports both WinPcap and Npcap
  if(DEFINED ENV{WPCAP_DIR})
    set(PCAP_ROOT $ENV{WPCAP_DIR})
    message(STATUS "MAAP: Using WinPcap from: ${PCAP_ROOT}")
  elseif(DEFINED ENV{NPCAP_SDK_DIR})
    set(PCAP_ROOT $ENV{NPCAP_SDK_DIR})
    message(STATUS "MAAP: Using Npcap SDK from: ${PCAP_ROOT}")
  else()
    message(FATAL_ERROR "MAAP: Neither WPCAP_DIR nor NPCAP_SDK_DIR environment variable is set. Please install WinPcap or Npcap SDK.")
  endif()
  
  include_directories(${OsDir} ${PCAP_ROOT}/Include)
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    link_directories(${PCAP_ROOT}/Lib/x64)
    message(STATUS "MAAP: Using 64-bit PCAP libraries from: ${PCAP_ROOT}/Lib/x64")
  else()
    link_directories(${PCAP_ROOT}/Lib)
    message(STATUS "MAAP: Using 32-bit PCAP libraries from: ${PCAP_ROOT}/Lib")
  endif()
  
  add_executable(maap_daemon ${ComSource} ${OsSource})
  target_link_libraries(maap_daemon wpcap Packet Iphlpapi ws2_32)
  
  # Intel HAL integration for Windows
  if(OPENAVNU_BUILD_INTEL_HAL)
    target_link_libraries(maap_daemon intel-ethernet-hal-static)
    target_include_directories(maap_daemon PRIVATE 
      ${CMAKE_SOURCE_DIR}/thirdparty/intel-ethernet-hal/include)
    target_compile_definitions(maap_daemon PRIVATE OPENAVNU_BUILD_INTEL_HAL)
    message(STATUS "MAAP: Intel HAL integration enabled for Windows")
  endif()
endif()

add_subdirectory("test")
add_subdirectory("tests")
