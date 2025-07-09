@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
echo Compiling probe test...
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /I. test_probe.c /Fo:build\test_probe.obj
link /OUT:build\test_probe.exe build\test_probe.obj build\intel_common.obj
echo Probe test completed!
