# CI Build Configuration Fix

## Issue: PCAP Library Configuration

The CI pipeline is now successfully initializing all submodules but failing at the build stage due to PCAP library configuration issues.

### Current Status ✅ 
- **Submodule Initialization**: ✅ All submodules successfully fetched and initialized
- **CMake Configuration**: ✅ Successfully configured with Visual Studio 2022
- **Build Progress**: ✅ Most components building successfully

### Build Errors ❌
```
error C1083: Cannot open include file: 'pcap.h': No such file or directory
fatal error LNK1181: cannot open input file 'wpcap.lib'
```

### Root Cause Analysis

The OpenAvnu build system expects the old WinPcap directory structure via the `WPCAP_DIR` environment variable:
- **CMake expects**: `$ENV{WPCAP_DIR}/Include` and `$ENV{WPCAP_DIR}/Lib/x64`
- **CI provides**: `NPCAP_SDK_DIR=C:\npcap-sdk` (modern Npcap SDK structure)

### Directory Structure Comparison

**Expected (WinPcap):**
```
$WPCAP_DIR/
├── Include/
│   ├── pcap.h
│   └── Packet32.h
└── Lib/
    ├── x64/
    │   └── wpcap.lib
    └── wpcap.lib
```

**Actual (Npcap SDK):**
```
$NPCAP_SDK_DIR/
├── Include/
│   ├── pcap.h
│   └── Packet32.h
└── Lib/
    ├── x64/
    │   └── wpcap.lib
    └── wpcap.lib
```

### Solution Options

#### Option 1: Set WPCAP_DIR Environment Variable (Recommended)
Add to CI pipeline before CMake configuration:
```yaml
- name: Configure PCAP paths
  run: |
    echo "WPCAP_DIR=C:\npcap-sdk" >> $GITHUB_ENV
  shell: bash
```

#### Option 2: Update CMakeLists.txt (Alternative)
Modify the CMakeLists.txt files to also check for NPCAP_SDK_DIR:
```cmake
# Check for both WPCAP_DIR and NPCAP_SDK_DIR
if(DEFINED ENV{WPCAP_DIR})
    set(PCAP_ROOT $ENV{WPCAP_DIR})
elseif(DEFINED ENV{NPCAP_SDK_DIR})
    set(PCAP_ROOT $ENV{NPCAP_SDK_DIR})
else()
    message(FATAL_ERROR "Neither WPCAP_DIR nor NPCAP_SDK_DIR environment variable is set")
endif()

include_directories(${PCAP_ROOT}/Include)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    link_directories(${PCAP_ROOT}/Lib/x64)
else()
    link_directories(${PCAP_ROOT}/Lib)
endif()
```

### Files That Need PCAP Configuration

1. **daemons/maap/CMakeLists.txt** - MAAP daemon
2. **daemons/mrpd/CMakeLists.txt** - MRPD daemon  
3. **daemons/mrpd/tests/simple/CMakeLists.txt** - MRPD tests
4. **daemons/maap/tests/CMakeLists.txt** - MAAP tests
5. **daemons/common/tests/CMakeLists.txt** - Common tests

### Build Progress Summary

**✅ Successfully Building:**
- CppUTest framework
- CppUTestExt framework  
- daemons/common/tests
- lib/avtp_pipeline/tests
- daemons/maap/tests

**❌ Failing Due to PCAP:**
- daemons/maap (maap_daemon)
- daemons/mrpd (mrpd)
- daemons/mrpd/tests/simple (mrpd_simple_test)

### Recommended CI Pipeline Fix

Add this step before the CMake configuration:

```yaml
- name: Configure PCAP environment for legacy build system
  run: |
    echo "WPCAP_DIR=$env:NPCAP_SDK_DIR" >> $env:GITHUB_ENV
  shell: powershell
```

This will make the legacy OpenAvnu build system compatible with the modern Npcap SDK without requiring code changes.

### Profile Framework Impact

The profile framework and L-Acoustics AVDECC integration are not affected by this issue as they don't directly depend on the PCAP library for their core functionality. The build failures are in the network daemons (MAAP, MRPD) which are separate components.

### Next Steps

1. **Immediate**: Configure `WPCAP_DIR` environment variable in CI
2. **Long-term**: Consider updating CMakeLists.txt files to be more flexible with PCAP library detection
3. **Testing**: Verify that all components build successfully after the fix
