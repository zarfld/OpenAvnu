@echo off
REM Quick setup script for ASIO Audio Listener development environment

echo ========================================================
echo ASIO Audio Listener - Development Environment Setup
echo ========================================================
echo.

echo This script will help you set up the development environment.
echo.
echo Required components:
echo   1. Visual Studio Build Tools (C++ compiler)
echo   2. ASIO SDK (from Steinberg)
echo   3. Npcap SDK (for network packet capture)
echo.

REM Check if we're running as administrator
net session >nul 2>&1
if %errorLevel% == 0 (
    echo Running as Administrator: YES
) else (
    echo Running as Administrator: NO
    echo.
    echo NOTICE: Some installations may require administrator privileges.
    echo Right-click and "Run as administrator" if you encounter issues.
)

echo.
echo ========================================================
echo Step 1: Visual Studio Build Tools
echo ========================================================
echo.

REM Check for existing Visual Studio installation
set "VS_FOUND="
if exist "C:\Program Files\Microsoft Visual Studio\2022" set "VS_FOUND=2022"
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019" set "VS_FOUND=2019"

if defined VS_FOUND (
    echo Found Visual Studio %VS_FOUND% installation
    echo You should be able to build from Developer Command Prompt
) else (
    echo Visual Studio not found.
    echo.
    echo Options:
    echo   1. Download Visual Studio Community 2022 (Free, Full IDE)
    echo      https://visualstudio.microsoft.com/vs/community/
    echo.
    echo   2. Download Build Tools for Visual Studio 2022 (Minimal)
    echo      https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022
    echo.
    echo   3. Use automated installer (downloads ~3GB):
    
    choice /M "Download Visual Studio Build Tools automatically"
    if errorlevel 2 goto :manual_vs
    if errorlevel 1 goto :auto_vs
    
    :auto_vs
    echo Downloading Visual Studio Build Tools...
    powershell -Command "Invoke-WebRequest -Uri 'https://aka.ms/vs/17/release/vs_buildtools.exe' -OutFile 'vs_buildtools.exe'"
    if exist vs_buildtools.exe (
        echo Starting Visual Studio Build Tools installer...
        echo.
        echo Please select "C++ build tools" workload during installation.
        start /wait vs_buildtools.exe --add Microsoft.VisualStudio.Workload.VCTools
        del vs_buildtools.exe
    ) else (
        echo Download failed. Please download manually.
    )
    goto :check_asio
    
    :manual_vs
    echo Please download and install Visual Studio Build Tools manually.
    echo After installation, run this script again.
)

:check_asio
echo.
echo ========================================================
echo Step 2: ASIO SDK
echo ========================================================
echo.

set "ASIO_FOUND="
if exist "C:\ASIOSDK2.3" set "ASIO_FOUND=C:\ASIOSDK2.3"
if exist "C:\ASIO_SDK" set "ASIO_FOUND=C:\ASIO_SDK"

if defined ASIO_FOUND (
    echo Found ASIO SDK at: %ASIO_FOUND%
) else (
    echo ASIO SDK not found.
    echo.
    echo The ASIO SDK requires registration with Steinberg:
    echo   1. Go to: https://www.steinberg.net/developers/
    echo   2. Create free developer account
    echo   3. Download ASIO SDK 2.3 or later
    echo   4. Extract to C:\ASIOSDK2.3
    echo.
    echo Alternative for testing: Install ASIO4ALL
    choice /M "Download ASIO4ALL (universal ASIO driver)"
    if errorlevel 2 goto :check_pcap
    if errorlevel 1 (
        echo Opening ASIO4ALL download page...
        start https://www.asio4all.org/
        echo.
        echo Please download and install ASIO4ALL for testing.
        echo You can use this for development even without professional audio hardware.
    )
)

:check_pcap
echo.
echo ========================================================
echo Step 3: Npcap SDK
echo ========================================================
echo.

set "PCAP_FOUND="
if exist "C:\npcap-sdk" set "PCAP_FOUND=C:\npcap-sdk"
if exist "C:\WpdPack" set "PCAP_FOUND=C:\WpdPack"

if defined PCAP_FOUND (
    echo Found PCAP SDK at: %PCAP_FOUND%
) else (
    echo PCAP SDK not found.
    echo.
    echo For network packet capture, you need Npcap SDK:
    choice /M "Download Npcap SDK"
    if errorlevel 2 goto :build_test
    if errorlevel 1 (
        echo Downloading Npcap SDK...
        powershell -Command "Invoke-WebRequest -Uri 'https://npcap.com/dist/npcap-sdk-1.13.zip' -OutFile 'npcap-sdk.zip'"
        if exist npcap-sdk.zip (
            echo Extracting to C:\npcap-sdk...
            powershell -Command "Expand-Archive -Path 'npcap-sdk.zip' -DestinationPath 'C:\npcap-sdk' -Force"
            del npcap-sdk.zip
            echo Npcap SDK installed successfully!
        ) else (
            echo Download failed. You can download manually from:
            echo https://npcap.com/dist/npcap-sdk-1.13.zip
        )
    )
)

:build_test
echo.
echo ========================================================
echo Step 4: Test Build
echo ========================================================
echo.

echo Testing build environment...
if exist build_simple.bat (
    echo Running simple build test...
    call build_simple.bat
    if exist asio_listener.exe (
        echo.
        echo SUCCESS: Build test completed!
        echo asio_listener.exe created successfully.
        echo.
        echo You can now build the ASIO Audio Listener.
    ) else (
        echo.
        echo Build test failed. Please check:
        echo   1. Visual Studio Build Tools installation
        echo   2. Developer Command Prompt usage
        echo   3. SDK paths and environment variables
    )
) else (
    echo build_simple.bat not found. Please run from asio-listener directory.
)

echo.
echo ========================================================
echo Setup Complete
echo ========================================================
echo.
echo Next steps:
echo   1. Open "Developer Command Prompt for VS 2022"
echo   2. Navigate to: D:\Repos\OpenAvnu\examples\asio-listener
echo   3. Run: build.bat (with full SDKs) or build_simple.bat (stub version)
echo   4. Test: asio_listener.exe "interface_name" "asio_driver"
echo.
echo For detailed instructions, see: SETUP_GUIDE.md
echo.
pause
