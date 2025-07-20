@echo off
echo =============================================================================
echo Intel Raw AVDECC Entity Test (Layer 2 Protocol)
echo =============================================================================
echo Entity ID: 0xC047E0FFFE167B89
echo Protocol: IEEE 1722.1-2021 AVDECC (EtherType 0x22F0)
echo Target: AVDECC Multicast MAC 91:e0:f0:01:00:00
echo Hardware: Intel I219-LM with hardware timestamping
echo =============================================================================
echo.

echo Starting Intel Raw AVDECC Entity for 15 seconds...
echo.
echo Expected output:
echo - Find Intel I219-LM interface
echo - Hardware timestamping enabled
echo - ENTITY_AVAILABLE broadcasts every 2 seconds
echo - Proper Layer 2 AVDECC packets
echo.
echo IMPORTANT: This should be visible in Wireshark on:
echo - EtherType 0x22F0 (AVDECC)
echo - Destination MAC: 91:e0:f0:01:00:00
echo - Source MAC: c0:47:0e:16:7b:89
echo.

cd /d "d:\Repos\OpenAvnu\build\bin\Release"

:: Start the entity in background
start "Intel Raw AVDECC Entity" /MIN intel_raw_avdecc_entity.exe

:: Wait 15 seconds
timeout /t 15 /nobreak

:: Stop the entity
taskkill /F /IM intel_raw_avdecc_entity.exe >nul 2>&1

echo.
echo =============================================================================
echo Test completed!
echo.
echo Check Wireshark for AVDECC packets with:
echo - Filter: eth.type == 0x22f0
echo - Destination: 91:e0:f0:01:00:00
echo - Source: c0:47:0e:16:7b:89
echo.
echo Check Hive Controller for discovered entity:
echo - Entity ID: C047E0FFFE167B89
echo - Intel I219-LM AVDECC Entity
echo =============================================================================
pause
