# PTP Protocol Variant Detection Script
# Detects what type of PTP the Milan devices are actually using

Write-Host "=== PTP PROTOCOL VARIANT DETECTION ===" -ForegroundColor Green
Write-Host "User confirmed devices ARE using PTP - determining variant" -ForegroundColor Cyan
Write-Host ""

$milanDevices = @("157.247.3.12", "157.247.1.112")

Write-Host "[LOCAL GPTP STATUS]" -ForegroundColor Yellow
Write-Host "==================" -ForegroundColor Gray

# Check what our gPTP is doing
Write-Host "Our gPTP Configuration:" -ForegroundColor Cyan
Write-Host "  Protocol: IEEE 802.1AS (gPTP)" -ForegroundColor White
Write-Host "  Transport: Layer 2 Ethernet" -ForegroundColor White
Write-Host "  Multicast: 01-1B-19-00-00-00" -ForegroundColor White
Write-Host "  UDP Ports: 319 (Event), 320 (General)" -ForegroundColor White

# Check if gPTP is listening
$gptpListening = netstat -an | Select-String "319|320"
if ($gptpListening) {
    Write-Host "✓ gPTP listening on UDP ports:" -ForegroundColor Green
    $gptpListening | ForEach-Object { Write-Host "    $_" -ForegroundColor White }
} else {
    Write-Host "✗ gPTP not listening on expected ports" -ForegroundColor Red
}

Write-Host ""

foreach ($device in $milanDevices) {
    Write-Host "[DEVICE] $device - PTP VARIANT ANALYSIS" -ForegroundColor Yellow
    Write-Host "==============================================" -ForegroundColor Gray
    
    # Basic connectivity
    Write-Host "Network connectivity:" -NoNewline
    $ping = Test-Connection -ComputerName $device -Count 1 -Quiet -ErrorAction SilentlyContinue
    if ($ping) {
        Write-Host " REACHABLE" -ForegroundColor Green
    } else {
        Write-Host " NOT REACHABLE" -ForegroundColor Red
        continue
    }
    
    Write-Host ""
    Write-Host "PTP Variant Detection:" -ForegroundColor Cyan
    
    # Test 1: IEEE 1588v2 over UDP (Layer 3)
    Write-Host "  1. IEEE 1588v2 over UDP (Layer 3):" -ForegroundColor White
    $ptp319 = Test-NetConnection -ComputerName $device -Port 319 -WarningAction SilentlyContinue -ErrorAction SilentlyContinue -InformationLevel Quiet
    $ptp320 = Test-NetConnection -ComputerName $device -Port 320 -WarningAction SilentlyContinue -ErrorAction SilentlyContinue -InformationLevel Quiet
    
    if ($ptp319 -or $ptp320) {
        Write-Host "     ✓ DETECTED - Device responds to PTP UDP ports" -ForegroundColor Green
        if ($ptp319) { Write-Host "       - Port 319 (Event): OPEN" -ForegroundColor Green }
        if ($ptp320) { Write-Host "       - Port 320 (General): OPEN" -ForegroundColor Green }
        Write-Host "       - Protocol: Likely IEEE 1588v2 over IP" -ForegroundColor Yellow
    } else {
        Write-Host "     ✗ Not detected" -ForegroundColor Red
    }
    
    # Test 2: IEEE 802.1AS (gPTP) Layer 2
    Write-Host "  2. IEEE 802.1AS (gPTP) over Layer 2:" -ForegroundColor White
    Write-Host "     → Our gPTP should detect this automatically" -ForegroundColor Yellow
    Write-Host "     → Uses multicast MAC: 01-1B-19-00-00-00" -ForegroundColor Yellow
    Write-Host "     → Check: No UDP ports needed for Layer 2" -ForegroundColor Yellow
    
    # Test 3: Hardware-based PTP
    Write-Host "  3. Hardware-based PTP (Transparent Clock):" -ForegroundColor White
    Write-Host "     → May not have open UDP ports" -ForegroundColor Yellow
    Write-Host "     → Acts as PTP-aware switch/bridge" -ForegroundColor Yellow
    Write-Host "     → Forwards PTP but doesn't originate" -ForegroundColor Yellow
    
    # Test 4: Proprietary PTP
    Write-Host "  4. Proprietary/Custom PTP:" -ForegroundColor White
    
    # Test other common industrial PTP ports
    $industrialPorts = @(1588, 8080, 8319, 8320)
    $foundIndustrial = @()
    
    foreach ($port in $industrialPorts) {
        $connection = Test-NetConnection -ComputerName $device -Port $port -WarningAction SilentlyContinue -ErrorAction SilentlyContinue -InformationLevel Quiet
        if ($connection) {
            $foundIndustrial += $port
        }
    }
    
    if ($foundIndustrial.Count -gt 0) {
        Write-Host "     ✓ DETECTED - Custom/Industrial PTP ports:" -ForegroundColor Green
        $foundIndustrial | ForEach-Object { Write-Host "       - Port ${_}: OPEN" -ForegroundColor Green }
    } else {
        Write-Host "     ✗ No custom PTP ports detected" -ForegroundColor Red
    }
    
    Write-Host ""
    Write-Host "Device Analysis Summary:" -ForegroundColor Cyan
    
    if ($ptp319 -or $ptp320) {
        Write-Host "  CONCLUSION: Standard IEEE 1588v2 PTP over UDP" -ForegroundColor Green
        Write-Host "  ISSUE: Our gPTP uses 802.1AS (Layer 2), device uses Layer 3" -ForegroundColor Yellow
        Write-Host "  SOLUTION: Configure device for 802.1AS or use IEEE 1588v2 daemon" -ForegroundColor White
    } elseif ($foundIndustrial.Count -gt 0) {
        Write-Host "  CONCLUSION: Industrial/Proprietary PTP implementation" -ForegroundColor Yellow
        Write-Host "  ACTION: Check device documentation for PTP configuration" -ForegroundColor White
    } else {
        Write-Host "  CONCLUSION: Device using Layer 2 PTP or Hardware-transparent" -ForegroundColor Yellow
        Write-Host "  STATUS: Should be compatible with our gPTP" -ForegroundColor Green
        Write-Host "  ISSUE: Device may not be in PTP Master/Slave mode" -ForegroundColor Red
    }
    
    Write-Host ""
}

