@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
echo.
echo ================================================
echo  Building Intel HAL Hardware Test Suite
echo ================================================
echo.
echo Compiling hardware test program...
cl /c /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /I. test_hardware.c /Fo:build\test_hardware.obj
cl /c /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /I. intel_common.c /Fo:build\test_hardware_common.obj
echo.
echo Linking hardware test executable...
link /OUT:build\test_hardware.exe build\test_hardware.obj build\test_hardware_common.obj
echo.
echo ================================================
echo  Hardware Test Build Complete!
echo ================================================
echo.
echo Running hardware test suite...
echo.
build\test_hardware.exe
