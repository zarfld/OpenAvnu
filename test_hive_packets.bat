@echo off
echo ===============================================
echo   AVDECC PACKET VERIFICATION TEST
echo   Verify Hive can see the same packets as entity
echo ===============================================
echo.

echo Step 1: Check if Wireshark is available...
where wireshark.exe >nul 2>&1
if %errorlevel% neq 0 (
    echo WARNING: Wireshark not found in PATH
    echo Install Wireshark to capture AVDECC packets
    echo.
)

echo Step 2: Network Interface Information
echo Target Interface GUID: 3DC822E6-8C68-424C-9798-63CFBF52994B
powershell -Command "Get-NetAdapter | Where-Object { $_.InterfaceGuid -eq '3DC822E6-8C68-424C-9798-63CFBF52994B' } | Format-Table Name, InterfaceDescription, Status, LinkSpeed"

echo.
echo Step 3: Instructions for Hive Configuration
echo ==========================================
echo 1. Start Hive-AVDECC as Administrator
echo 2. Go to Hive Settings/Preferences
echo 3. Select Network Interface: Intel(R) Ethernet Connection (22) I219-LM
echo 4. Click Apply/OK
echo 5. Click Refresh in main Hive window
echo.

echo Step 4: Wireshark Packet Capture Instructions
echo ============================================
echo 1. Start Wireshark as Administrator
echo 2. Select interface: Intel(R) Ethernet Connection (22) I219-LM
echo 3. Start capture
echo 4. Use filter: eth.type == 0x22f0
echo 5. You should see packets with destination MAC: 91:e0:f0:01:00:00
echo 6. Packets should appear every 2 seconds from the entity
echo.

echo Step 5: Running entity for 60 seconds...
echo Press Ctrl+C to stop early
echo.

echo Starting entity in 3 seconds...
timeout /t 3 >nul

echo [ENTITY STARTING]
.\build\examples\Release\raw_ethernet_avdecc_entity.exe --interface "3DC822E6-8C68-424C-9798-63CFBF52994B" --duration 60

echo.
echo ===============================================
echo   TEST COMPLETE
echo ===============================================
echo.
echo Next steps:
echo 1. Check if Wireshark captured AVDECC packets
echo 2. Verify Hive is configured for same interface
echo 3. Look for "OpenAvnu AVDECC Entity" in Hive device list
echo.
echo If Wireshark sees packets but Hive doesn't:
echo - Interface selection problem in Hive
echo - Try running Hive and entity on separate machines
echo - Check Hive permissions (run as Administrator)
echo.
pause
