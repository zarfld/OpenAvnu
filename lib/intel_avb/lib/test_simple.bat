@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
echo Compiling simple test...
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /I. test_simple.c /Fo:build\test_simple.obj
link /OUT:build\test_simple.exe build\test_simple.obj
echo Simple test completed!
