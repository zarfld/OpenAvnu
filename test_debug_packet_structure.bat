@echo off
echo =============================================================================
echo Intel PCAP AVDECC Entity - Packet Structure Debug Test
echo =============================================================================
echo Testing improved packet structure with detailed debug output
echo Looking for fixes to "Not enough data in buffer" error
echo =============================================================================
echo.

cd /d "d:\Repos\OpenAvnu\build\bin\Release"

echo Starting entity for 8 seconds to capture debug output...
echo.

:: Start the entity and capture output
powershell -Command "$proc = Start-Process -FilePath 'intel_pcap_avdecc_entity.exe' -PassThru -WindowStyle Hidden; Start-Sleep -Seconds 8; Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue"

echo.
echo =============================================================================
echo Test completed! 
echo.
echo Check the debug output above for:
echo 1. Raw Entity ID bytes 
echo 2. Packet structure sizes
echo 3. AVTP control data length
echo.
echo Then check Wireshark and Hive for improvements!
echo =============================================================================
pause
