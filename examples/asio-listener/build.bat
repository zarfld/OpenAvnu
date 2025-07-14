@echo off
REM ASIO Audio Listener Build Script for Windows
REM Automatically detects and configures Visual Studio environment

echo ====================================================
echo ASIO AVB Audio Listener - Build Script
echo ====================================================
echo.

REM Try to find Visual Studio installation
set "VS_FOUND="
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

if exist "%VSWHERE%" (
    echo Looking for Visual Studio installation...
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
        set "VS_PATH=%%i"
        set "VS_FOUND=1"
    )
)

if not defined VS_FOUND (
    echo ERROR: Visual Studio not found!
    echo.
    echo Please install one of the following:
    echo   1. Visual Studio 2019/2022 with C++ build tools
    echo   2. Visual Studio Build Tools 2019/2022
    echo   3. Visual Studio Community (free)
    echo.
    echo Download from: https://visualstudio.microsoft.com/downloads/
    pause
    exit /b 1
)

echo Found Visual Studio at: %VS_PATH%

REM Set up Visual Studio environment
if exist "%VS_PATH%\VC\Auxiliary\Build\vcvars64.bat" (
    echo Setting up Visual Studio x64 environment...
    call "%VS_PATH%\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
) else (
    echo ERROR: Visual Studio build tools not properly installed
    pause
    exit /b 1
)

REM Check for required dependencies
echo.
echo Checking dependencies...

REM Check ASIO SDK
set "ASIO_SDK_DIR="
if exist "C:\ASIOSDK2.3" set "ASIO_SDK_DIR=C:\ASIOSDK2.3"
if exist "C:\ASIO_SDK" set "ASIO_SDK_DIR=C:\ASIO_SDK"
if exist "%USERPROFILE%\Documents\ASIOSDK2.3" set "ASIO_SDK_DIR=%USERPROFILE%\Documents\ASIOSDK2.3"

if not defined ASIO_SDK_DIR (
    echo WARNING: ASIO SDK not found!
    echo.
    echo Download from: https://www.steinberg.net/developers/
    echo Expected locations:
    echo   - C:\ASIOSDK2.3
    echo   - C:\ASIO_SDK
    echo   - %USERPROFILE%\Documents\ASIOSDK2.3
    echo.
    echo Continuing with stub implementation...
    set "ASIO_SDK_DIR=."
    set "ASIO_STUB=1"
) else (
    echo Found ASIO SDK at: %ASIO_SDK_DIR%
)

REM Check PCAP SDK
set "PCAP_DIR="
if exist "C:\npcap-sdk" set "PCAP_DIR=C:\npcap-sdk"
if exist "C:\WpdPack" set "PCAP_DIR=C:\WpdPack"
if exist "%USERPROFILE%\npcap-sdk" set "PCAP_DIR=%USERPROFILE%\npcap-sdk"

if not defined PCAP_DIR (
    echo WARNING: PCAP SDK not found!
    echo.
    echo Download one of:
    echo   - Npcap SDK (recommended): https://npcap.com/dist/npcap-sdk-1.13.zip
    echo   - WinPcap SDK (legacy): https://www.winpcap.org/install/bin/WpdPack_4_1_2.zip
    echo Expected locations:
    echo   - C:\npcap-sdk
    echo   - C:\WpdPack
    echo   - %USERPROFILE%\npcap-sdk
    echo.
    echo Cannot build without PCAP SDK!
    pause
    exit /b 1
) else (
    echo Found PCAP SDK at: %PCAP_DIR%
)

echo.
echo ====================================================
echo Building ASIO Audio Listener...
echo ====================================================

REM Set compiler flags
set "CFLAGS=/D_CRT_SECURE_NO_WARNINGS /DWIN32_LEAN_AND_MEAN /I. /I"%ASIO_SDK_DIR%" /I"%PCAP_DIR%\Include""
set "LDFLAGS=ws2_32.lib winmm.lib "%PCAP_DIR%\Lib\x64\wpcap.lib" "%PCAP_DIR%\Lib\x64\Packet.lib""

if defined ASIO_STUB (
    set "CFLAGS=%CFLAGS% /DASIO_STUB"
)

REM Clean previous build
if exist asio_listener.exe del asio_listener.exe
if exist *.obj del *.obj

REM Compile
echo Compiling asio_listener.c...
cl %CFLAGS% /c asio_listener.c
if errorlevel 1 (
    echo.
    echo ERROR: Compilation failed!
    echo.
    echo Common issues:
    echo   1. Missing ASIO SDK headers
    echo   2. Missing PCAP SDK headers  
    echo   3. Incorrect include paths
    echo.
    pause
    exit /b 1
)

REM Link
echo Linking executable...
link asio_listener.obj %LDFLAGS% /out:asio_listener.exe
if errorlevel 1 (
    echo.
    echo ERROR: Linking failed!
    echo.
    echo Common issues:
    echo   1. Missing PCAP libraries
    echo   2. Incorrect library paths
    echo   3. Architecture mismatch (x86 vs x64)
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
echo Usage: asio_listener.exe ^<network_interface^> [asio_driver_name]
echo Example: asio_listener.exe "Intel I219" "ASIO4ALL v2"
echo.
echo To run with ASIO4ALL (universal driver):
echo   1. Download and install ASIO4ALL from http://www.asio4all.org/
echo   2. Run: asio_listener.exe "Intel I219" "ASIO4ALL v2"
echo.
pause
