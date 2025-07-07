# gPTP Windows Build Fix

This patch file contains a fix for the gPTP submodule to resolve Windows build issues.

## Problem
The gPTP CMakeLists.txt uses `${CMAKE_SOURCE_DIR}` to copy configuration files, but when used as a submodule, this refers to the parent project's source directory instead of the gPTP submodule directory, causing build failures.

## Solution
Change `${CMAKE_SOURCE_DIR}` to `${CMAKE_CURRENT_SOURCE_DIR}` in the configuration file copy commands.

## Application
If you encounter build errors related to missing configuration files during Windows builds, apply this patch:

```bash
cd thirdparty/gptp
git apply ../../gptp_windows_build_fix.patch
```

## Files Modified
- `CMakeLists.txt` - Configuration file copy commands updated

## Status
This fix has been submitted as a pull request to the upstream zarfld/gptp repository and should be included in future releases.
