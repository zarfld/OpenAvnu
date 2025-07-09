@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
echo Compiling minimal test...
cl /c /O2 /W3 test_minimal.c /Fo:build\test_minimal.obj
echo Minimal compilation completed!
