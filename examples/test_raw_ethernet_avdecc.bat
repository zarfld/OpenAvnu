@echo off
echo.
echo =====================================
echo  OpenAvnu Raw Ethernet AVDECC Test
echo =====================================
echo.

REM Check if running as Administrator
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo ❌ ERROR: This script must be run as Administrator
    echo.
    echo Raw Ethernet access requires administrator privileges.
    echo Right-click on this batch file and select "Run as administrator"
    echo.
    pause
    exit /b 1
)

echo ✅ Running with Administrator privileges

REM Check if the executable exists
if exist "raw_ethernet_avdecc_entity.exe" (
    set ENTITY_EXE=raw_ethernet_avdecc_entity.exe
    echo ✅ Found local raw_ethernet_avdecc_entity.exe
) else if exist "..\build\examples\Release\raw_ethernet_avdecc_entity.exe" (
    set ENTITY_EXE=..\build\examples\Release\raw_ethernet_avdecc_entity.exe
    echo ✅ Found CMake build raw_ethernet_avdecc_entity.exe
) else (
    echo ❌ ERROR: raw_ethernet_avdecc_entity.exe not found
    echo.
    echo Please build the entity first using:
    echo    build_raw_ethernet_avdecc.bat
    echo    OR
    echo    cmake --build ..\build --target raw_ethernet_avdecc_entity --config Release
    echo.
    pause
    exit /b 1
)

REM Check for Npcap installation
echo.
echo 🔍 Checking for Npcap installation...

sc query npcap >nul 2>&1
if %errorlevel% == 0 (
    echo ✅ Npcap service found and running
) else (
    echo ⚠️  Npcap service not found
    echo.
    echo For production AVDECC compatibility, install Npcap:
    echo    1. Download from: https://npcap.com/
    echo    2. Install with "WinPcap API-compatible Mode" enabled
    echo    3. Restart this test
    echo.
    echo Continuing with available build...
    echo.
)

REM Run the entity
echo.
echo 🚀 Starting OpenAvnu Raw Ethernet AVDECC Entity...
echo.
echo This will:
echo   1. Send ADP Entity Available advertisements every 2 seconds
echo   2. Listen for AECP READ_DESCRIPTOR commands from Hive
echo   3. Respond with Entity and Configuration descriptors
echo.
echo ⏱️  Running for 60 seconds (use Ctrl+C to stop early)
echo.
echo 📋 Testing Steps:
echo   1. Start Hive-AVDECC on the same machine or network
echo   2. Select the same network interface in Hive settings
echo   3. Look for "OpenAvnu AVDECC Entity" in Hive's device list
echo   4. Try to read entity information in Hive
echo.
echo 🎯 Expected Results:
echo   - ADP advertisements sent every 2 seconds
echo   - Hive discovers and lists the entity
echo   - Entity and Configuration descriptors readable
echo.

pause

echo Starting entity (60 seconds)...
echo.

"%ENTITY_EXE%" --duration 60

echo.
echo 🏁 Test completed!
echo.
echo 📊 Results Analysis:
echo   - If you saw "Raw Ethernet TX" messages: Packets were sent
echo   - If you saw "AECP Command" messages: Hive tried to read descriptors
echo   - If entity appeared in Hive: SUCCESS - Professional tool compatibility achieved!
echo.
echo � Troubleshooting:
echo   - No TX messages: Check Npcap installation and administrator privileges
echo   - TX but no Hive discovery: Check interface selection in Hive settings
echo   - Discovery but no descriptors: Check firewall or network configuration
echo.
echo   - If discoverable by Hive-AVDECC: ✅ Professional tool compatibility achieved
echo   - If ADP advertisements sent: ✅ IEEE 1722.1 protocol compliance confirmed
echo.

pause
