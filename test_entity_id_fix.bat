@echo off
echo Testing Windows byte swap Entity ID fix...
echo.

cd /d "d:\Repos\OpenAvnu"

echo Starting AVDECC entity for 15 seconds...
echo Expected Entity ID in Hive: 0xC047E0FFFE167B89 (not 0x00000000c047e0ff)
echo Expected Entity Model ID: 0xC047E00000001234 (not 0xfe167b890c047e00)
echo.

timeout /t 15 | ".\build\bin\Release\intel_pcap_avdecc_entity.exe"

echo.
echo Test completed! 
echo Check Hive Controller to see if Entity ID is now correct:
echo   - Should show 0xC047E0FFFE167B89
echo   - Should show green/healthy status (not error state)
echo   - Available Index should be stable (no more "incoherently changed")
pause
