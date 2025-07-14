@echo off
REM Simple build script using Windows SDK compiler
REM This avoids the need for full Visual Studio installation

echo ====================================================
echo ASIO AVB Audio Listener - Simple Build
echo ====================================================
echo.

REM Try to find Windows SDK
set "SDK_FOUND="
set "CL_PATH="

REM Check common Windows SDK locations
for /d %%i in ("C:\Program Files (x86)\Windows Kits\10\bin\*") do (
    if exist "%%i\x64\cl.exe" (
        set "CL_PATH=%%i\x64"
        set "SDK_FOUND=1"
        goto :found_sdk
    )
)

for /d %%i in ("C:\Program Files\Microsoft SDKs\Windows\*") do (
    if exist "%%i\bin\x64\cl.exe" (
        set "CL_PATH=%%i\bin\x64"
        set "SDK_FOUND=1"
        goto :found_sdk
    )
)

:found_sdk
if not defined SDK_FOUND (
    echo WARNING: Windows SDK compiler not found
    echo Trying to use system PATH...
    where cl.exe >nul 2>&1
    if errorlevel 1 (
        echo.
        echo ERROR: No C compiler found!
        echo.
        echo Install one of:
        echo   1. Visual Studio 2019/2022 with C++ tools
        echo   2. Visual Studio Build Tools
        echo   3. Windows SDK with compiler tools
        echo.
        echo Or use our simplified gcc version:
        echo   build_simple.bat
        echo.
        pause
        exit /b 1
    )
    set "CL_PATH="
) else (
    echo Found Windows SDK compiler at: %CL_PATH%
    set "PATH=%CL_PATH%;%PATH%"
)

REM Check for PCAP SDK (required)
set "PCAP_DIR="
if exist "C:\npcap-sdk" set "PCAP_DIR=C:\npcap-sdk"
if exist "C:\WpdPack" set "PCAP_DIR=C:\WpdPack"
if exist "%USERPROFILE%\npcap-sdk" set "PCAP_DIR=%USERPROFILE%\npcap-sdk"

if not defined PCAP_DIR (
    echo.
    echo Creating minimal PCAP stub for compilation...
    echo This will create a version that compiles but doesn't capture packets.
    echo For full functionality, install Npcap SDK.
    echo.
    set "USE_PCAP_STUB=1"
) else (
    echo Found PCAP SDK at: %PCAP_DIR%
)

echo.
echo ====================================================
echo Building with stubs (development version)...
echo ====================================================

REM Clean previous build
if exist asio_listener.exe del asio_listener.exe >nul 2>&1
if exist *.obj del *.obj >nul 2>&1

REM Set compiler flags for stub build
set "CFLAGS=/D_CRT_SECURE_NO_WARNINGS /DWIN32_LEAN_AND_MEAN /DASIO_STUB /I."

if defined USE_PCAP_STUB (
    set "CFLAGS=%CFLAGS% /DPCAP_STUB"
    set "LDFLAGS=ws2_32.lib"
) else (
    set "CFLAGS=%CFLAGS% /I"%PCAP_DIR%\Include""
    set "LDFLAGS=ws2_32.lib "%PCAP_DIR%\Lib\x64\wpcap.lib" "%PCAP_DIR%\Lib\x64\Packet.lib""
)

echo Compiling with ASIO stub implementation...
echo Compiler flags: %CFLAGS%

REM Compile
cl %CFLAGS% asio_listener.c /link %LDFLAGS% /out:asio_listener.exe
if errorlevel 1 (
    echo.
    echo ERROR: Build failed!
    echo.
    echo If you get missing header errors, the stubs may need updates.
    echo Try installing the full ASIO SDK and Npcap SDK for complete functionality.
    echo.
    pause
    exit /b 1
)

echo.
echo ====================================================
echo Build completed successfully!
echo ====================================================
echo.
echo Output: asio_listener.exe
echo.
echo NOTE: This is a development build with stub implementations.
echo       For full functionality, you need:
echo       1. ASIO SDK from Steinberg
echo       2. Npcap SDK for packet capture
echo       3. Professional ASIO audio drivers
echo.
echo Test run (limited functionality):
echo   asio_listener.exe "any_interface" "stub_driver"
echo.

if defined USE_PCAP_STUB (
    echo WARNING: Built with PCAP stub - no network capture
    echo Install Npcap SDK for actual AVB packet reception
    echo.
)

pause
