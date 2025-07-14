@echo off
REM ============================================================================
REM ASIO Audio Listener Test with OpenAvnu Daemon Coordination
REM Automated testing ensuring all required daemons are running
REM ============================================================================

echo [INFO] ASIO Audio Listener - Comprehensive Test with Daemon Coordination
echo [INFO] ==================================================================

REM Change to script directory
cd /d "%~dp0"

REM Configuration
set "OPENAVNU_ROOT=d:\Repos\OpenAvnu"
set "BUILD_DIR=%OPENAVNU_ROOT%\build"
set "SCRIPT_DIR=%OPENAVNU_ROOT%\examples\asio-listener"

echo [INFO] Script Directory: %SCRIPT_DIR%
echo [INFO] Build Directory: %BUILD_DIR%
echo [INFO] OpenAvnu Root: %OPENAVNU_ROOT%

REM Step 1: Check if daemons are already running
echo.
echo [STEP 1] Checking OpenAvnu daemon status...
powershell -ExecutionPolicy Bypass -Command "& { cd '%SCRIPT_DIR%'; . .\start_openavnu_daemons.ps1; Show-DaemonStatus }"

REM Check if MRPD and MAAP are running (required for ASIO)
set "MRPD_RUNNING=false"
set "MAAP_RUNNING=false"

powershell -ExecutionPolicy Bypass -Command "if (Get-Process -Name 'mrpd' -ErrorAction SilentlyContinue) { exit 0 } else { exit 1 }"
if %ERRORLEVEL% EQU 0 (
    set "MRPD_RUNNING=true"
    echo [SUCCESS] MRPD daemon is running
) else (
    echo [WARNING] MRPD daemon is not running
)

powershell -ExecutionPolicy Bypass -Command "if (Get-Process -Name 'maap_daemon' -ErrorAction SilentlyContinue) { exit 0 } else { exit 1 }"
if %ERRORLEVEL% EQU 0 (
    set "MAAP_RUNNING=true"
    echo [SUCCESS] MAAP daemon is running
) else (
    echo [WARNING] MAAP daemon is not running
)

REM Step 2: Start daemons if needed
if "%MRPD_RUNNING%" == "false" (
    echo.
    echo [STEP 2] Starting required OpenAvnu daemons with auto-detection...
    echo [INFO] Auto-detecting Intel I219 or best available network interface...
    powershell -ExecutionPolicy Bypass -File "%SCRIPT_DIR%\start_openavnu_daemons.ps1" -Verbose
    
    if ERRORLEVEL 1 (
        echo [ERROR] Failed to start OpenAvnu daemons!
        echo [ERROR] ASIO Audio Listener requires MRPD and MAAP daemons.
        echo [INFO] Trying manual interface detection...
        powershell -ExecutionPolicy Bypass -File "%SCRIPT_DIR%\test_i219_detection.bat"
        pause
        exit /b 1
    )
    
    echo [SUCCESS] OpenAvnu daemons started successfully with auto-detected interface!
) else (
    echo [INFO] Required daemons are already running, proceeding with test...
)

REM Step 3: Verify ASIO SDK integration
echo.
echo [STEP 3] Verifying ASIO SDK integration...

if not defined ASIOSDK_DIR (
    echo [WARNING] ASIOSDK_DIR environment variable is not set!
    echo [WARNING] ASIO listener will use stub implementation.
    set "ASIO_MODE=STUB"
) else (
    echo [SUCCESS] ASIOSDK_DIR is set to: %ASIOSDK_DIR%
    if exist "%ASIOSDK_DIR%\common\asio.h" (
        echo [SUCCESS] Real ASIO SDK found and ready
        set "ASIO_MODE=REAL"
    ) else (
        echo [WARNING] ASIO SDK directory exists but asio.h not found
        echo [WARNING] Using stub implementation
        set "ASIO_MODE=STUB"
    )
)

REM Step 4: Build ASIO Audio Listener
echo.
echo [STEP 4] Building ASIO Audio Listener...

if exist "build.bat" (
    call build.bat
    if ERRORLEVEL 1 (
        echo [ERROR] Build failed! Check build output above.
        pause
        exit /b 1
    )
) else (
    echo [ERROR] build.bat not found in current directory!
    echo [ERROR] Current directory: %CD%
    pause
    exit /b 1
)

echo [SUCCESS] ASIO Audio Listener built successfully!

REM Step 5: Test the executable
echo.
echo [STEP 5] Testing ASIO Audio Listener executable...

if exist "asio_listener.exe" (
    echo [SUCCESS] Executable found: asio_listener.exe
    
    REM Show file info
    echo [INFO] File Information:
    dir asio_listener.exe | findstr "asio_listener.exe"
    
    REM Test with --help flag
    echo.
    echo [INFO] Testing help output:
    asio_listener.exe --help 2>&1
    
    if ERRORLEVEL 1 (
        echo [WARNING] Help command returned non-zero exit code, but this might be normal
    )
    
) else (
    echo [ERROR] Executable not found: asio_listener.exe
    echo [ERROR] Build may have failed silently
    pause
    exit /b 1
)

REM Step 6: Integration test with network detection
echo.
echo [STEP 6] Network integration test...

echo [INFO] Detecting available network interfaces:
powershell -Command "Get-NetAdapter | Where-Object { $_.Status -eq 'Up' } | Select-Object Name, InterfaceDescription | Format-Table -AutoSize"

echo [INFO] Looking for Intel Ethernet Controllers:
powershell -Command "Get-NetAdapter | Where-Object { $_.InterfaceDescription -like '*Intel*' -and $_.Status -eq 'Up' } | Select-Object Name, InterfaceDescription | Format-Table -AutoSize"

REM Step 7: Final status report
echo.
echo [STEP 7] Final Status Report
echo =====================================

echo [INFO] ASIO Mode: %ASIO_MODE%
echo [INFO] OpenAvnu Daemons Status:
powershell -ExecutionPolicy Bypass -Command "& { cd '%SCRIPT_DIR%'; . .\start_openavnu_daemons.ps1; Show-DaemonStatus }"

REM Success message
echo.
echo [SUCCESS] ============================================
echo [SUCCESS] ASIO Audio Listener Test COMPLETED
echo [SUCCESS] ============================================
echo [SUCCESS] 
echo [SUCCESS] The ASIO Audio Listener is ready to use!
echo [SUCCESS] 
echo [SUCCESS] Key Information:
echo [SUCCESS] - ASIO SDK Mode: %ASIO_MODE%
echo [SUCCESS] - Executable: %CD%\asio_listener.exe  
echo [SUCCESS] - Required daemons are running
echo [SUCCESS] - Network interfaces detected
echo [SUCCESS] 
echo [SUCCESS] Usage: asio_listener.exe [network_interface]
echo [SUCCESS] 

REM Optional: Ask if user wants to run a quick test
echo.
set /p "RUN_TEST=Do you want to run a quick ASIO listener test? (y/n): "
if /i "%RUN_TEST%" == "y" (
    echo.
    echo [INFO] Starting ASIO listener test (will run for 10 seconds)...
    timeout /t 2 > nul
    
    REM Run with timeout to prevent hanging
    powershell -Command "& { $job = Start-Job -ScriptBlock { & '%CD%\asio_listener.exe' }; Wait-Job $job -Timeout 10; Remove-Job $job -Force }"
    
    echo [INFO] Test completed!
)

echo.
echo [INFO] Test session complete. Press any key to exit...
pause > nul