Write-Host "[COMPATIBILITY ANALYSIS]" -ForegroundColor Yellow
Write-Host "========================" -ForegroundColor Gray

Write-Host "Our System: IEEE 802.1AS (gPTP) - Layer 2 Ethernet" -ForegroundColor Cyan
Write-Host "Milan Devices: " -NoNewline -ForegroundColor Cyan

# Based on results, provide specific guidance
if ($ptp319 -or $ptp320) {
    Write-Host "IEEE 1588v2 over UDP - Layer 3" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "COMPATIBILITY ISSUE IDENTIFIED:" -ForegroundColor Red
    Write-Host "  ✗ Protocol mismatch: 802.1AS (Layer 2) vs 1588v2 (Layer 3)" -ForegroundColor Red
    Write-Host ""
    Write-Host "SOLUTIONS:" -ForegroundColor Green
    Write-Host "  1. Configure Milan devices for 802.1AS mode" -ForegroundColor White
    Write-Host "  2. Switch to IEEE 1588v2 daemon (PTPd or LinuxPTP)" -ForegroundColor White
    Write-Host "  3. Use protocol bridge/gateway" -ForegroundColor White
} else {
    Write-Host "Likely Layer 2 or Hardware-based" -ForegroundColor Green
    Write-Host ""
    Write-Host "COMPATIBILITY: Should work with our gPTP" -ForegroundColor Green
    Write-Host "ISSUE: Devices may need configuration to enable PTP" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "TROUBLESHOOTING:" -ForegroundColor Green
    Write-Host "  1. Access device web interface to enable PTP" -ForegroundColor White
    Write-Host "  2. Check for 'IEEE 1588' or 'PTP' settings" -ForegroundColor White
    Write-Host "  3. Enable 'Milan' or 'AVB' profile if available" -ForegroundColor White
}

Write-Host ""
Write-Host "=== PTP VARIANT DETECTION COMPLETE ===" -ForegroundColor Green