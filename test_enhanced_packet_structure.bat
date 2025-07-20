@echo off
echo === Enhanced Intel PCAP AVDECC Entity Test ===
echo.
echo This test will run the enhanced entity with improved packet structure
echo and detailed debug output to verify all field encodings are correct.
echo.
echo Key improvements:
echo - Fixed Entity Model ID encoding (was 0x0000000000000000)
echo - Fixed all capability field encodings
echo - Enhanced debug output with verification
echo - gPTP Grandmaster ID properly set
echo.
echo Press any key to start the enhanced entity...
pause > nul

cd "D:\Repos\OpenAvnu"
.\build\bin\Release\intel_pcap_avdecc_entity.exe

echo.
echo Test completed. Check the debug output above for field verification.
pause
