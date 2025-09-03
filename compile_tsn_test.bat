@echo off
echo Compiling TSN Integration Test for I226 Hardware...
echo.

REM Set up Visual Studio environment
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" x64

REM Navigate to source directory
cd /d "C:\Users\dzarf\source\repos\OpenAvnu-2\lib\avtp_pipeline"

echo Compiling with Visual Studio cl.exe...
cl.exe /I..\..\thirdparty\intel-ethernet-hal\include ^
       /I..\..\build\thirdparty\intel-ethernet-hal\Release ^
       test_intel_tsn_integration.c intel_tsn_integration.c ^
       /link ..\..\build\thirdparty\intel-ethernet-hal\Release\intel-ethernet-hal-static.lib ^
       ws2_32.lib ^
       /OUT:test_tsn_i226.exe

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ✅ Compilation successful! Running TSN test...
    echo.
    test_tsn_i226.exe
) else (
    echo.
    echo ❌ Compilation failed. Trying alternate approach...
    echo.
    
    REM Try MinGW if available
    where gcc >nul 2>nul
    if %ERRORLEVEL% EQU 0 (
        echo Trying with MinGW gcc...
        gcc -I..\..\thirdparty\intel-ethernet-hal\include ^
            -L..\..\build\thirdparty\intel-ethernet-hal\Release ^
            test_intel_tsn_integration.c intel_tsn_integration.c ^
            -lintel-ethernet-hal-static -lws2_32 ^
            -o test_tsn_i226_gcc.exe
        
        if %ERRORLEVEL% EQU 0 (
            echo MinGW compilation successful! Running test...
            test_tsn_i226_gcc.exe
        ) else (
            echo Both compilation attempts failed.
        )
    ) else (
        echo No alternative compiler found.
    )
)

pause
