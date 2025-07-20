@echo off
echo === Testing CORRECTED Intel PCAP AVDECC Entity ===
echo.
echo This test will run the entity with CORRECTED byte order conversion.
echo.
echo Expected fixes:
echo - Entity ID: Should be 0xC047E0FFFE167B89 (not scrambled)
echo - Entity Model ID: Should be 0xC047E0000001234 (not scrambled)  
echo - Valid Time: Should be 5 (not 0)
echo - Packet size: Should be 86 bytes (not 78)
echo.
echo The debug output will show before/after values to verify the fix.
echo.
echo Press any key to start the corrected entity...
pause > nul

cd "D:\Repos\OpenAvnu"
echo.
echo Starting enhanced entity with corrected byte order conversion...
echo.
.\build\bin\Release\intel_pcap_avdecc_entity.exe

echo.
echo Test completed. Check Wireshark to verify the packet structure is now correct.
echo Expected to see:
echo - Entity ID: 0xC047E0FFFE167B89
echo - Entity Model ID: 0xC047E0000001234
echo - Valid Time: 5
echo - All capability fields properly populated
pause
