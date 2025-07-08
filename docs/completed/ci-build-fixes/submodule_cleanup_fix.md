# Submodule Cleanup Fix

## Issue
GitHub pipeline Windows build was failing with the error:
```
fatal: No url found for submodule path 'thirdparty/la_avdecc' in .gitmodules
```

## Root Cause
A stale git index entry existed for `thirdparty/la_avdecc` even though:
- The L-Acoustics AVDECC submodule was correctly moved to `lib/la_avdecc`
- The `.gitmodules` file was correctly updated
- The old git index reference was not properly cleaned up

## Fix Applied
1. Removed the stale cached reference: `git rm --cached thirdparty/la_avdecc`
2. Cleaned up any physical directory remnants
3. Committed the fix

## Result
- GitHub pipeline Windows builds should now succeed
- All submodules are properly configured
- No duplicate or stale references remain

## Verification
Run `git submodule status` to verify all submodules are correctly configured:
```
avdecc-lib (remotes/origin/avnu-master)
lib/atl_avb 
lib/igb_avb (v1.1.1~4)
lib/la_avdecc (v4.1.0)
thirdparty/cpputest (v3.4-227-g1d95a390)
thirdparty/gptp (heads/master)
thirdparty/open1722 (v0.8.0-2-g440e07e)
```
