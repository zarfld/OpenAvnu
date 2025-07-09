@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
echo.
echo ========================================================
echo  Building Intel HAL with Windows Driver Interface
echo ========================================================
echo.
echo Compiling core Intel HAL library...
cl /c /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /I. intel.c /Fo:build\intel.obj
cl /c /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /I. intel_common.c /Fo:build\intel_common.obj
cl /c /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /I. intel_i210.c /Fo:build\intel_i210.obj
cl /c /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /I. intel_i219.c /Fo:build\intel_i219.obj
cl /c /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /I. intel_i225.c /Fo:build\intel_i225.obj
echo.
echo Compiling Windows driver interface...
cl /c /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /I. intel_windows.c /Fo:build\intel_windows.obj
echo.
echo Creating Intel HAL static library...
lib /OUT:build\intel_avb_full.lib build\intel.obj build\intel_common.obj build\intel_i210.obj build\intel_i219.obj build\intel_i225.obj build\intel_windows.obj
echo.
echo Creating Intel HAL dynamic library...
link /DLL /OUT:build\intel_avb_full.dll build\intel.obj build\intel_common.obj build\intel_i210.obj build\intel_i219.obj build\intel_i225.obj build\intel_windows.obj
echo.
echo Compiling driver interface test program...
cl /c /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /I. test_driver.c /Fo:build\test_driver.obj
link /OUT:build\test_driver.exe build\test_driver.obj build\intel_common.obj build\intel_windows.obj
echo.
echo ========================================================
echo  Build Complete!
echo ========================================================
echo.
echo Output files:
echo   build\intel_avb_full.lib  - Full Intel HAL static library
echo   build\intel_avb_full.dll  - Full Intel HAL dynamic library  
echo   build\test_driver.exe     - Windows driver interface test
echo.
echo Running driver interface test...
echo.
build\test_driver.exe
