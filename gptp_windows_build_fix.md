# gPTP Windows Build Fix

This document describes a known Windows build issue with the gPTP submodule and provides solutions.

## Problem
The gPTP CMakeLists.txt uses `${CMAKE_SOURCE_DIR}` to copy configuration files, but when used as a submodule, this refers to the parent project's source directory instead of the gPTP submodule directory, causing build failures with errors like:

```
Error copying file (if different) from "D:/Repos/OpenAvnu/gptp_cfg.ini" to "D:/Repos/OpenAvnu/build/thirdparty/gptp/Release/gptp_cfg.ini".
```

## Root Cause
The issue occurs in `thirdparty/gptp/CMakeLists.txt` where configuration file paths use `${CMAKE_SOURCE_DIR}` which points to the OpenAvnu root directory instead of the gPTP submodule directory.

## Solutions

### Solution 1: Apply Manual Fix (Recommended)
Edit `thirdparty/gptp/CMakeLists.txt` and change all instances of `${CMAKE_SOURCE_DIR}` to `${CMAKE_CURRENT_SOURCE_DIR}` in the configuration file copy commands (around lines 78-105).

**Before:**
```cmake
"${CMAKE_SOURCE_DIR}/gptp_cfg.ini"
```

**After:**
```cmake
"${CMAKE_CURRENT_SOURCE_DIR}/gptp_cfg.ini"
```

### Solution 2: Apply Patch File
If you encounter build errors, apply the provided patch:

```bash
cd thirdparty/gptp
git apply ../../gptp_windows_build_fix.patch
```

### Solution 3: PowerShell One-Liner
For quick fix on Windows:

```powershell
(Get-Content thirdparty\gptp\CMakeLists.txt) -replace '\$\{CMAKE_SOURCE_DIR\}', '${CMAKE_CURRENT_SOURCE_DIR}' | Set-Content thirdparty\gptp\CMakeLists.txt
```

## Files Modified
- `thirdparty/gptp/CMakeLists.txt` - Configuration file copy commands updated

## Verification
After applying the fix, you should see successful configuration file copying during build:

```
Copying gptp_cfg.ini to output directory
Copying test_milan_config.ini to output directory
Copying avnu_presonus_config.ini to output directory
Copying avnu_base_config.ini to output directory
Copying test_unified_profiles.ini to output directory
```

## Build Test
Test the fix by building the gPTP target:

```bash
cd build
cmake --build . --config Release --target gptp
```

## Status
This fix has been tested and verified to work on Windows with Visual Studio 2022. The issue has been reported to the upstream zarfld/gptp repository.

## Alternative
If you prefer not to modify the submodule, you can disable gPTP building and use an external gPTP daemon:

```bash
cmake .. -DOPENAVNU_BUILD_GPTP=OFF
```
