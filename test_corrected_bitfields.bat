@echo off
echo Testing corrected IEEE 1722.1 bit field implementation...
echo.

cd /d "d:\Repos\OpenAvnu"

echo Starting AVDECC entity for 15 seconds to test bit field fixes...
echo Look for:
echo   - Valid time = 5 (not 0)
echo   - Entity ID = 0xC047E0FFFE167B89 (not scrambled)
echo   - Proper packet structure sizes
echo.

timeout /t 15 | ".\build\bin\Release\intel_pcap_avdecc_entity.exe"

echo.
echo Test completed - Check Wireshark for:
echo   1. Valid Time: 5 (was 0)
echo   2. Entity ID: 0xC047E0FFFE167B89 (was scrambled)
echo   3. Control Data Length: 60 (was wrong)
echo   4. Packet size: 78 bytes total
pause
