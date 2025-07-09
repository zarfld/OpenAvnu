@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
echo Generating preprocessed file...
cl /EP /D_CRT_SECURE_NO_WARNINGS /I. intel_common.c > build\intel_common_preprocessed.c
echo Preprocessed file generated.
