@echo off
echo ============================================
echo    HIVE-AVDECC DISCOVERY VERIFICATION
echo    REAL SUCCESS TEST - NO ASSUMPTIONS
echo ============================================
echo.
echo WARNING: Previous validation was NOT real success!
echo Only Hive-AVDECC discovery proves actual functionality.
echo.
echo SETUP REQUIREMENTS:
echo 1. Power ON RME audio interface
echo 2. Connect Intel I219 (C0:47:0E:16:7B:89) to RME network
echo 3. Start Hive-AVDECC on same network segment
echo 4. Run this test
echo.
echo REAL SUCCESS CRITERIA:
echo - OpenAvnu entity appears in Hive-AVDECC device list
echo - Entity ID: 0x001B21481B2F193D visible
echo - MILAN compliance detected by Hive
echo.
pause
echo.
echo Starting OpenAvnu Raw Ethernet AVDECC Entity...
echo Duration: 60 seconds for proper discovery
echo.
cd build\examples\Release
.\raw_ethernet_avdecc_entity.exe 60
echo.
echo ============================================
echo TEST COMPLETE - CHECK HIVE-AVDECC NOW!
echo ============================================
echo.
echo VERIFY IN HIVE-AVDECC:
echo 1. Look for device with Entity ID: 0x001B21481B2F193D
echo 2. Verify MAC address: C0:47:0E:16:7B:89
echo 3. Check MILAN compliance indication
echo 4. Verify stream capabilities visible
echo.
echo RESULT:
set /p result="Did OpenAvnu entity appear in Hive-AVDECC? (y/n): "
if /i "%result%"=="y" (
    echo ✅ SUCCESS: Hive-AVDECC discovery confirmed!
    echo OpenAvnu Raw Ethernet AVDECC integration working!
) else (
    echo ❌ FAILURE: Entity not discovered by Hive-AVDECC
    echo Investigation required - check network and implementation
)
echo.
pause
