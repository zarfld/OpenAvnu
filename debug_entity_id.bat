@echo off
echo Testing Entity ID byte order conversion debugging...
echo.

cd /d "d:\Repos\OpenAvnu"

echo Starting AVDECC entity for 10 seconds to debug byte order conversion...
echo Look for Entity ID conversion debugging info
echo.

timeout /t 10 | ".\build\bin\Release\intel_pcap_avdecc_entity.exe" 2>&1 | findstr /i "DEBUG.*Entity.*ID\|Input.*bytes\|Output.*bytes\|htonll"

echo.
echo Debug output captured above
pause
