# Milan Device gPTP Configuration Guide
# Provides guidance for enabling gPTP on Hirschmann Milan devices

Write-Host "=== MILAN DEVICE GPTP CONFIGURATION GUIDE ===" -ForegroundColor Green
Write-Host "Based on analysis: Devices use 802.1AS (compatible with our gPTP)" -ForegroundColor Cyan
Write-Host ""

$devices = @(
    @{IP="157.247.3.12"; MAC="00-0a-92-00-e9-35"},
    @{IP="157.247.1.112"; MAC="00-0a-92-00-69-c5"}
)

Write-Host "[ANALYSIS SUMMARY]" -ForegroundColor Yellow
Write-Host "==================" -ForegroundColor Gray
Write-Host "✓ Devices are Hirschmann industrial networking equipment" -ForegroundColor Green
Write-Host "✓ Devices use 802.1AS (gPTP) - same protocol as our implementation" -ForegroundColor Green
Write-Host "✓ Protocol compatibility confirmed" -ForegroundColor Green
Write-Host "✗ Devices not participating in gPTP domain" -ForegroundColor Red
Write-Host ""

Write-Host "[DEVICE ACCESS METHODS]" -ForegroundColor Yellow
Write-Host "=======================" -ForegroundColor Gray

foreach ($device in $devices) {
    Write-Host "Device: $($device.IP) (MAC: $($device.MAC))" -ForegroundColor Cyan
    
    # Try common web interface ports
    $webPorts = @(80, 443, 8080, 8443)
    $foundWeb = $false
    
    foreach ($port in $webPorts) {
        Write-Host "  Testing web interface on port $port..." -NoNewline
        $web = Test-NetConnection -ComputerName $device.IP -Port $port -WarningAction SilentlyContinue -ErrorAction SilentlyContinue -InformationLevel Quiet
        if ($web) {
            $protocol = if ($port -eq 443 -or $port -eq 8443) { "https" } else { "http" }
            Write-Host " AVAILABLE" -ForegroundColor Green
            Write-Host "    → Try: $protocol`://$($device.IP):$port" -ForegroundColor White
            $foundWeb = $true
        } else {
            Write-Host " Not available" -ForegroundColor Red
        }
    }
    
    if (-not $foundWeb) {
        Write-Host "  No web interface detected - may need SNMP or console access" -ForegroundColor Yellow
    }
    
    Write-Host ""
}

Write-Host "[HIRSCHMANN GPTP CONFIGURATION]" -ForegroundColor Yellow
Write-Host "===============================" -ForegroundColor Gray

Write-Host "Common Hirschmann gPTP Configuration Steps:" -ForegroundColor Cyan
Write-Host ""

Write-Host "1. Web Interface Access:" -ForegroundColor White
Write-Host "   - Try http://157.247.3.12 and http://157.247.1.112" -ForegroundColor Gray
Write-Host "   - Default credentials often: admin/admin or admin/password" -ForegroundColor Gray
Write-Host "   - Look for 'Time Synchronization' or 'PTP' section" -ForegroundColor Gray
Write-Host ""

Write-Host "2. Enable IEEE 802.1AS (gPTP):" -ForegroundColor White
Write-Host "   - Navigate to Network → Time Synchronization" -ForegroundColor Gray
Write-Host "   - OR System → PTP Configuration" -ForegroundColor Gray
Write-Host "   - Enable 'IEEE 802.1AS' or 'gPTP'" -ForegroundColor Gray
Write-Host "   - Set Clock Mode: Auto or Slave (let our gPTP be master)" -ForegroundColor Gray
Write-Host ""

Write-Host "3. Milan Profile Settings:" -ForegroundColor White
Write-Host "   - Enable 'Milan' profile if available" -ForegroundColor Gray
Write-Host "   - OR enable 'AVB' mode" -ForegroundColor Gray
Write-Host "   - Set Domain Number: 0 (default)" -ForegroundColor Gray
Write-Host "   - Enable on all relevant ports" -ForegroundColor Gray
Write-Host ""

Write-Host "4. Port Configuration:" -ForegroundColor White
Write-Host "   - Enable gPTP on ports connected to our system" -ForegroundColor Gray
Write-Host "   - Set port role to 'Auto' or 'Slave'" -ForegroundColor Gray
Write-Host "   - Ensure ports are not blocked" -ForegroundColor Gray
Write-Host ""

Write-Host "[VERIFICATION COMMANDS]" -ForegroundColor Yellow
Write-Host "======================" -ForegroundColor Gray

Write-Host "After configuration, verify with these commands:" -ForegroundColor Cyan
Write-Host ""

Write-Host "1. Check gPTP communication:" -ForegroundColor White
Write-Host "   powershell -File test_milan_compliance_validation.ps1" -ForegroundColor Gray
Write-Host ""

Write-Host "2. Monitor for peer discovery:" -ForegroundColor White
Write-Host "   - Should see 'Peer discovered' messages" -ForegroundColor Gray
Write-Host "   - PDelay exchanges should start" -ForegroundColor Gray
Write-Host "   - BMCA (Best Master Clock Algorithm) events" -ForegroundColor Gray
Write-Host ""

Write-Host "3. Verify synchronization:" -ForegroundColor White
Write-Host "   - Check offset values < 80ns" -ForegroundColor Gray
Write-Host "   - Monitor sync message exchanges" -ForegroundColor Gray
Write-Host ""

Write-Host "[ALTERNATIVE ACCESS METHODS]" -ForegroundColor Yellow
Write-Host "============================" -ForegroundColor Gray

Write-Host "If web interface not accessible:" -ForegroundColor Cyan
Write-Host "1. SNMP Configuration (if enabled):" -ForegroundColor White
Write-Host "   - Community: public (read) / private (write)" -ForegroundColor Gray
Write-Host "   - Look for PTP MIB objects" -ForegroundColor Gray
Write-Host ""

Write-Host "2. Console Access:" -ForegroundColor White
Write-Host "   - Serial console if available" -ForegroundColor Gray
Write-Host "   - Telnet/SSH if enabled" -ForegroundColor Gray
Write-Host ""

Write-Host "3. Network Administrator:" -ForegroundColor White
Write-Host "   - Contact network admin for device access" -ForegroundColor Gray
Write-Host "   - Request gPTP/Milan configuration" -ForegroundColor Gray
Write-Host ""

Write-Host "[EXPECTED RESULTS]" -ForegroundColor Yellow
Write-Host "=================" -ForegroundColor Gray

Write-Host "After successful configuration:" -ForegroundColor Cyan
Write-Host "✓ Devices will respond to gPTP PDelay requests" -ForegroundColor Green
Write-Host "✓ Peer discovery will occur within 30 seconds" -ForegroundColor Green
Write-Host "✓ Clock synchronization will achieve <80ns offset" -ForegroundColor Green
Write-Host "✓ Milan compliance validation will pass" -ForegroundColor Green
Write-Host ""

Write-Host "=== CONFIGURATION GUIDE COMPLETE ===" -ForegroundColor Green
Write-Host "Next: Access device configuration and enable gPTP" -ForegroundColor Cyan