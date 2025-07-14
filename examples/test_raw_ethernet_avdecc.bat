@echo off
echo ===========================================
echo   OpenAvnu Raw Ethernet AVDECC Entity Test
echo   Critical Path Solution for Hive-AVDECC
echo ===========================================
echo.

echo 🎯 Testing Raw Ethernet AVDECC Entity...
echo.

echo 📋 Prerequisites Check:
echo   1. Npcap installed and running
echo   2. Administrator privileges for Raw Ethernet access
echo   3. Network interface available for AVDECC
echo.

echo ⚠️  IMPORTANT: Run as Administrator for Raw Ethernet access
echo.

if not exist "..\build\examples\Release\raw_ethernet_avdecc_entity.exe" (
    echo ❌ Raw Ethernet AVDECC Entity not found!
    echo    Please build using: cmake --build ..\build --target raw_ethernet_avdecc_entity --config Release
    pause
    exit /b 1
)

echo ✅ Raw Ethernet AVDECC Entity found
echo.

echo 🚀 Starting Raw Ethernet AVDECC Entity...
echo.
echo 💡 This entity:
echo    - Uses Real Raw Ethernet (not UDP simulation)
echo    - Sends proper IEEE 1722.1 AVDECC frames
echo    - Should be discoverable by Hive-AVDECC
echo    - Implements MILAN-compliant entity capabilities
echo.

echo Press Ctrl+C to stop the entity when testing is complete.
echo.

REM Start the Raw Ethernet AVDECC entity
"..\build\examples\Release\raw_ethernet_avdecc_entity.exe"

echo.
echo 👋 Raw Ethernet AVDECC Entity stopped
echo.

echo 📊 Testing Results:
echo   - If entity started successfully: ✅ Raw Ethernet transport working
echo   - If discoverable by Hive-AVDECC: ✅ Professional tool compatibility achieved
echo   - If ADP advertisements sent: ✅ IEEE 1722.1 protocol compliance confirmed
echo.

pause
