# CMakeLists.txt PCAP Configuration Fix

This shows how to update the CMakeLists.txt files to support both legacy WinPcap and modern Npcap configurations.

## Example Fix for daemons/maap/CMakeLists.txt

### Current Code (Lines 33-40):
```cmake
include_directories( ${OsDir} $ENV{WPCAP_DIR}/Include )
if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
  link_directories($ENV{WPCAP_DIR}/Lib/x64)
elseif( CMAKE_SIZEOF_VOID_P EQUAL 4 )
  link_directories($ENV{WPCAP_DIR}/Lib)
endif()
add_executable(maap_daemon ${ComSource} ${OsSource})
target_link_libraries(maap_daemon wpcap Packet Iphlpapi ws2_32)
```

### Improved Code (Flexible PCAP Detection):
```cmake
# Flexible PCAP library detection - supports both WinPcap and Npcap
if(DEFINED ENV{WPCAP_DIR})
    set(PCAP_ROOT $ENV{WPCAP_DIR})
    message(STATUS "Using WinPcap from: ${PCAP_ROOT}")
elseif(DEFINED ENV{NPCAP_SDK_DIR})
    set(PCAP_ROOT $ENV{NPCAP_SDK_DIR})
    message(STATUS "Using Npcap SDK from: ${PCAP_ROOT}")
else()
    message(FATAL_ERROR "Neither WPCAP_DIR nor NPCAP_SDK_DIR environment variable is set. Please install WinPcap or Npcap SDK.")
endif()

include_directories(${OsDir} ${PCAP_ROOT}/Include)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    link_directories(${PCAP_ROOT}/Lib/x64)
    message(STATUS "Using 64-bit PCAP libraries from: ${PCAP_ROOT}/Lib/x64")
else()
    link_directories(${PCAP_ROOT}/Lib)
    message(STATUS "Using 32-bit PCAP libraries from: ${PCAP_ROOT}/Lib")
endif()

add_executable(maap_daemon ${ComSource} ${OsSource})
target_link_libraries(maap_daemon wpcap Packet Iphlpapi ws2_32)
```

## Files That Need Similar Updates

1. **daemons/maap/CMakeLists.txt** (lines 33-40)
2. **daemons/mrpd/CMakeLists.txt** (lines 15-23)  
3. **daemons/mrpd/tests/simple/CMakeLists.txt** (lines 35-43)
4. **daemons/maap/tests/CMakeLists.txt** (lines 27-29)
5. **daemons/common/tests/CMakeLists.txt** (lines 25-27)

## Benefits of This Approach

- **Backward Compatible**: Still works with existing WinPcap installations
- **Forward Compatible**: Supports modern Npcap SDK installations  
- **Clear Error Messages**: Tells users exactly what's missing
- **Flexible**: Works in both development and CI environments
- **Informative**: Provides debug output showing which PCAP library is being used

## Alternative Quick Fix for CI

If you prefer not to modify the CMakeLists.txt files, the CI pipeline can simply set:
```yaml
env:
  WPCAP_DIR: ${{ env.NPCAP_SDK_DIR }}
```

This makes the existing code work with Npcap by aliasing the environment variable.

## Testing the Fix

After applying either approach, the build should successfully find:
- `pcap.h` header file
- `wpcap.lib` library file  
- `Packet.lib` library file

All in the correct architecture-specific directories (x64 for 64-bit builds).
