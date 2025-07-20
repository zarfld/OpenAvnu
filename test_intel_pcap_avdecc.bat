@echo off
echo =============================================================================
echo Intel PCAP AVDECC Entity Test (Proper Layer 2 Protocol)
echo =============================================================================
echo Entity ID: 0xC047E0FFFE167B89
echo Protocol: IEEE 1722.1-2021 AVDECC (EtherType 0x22F0)
echo Target: AVDECC Multicast MAC 91:e0:f0:01:00:00
echo Hardware: Intel I219-LM with hardware timestamping
echo Method: PCAP Layer 2 packet injection
echo =============================================================================
echo.

echo Starting Intel PCAP AVDECC Entity for 20 seconds...
echo.
echo Expected output:
echo - Find Intel I219-LM interface via PCAP
echo - Hardware timestamping enabled
echo - ENTITY_AVAILABLE broadcasts every 2 seconds via Layer 2
echo - Proper AVDECC packets with EtherType 0x22F0
echo.
echo CRITICAL: This should NOW be visible in Wireshark as:
echo - EtherType 0x22F0 (AVDECC) - NOT IPv4!
echo - Destination MAC: 91:e0:f0:01:00:00
echo - Source MAC: c0:47:0e:16:7b:89
echo - Protocol: IEEE 1722.1 AVDECC
echo.
echo CRITICAL: This should NOW be discoverable by Hive Controller!
echo.

cd /d "d:\Repos\OpenAvnu\build\bin\Release"

:: Start the entity in background
start "Intel PCAP AVDECC Entity" /MIN intel_pcap_avdecc_entity.exe

:: Wait 20 seconds for multiple broadcasts
timeout /t 20 /nobreak

:: Stop the entity
taskkill /F /IM intel_pcap_avdecc_entity.exe >nul 2>&1

echo.
echo =============================================================================
echo Test completed!
echo.
echo NOW check Wireshark for PROPER AVDECC packets:
echo - Filter: eth.type == 0x22f0
echo - Should see IEEE 1722.1 AVDECC packets
echo - NOT IPv4 multicast anymore!
echo.
echo NOW check Hive Controller:
echo - Should discover Entity ID: C047E0FFFE167B89
echo - Intel I219-LM AVDECC Entity
echo - With proper AVDECC protocol compliance
echo =============================================================================
pause
