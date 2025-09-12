# Simple PTP Variant Detection
Write-Host "=== PTP VARIANT DETECTION ===" -ForegroundColor Green
Write-Host "Checking what PTP variant the Milan devices are using..." -ForegroundColor Cyan
Write-Host ""

$devices = @("157.247.3.12", "157.247.1.112")

# Check our gPTP status
Write-Host "[OUR GPTP]" -ForegroundColor Yellow
$listening = netstat -an | Select-String '319|320'
if ($listening) {
    Write-Host "✓ gPTP listening on PTP ports:" -ForegroundColor Green
    $listening | ForEach-Object { Write-Host "  $_" -ForegroundColor White }
} else {
    Write-Host "✗ gPTP not listening" -ForegroundColor Red
}
Write-Host ""

foreach ($device in $devices) {
    Write-Host "[DEVICE $device]" -ForegroundColor Yellow
    
    # Basic connectivity
    $ping = Test-Connection -ComputerName $device -Count 1 -Quiet -ErrorAction SilentlyContinue
    if (-not $ping) {
        Write-Host "✗ Device not reachable" -ForegroundColor Red
        continue
    }
    Write-Host "✓ Device reachable" -ForegroundColor Green
    
    # Test standard PTP ports
    Write-Host "Testing PTP ports:" -ForegroundColor Cyan
    
    $port319 = Test-NetConnection -ComputerName $device -Port 319 -WarningAction SilentlyContinue -ErrorAction SilentlyContinue -InformationLevel Quiet
    $port320 = Test-NetConnection -ComputerName $device -Port 320 -WarningAction SilentlyContinue -ErrorAction SilentlyContinue -InformationLevel Quiet
    
    if ($port319) {
        Write-Host "  ✓ Port 319 (PTP Event) OPEN" -ForegroundColor Green
    } else {
        Write-Host "  ✗ Port 319 (PTP Event) CLOSED" -ForegroundColor Red
    }
    
    if ($port320) {
        Write-Host "  ✓ Port 320 (PTP General) OPEN" -ForegroundColor Green
    } else {
        Write-Host "  ✗ Port 320 (PTP General) CLOSED" -ForegroundColor Red
    }
    
    # Analysis
    if ($port319 -or $port320) {
        Write-Host "RESULT: Device uses IEEE 1588v2 PTP over UDP" -ForegroundColor Yellow
        Write-Host "ISSUE: Our gPTP uses 802.1AS (Layer 2), device uses Layer 3" -ForegroundColor Red
        Write-Host "ACTION: Need protocol compatibility" -ForegroundColor White
    } else {
        Write-Host "RESULT: Device may use Layer 2 PTP or need configuration" -ForegroundColor Yellow
        Write-Host "STATUS: Should be compatible with our 802.1AS gPTP" -ForegroundColor Green
        Write-Host "ACTION: Check if device PTP is enabled" -ForegroundColor White
    }
    
    Write-Host ""
}

Write-Host "=== SUMMARY ===" -ForegroundColor Green
Write-Host "Since user confirmed devices ARE using PTP:" -ForegroundColor Cyan
Write-Host "1. If ports 319/320 are open: Devices use IEEE 1588v2 (incompatible)" -ForegroundColor Yellow
Write-Host "2. If ports closed: Devices likely use Layer 2 PTP (compatible)" -ForegroundColor Yellow
Write-Host "3. Need to configure devices or check PTP settings" -ForegroundColor White