@echo off
REM ============================================================================
REM Intel I219 Network Adapter Detection and Configuration Test
REM ============================================================================

echo [INFO] Intel I219 Network Interface Detection Test
echo [INFO] =============================================

echo.
echo [STEP 1] Detecting Intel I219 network adapters...

powershell -Command "Write-Host 'Available Intel Network Adapters:' -ForegroundColor Cyan; Get-NetAdapter | Where-Object { $_.InterfaceDescription -like '*Intel*' } | Select-Object Name, InterfaceDescription, Status, LinkSpeed | Format-Table -AutoSize"

echo.
echo [STEP 2] Specifically looking for I219 adapters...

powershell -Command "Write-Host 'Intel I219 Adapters:' -ForegroundColor Green; $i219 = Get-NetAdapter | Where-Object { $_.InterfaceDescription -like '*Intel*' -and $_.InterfaceDescription -like '*I219*' }; if ($i219) { $i219 | Select-Object Name, InterfaceDescription, Status, LinkSpeed | Format-Table -AutoSize } else { Write-Host 'No Intel I219 adapters found' -ForegroundColor Yellow }"

echo.
echo [STEP 3] Testing auto-detection with our working script...

cd /d "%~dp0"
powershell -ExecutionPolicy Bypass -File "debug_i219_final.ps1"

echo.
echo [STEP 4] Testing simplified auto-detection...

powershell -ExecutionPolicy Bypass -File "final_i219_test.ps1"

echo.
echo [STEP 5] Network interface capabilities check...

powershell -Command "$adapter = Get-NetAdapter | Where-Object { $_.InterfaceDescription -like '*Intel*' -and $_.Status -eq 'Up' } | Select-Object -First 1; if ($adapter) { Write-Host 'Testing adapter:' $adapter.Name -ForegroundColor Green; Write-Host 'Description:' $adapter.InterfaceDescription; Write-Host 'Status:' $adapter.Status; Write-Host 'Link Speed:' $adapter.LinkSpeed; $adapterAdvanced = Get-NetAdapterAdvancedProperty -Name $adapter.Name -ErrorAction SilentlyContinue; if ($adapterAdvanced) { Write-Host 'Advanced Properties available for configuration' -ForegroundColor Green } else { Write-Host 'Limited advanced properties' -ForegroundColor Yellow } } else { Write-Host 'No suitable Intel adapter found' -ForegroundColor Red }"

echo.
echo [INFO] I219 Detection Test Complete!
echo [INFO] The script should automatically detect and use your Intel I219 adapter.
echo.
pause
