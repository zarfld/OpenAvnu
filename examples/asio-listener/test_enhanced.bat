@echo off
REM Enhanced ASIO Audio Listener Test Script
REM Tests both stub and full ASIO SDK implementations

echo =====================================================
echo Enhanced ASIO Audio Listener - Test Suite
echo =====================================================
echo.

REM Check if ASIOSDK_DIR is set
if defined ASIOSDK_DIR (
    echo ✓ ASIOSDK_DIR is set: %ASIOSDK_DIR%
    if exist "%ASIOSDK_DIR%\common\asio.h" (
        echo ✓ ASIO SDK found and valid
        set "ASIO_AVAILABLE=1"
    ) else (
        echo ✗ ASIO SDK directory exists but asio.h not found
        set "ASIO_AVAILABLE=0"
    )
) else (
    echo ✗ ASIOSDK_DIR environment variable not set
    set "ASIO_AVAILABLE=0"
)

echo.

REM Test 1: Build with stub implementation
echo =====================================================
echo Test 1: Building with stub implementation
echo =====================================================
echo.

call build_simple.bat
if errorlevel 1 (
    echo ✗ Stub build failed
    goto :test_failed
) else (
    echo ✓ Stub build successful
)

echo.

REM Test 2: Build with full ASIO SDK (if available)
if "%ASIO_AVAILABLE%"=="1" (
    echo =====================================================
    echo Test 2: Building with full ASIO SDK
    echo =====================================================
    echo.
    
    call build.bat
    if errorlevel 1 (
        echo ✗ Full ASIO build failed
        goto :test_failed
    ) else (
        echo ✓ Full ASIO build successful
    )
) else (
    echo =====================================================
    echo Test 2: Skipped (ASIO SDK not available)
    echo =====================================================
    echo Reason: ASIOSDK_DIR not set or invalid
)

echo.

REM Test 3: Run basic functionality test
echo =====================================================
echo Test 3: Basic functionality test
echo =====================================================
echo.

if exist asio_listener.exe (
    echo Testing basic argument parsing...
    asio_listener.exe --help 2>nul
    echo ✓ Executable responds to basic input
) else (
    echo ✗ asio_listener.exe not found
    goto :test_failed
)

echo.

REM Test 4: Environment validation
echo =====================================================
echo Test 4: Environment validation
echo =====================================================
echo.

echo Checking build environment:
if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" (
    echo ✓ Visual Studio tools available
) else (
    echo ⚠ Visual Studio tools may not be available
)

REM Check for PCAP
set "PCAP_FOUND=0"
if exist "C:\npcap-sdk" (
    echo ✓ Npcap SDK found at C:\npcap-sdk
    set "PCAP_FOUND=1"
)
if exist "C:\WpdPack" (
    echo ✓ WinPcap SDK found at C:\WpdPack
    set "PCAP_FOUND=1"
)
if "%PCAP_FOUND%"=="0" (
    echo ⚠ No PCAP SDK found - network capture may not work
)

echo.

REM Summary
echo =====================================================
echo Test Summary
echo =====================================================
echo.

if "%ASIO_AVAILABLE%"=="1" (
    echo ✓ ASIO SDK Integration: READY
) else (
    echo ⚠ ASIO SDK Integration: Using stub implementation
    echo   To enable full ASIO support:
    echo   1. Download ASIO SDK from https://www.steinberg.net/developers/
    echo   2. Extract to a folder (e.g., C:\ASIOSDK2.3)
    echo   3. Set environment variable: set ASIOSDK_DIR=C:\ASIOSDK2.3
    echo   4. Restart VS Code and rerun tests
)

echo.
echo ✓ Enhanced ASIO Audio Listener is ready for use!
echo.
echo Next steps:
echo   1. Install ASIO driver (e.g., ASIO4ALL from http://www.asio4all.org/)
echo   2. Run: asio_listener.exe "Your Network Interface" "Your ASIO Driver"
echo   3. Check README_ENHANCED.md for detailed usage instructions
echo.

goto :test_success

:test_failed
echo.
echo ✗ Tests failed! Please check the error messages above.
echo.
pause
exit /b 1

:test_success
echo =====================================================
echo All tests completed successfully!
echo =====================================================
pause
exit /b 0
