# gPTP Layer 2 Multicast Traffic Analysis
# Check if our gPTP is sending proper 802.1AS multicast announcements

Write-Host "=== GPTP LAYER 2 MULTICAST ANALYSIS ===" -ForegroundColor Green
Write-Host "Checking if our gPTP is sending proper 802.1AS announce messages" -ForegroundColor Cyan
Write-Host ""

Write-Host "[USER INSIGHT VALIDATION]" -ForegroundColor Yellow
Write-Host "=========================" -ForegroundColor Gray
Write-Host "✓ gPTP/802.1AS uses Layer 2 Ethernet multicast" -ForegroundColor Green
Write-Host "✓ Multicast MAC: 01-1B-19-00-00-00" -ForegroundColor Green
Write-Host "✓ UDP ports 319/320 are NOT used for Layer 2 gPTP" -ForegroundColor Green
Write-Host "✓ Previous port scanning was incorrect approach" -ForegroundColor Yellow
Write-Host ""

Write-Host "[GPTP PROCESS STATUS]" -ForegroundColor Yellow
Write-Host "====================" -ForegroundColor Gray

# Check if gPTP is running
$gptpProcess = Get-Process -Name "daemon_cl" -ErrorAction SilentlyContinue
if ($gptpProcess) {
    Write-Host "✓ gPTP daemon running (PID: $($gptpProcess.Id))" -ForegroundColor Green
    Write-Host "  Start Time: $($gptpProcess.StartTime)" -ForegroundColor White
    Write-Host "  CPU Time: $($gptpProcess.TotalProcessorTime)" -ForegroundColor White
} else {
    Write-Host "✗ gPTP daemon not running - starting test..." -ForegroundColor Yellow
    # Start Milan compliance test to get gPTP running
    Write-Host "Starting Milan compliance test to activate gPTP..." -ForegroundColor Cyan
    Start-Process -FilePath "powershell" -ArgumentList "-ExecutionPolicy", "Bypass", "-File", "thirdparty\gptp\test_milan_compliance_validation.ps1" -WindowStyle Hidden
    Start-Sleep -Seconds 5
    $gptpProcess = Get-Process -Name "daemon_cl" -ErrorAction SilentlyContinue
    if ($gptpProcess) {
        Write-Host "✓ gPTP daemon now running (PID: $($gptpProcess.Id))" -ForegroundColor Green
    } else {
        Write-Host "✗ Failed to start gPTP daemon" -ForegroundColor Red
        exit 1
    }
}

Write-Host ""

Write-Host "[MULTICAST TRAFFIC ANALYSIS]" -ForegroundColor Yellow
Write-Host "============================" -ForegroundColor Gray

Write-Host "Checking for gPTP multicast activity..." -ForegroundColor Cyan

# Check network interface multicast memberships
Write-Host "Network interface multicast memberships:" -ForegroundColor White
try {
    $multicastInfo = netsh interface ipv4 show joins
    $gptpMulticast = $multicastInfo | Select-String "01-1B-19"
    if ($gptpMulticast) {
        Write-Host "✓ gPTP multicast membership detected:" -ForegroundColor Green
        $gptpMulticast | ForEach-Object { Write-Host "  $_" -ForegroundColor White }
    } else {
        Write-Host "✗ No gPTP multicast membership found" -ForegroundColor Yellow
        Write-Host "  Note: Windows may not show Ethernet multicast in netsh" -ForegroundColor Gray
    }
} catch {
    Write-Host "Could not query multicast memberships" -ForegroundColor Yellow
}

Write-Host ""

# Check for recent gPTP log activity 
Write-Host "Recent gPTP log activity:" -ForegroundColor Cyan
$logFile = "gptp_log.txt"
if (Test-Path $logFile) {
    Write-Host "✓ Found gPTP log file" -ForegroundColor Green
    $recentLogs = Get-Content $logFile -Tail 10 -ErrorAction SilentlyContinue
    if ($recentLogs) {
        Write-Host "Recent log entries:" -ForegroundColor White
        $recentLogs | ForEach-Object { Write-Host "  $_" -ForegroundColor Gray }
        
        # Look for announce and sync message activity
        $announceCount = ($recentLogs | Select-String "announce" -AllMatches).Count
        $syncCount = ($recentLogs | Select-String "sync" -AllMatches).Count
        $pdelayCount = ($recentLogs | Select-String "pdelay" -AllMatches).Count
        
        Write-Host ""
        Write-Host "Message activity in recent logs:" -ForegroundColor Cyan
        Write-Host "  Announce messages: $announceCount" -ForegroundColor White
        Write-Host "  Sync messages: $syncCount" -ForegroundColor White  
        Write-Host "  PDelay messages: $pdelayCount" -ForegroundColor White
    }
} else {
    Write-Host "✗ No gPTP log file found" -ForegroundColor Yellow
}

Write-Host ""

Write-Host "[LAYER 2 ANNOUNCE VERIFICATION]" -ForegroundColor Yellow
Write-Host "===============================" -ForegroundColor Gray

Write-Host "Checking if gPTP is configured for announce transmission:" -ForegroundColor Cyan

# Check gPTP configuration for announce settings
$configFile = "thirdparty\gptp\gptp_cfg.ini"
if (Test-Path $configFile) {
    Write-Host "✓ Found gPTP config file" -ForegroundColor Green
    $configContent = Get-Content $configFile -ErrorAction SilentlyContinue
    
    # Look for announce-related settings
    $announceSettings = $configContent | Select-String "announce|master|priority|domain"
    if ($announceSettings) {
        Write-Host "Announce-related configuration:" -ForegroundColor White
        $announceSettings | ForEach-Object { Write-Host "  $_" -ForegroundColor Gray }
    }
    
    # Check if configured as potential master
    $masterSettings = $configContent | Select-String "priority1|priority2|clockClass"
    if ($masterSettings) {
        Write-Host ""
        Write-Host "Master clock settings:" -ForegroundColor White
        $masterSettings | ForEach-Object { Write-Host "  $_" -ForegroundColor Gray }
    }
} else {
    Write-Host "✗ No gPTP config file found" -ForegroundColor Yellow
}

Write-Host ""

Write-Host "[MULTICAST ANNOUNCEMENT STATUS]" -ForegroundColor Yellow
Write-Host "===============================" -ForegroundColor Gray

Write-Host "Key Questions to Verify:" -ForegroundColor Cyan
Write-Host "1. Is our gPTP sending announce messages? (Should be every 1-8 seconds)" -ForegroundColor White
Write-Host "2. Is our gPTP configured as potential master?" -ForegroundColor White
Write-Host "3. Are announce messages reaching Layer 2 multicast?" -ForegroundColor White
Write-Host "4. Are Milan devices receiving but not responding?" -ForegroundColor White
Write-Host ""

Write-Host "Next Steps:" -ForegroundColor Cyan
Write-Host "- Monitor network with Wireshark for 01-1B-19-00-00-00 traffic" -ForegroundColor White
Write-Host "- Check if announce messages are being transmitted" -ForegroundColor White
Write-Host "- Verify BMCA (Best Master Clock Algorithm) is active" -ForegroundColor White
Write-Host ""

Write-Host "=== LAYER 2 MULTICAST ANALYSIS COMPLETE ===" -ForegroundColor Green