@echo off
REM Intel AVB HAL Library Windows Build Script
REM
REM This script sets up the Visual Studio environment and builds the Intel HAL library

echo Building Intel AVB HAL Library for Windows...

REM Setup Visual Studio 2022 environment
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

REM Create output directory
if not exist "build" mkdir build

REM Compile source files
echo Compiling source files...
cl /c /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /I. /I..\..\..\lib\common intel.c /Fo:build\intel.obj
cl /c /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /I. /I..\..\..\lib\common intel_common.c /Fo:build\intel_common.obj
cl /c /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /I. /I..\..\..\lib\common intel_i210.c /Fo:build\intel_i210.obj
cl /c /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /I. /I..\..\..\lib\common intel_i219.c /Fo:build\intel_i219.obj
cl /c /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /I. /I..\..\..\lib\common intel_i225.c /Fo:build\intel_i225.obj

REM Create static library
echo Creating static library...
lib /OUT:build\intel_avb.lib build\intel.obj build\intel_common.obj build\intel_i210.obj build\intel_i219.obj build\intel_i225.obj

REM Create DLL
echo Creating dynamic library...
link /DLL /OUT:build\intel_avb.dll build\intel.obj build\intel_common.obj build\intel_i210.obj build\intel_i219.obj build\intel_i225.obj

REM Compile test program
echo Compiling test program...
cl /c /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /I. test_intel.c /Fo:build\test_intel.obj
cl /c /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /I. intel_common.c /Fo:build\test_intel_common.obj
link /OUT:build\test_intel.exe build\test_intel.obj build\test_intel_common.obj

echo Build complete!
echo.
echo Output files:
echo   build\intel_avb.lib   - Static library
echo   build\intel_avb.dll   - Dynamic library  
echo   build\test_intel.exe  - Test program
echo.
pause
