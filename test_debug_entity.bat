@echo off
echo Testing AVDECC Entity Debug Output...
echo.

cd /d "d:\Repos\OpenAvnu"

echo Starting AVDECC entity for 10 seconds...
timeout /t 10 | ".\build\bin\Release\intel_pcap_avdecc_entity.exe"

echo.
echo Test completed - Check debug output above for structure alignment and byte order issues
pause
