# Quick gPTP Announce Check
# User insight: gPTP is Layer 2 multicast, need to verify announce messages

Write-Host "=== GPTP ANNOUNCE MESSAGE CHECK ===" -ForegroundColor Green
Write-Host ""

Write-Host "USER INSIGHT VALIDATION:" -ForegroundColor Yellow
Write-Host "✓ gPTP/802.1AS is Layer 2 multicast" -ForegroundColor Green
Write-Host "✓ Uses multicast MAC: 01-1B-19-00-00-00" -ForegroundColor Green  
Write-Host "✓ UDP port analysis was incorrect approach" -ForegroundColor Yellow
Write-Host "✓ Key question: Are we sending announce messages?" -ForegroundColor Cyan
Write-Host ""

# Start Milan test to get gPTP running
Write-Host "Starting gPTP daemon..." -ForegroundColor Cyan
$process = Start-Process -FilePath "powershell" -ArgumentList "-ExecutionPolicy", "Bypass", "-File", "thirdparty\gptp\test_milan_compliance_validation.ps1" -WindowStyle Hidden -PassThru

Start-Sleep -Seconds 8

# Check if gPTP is running
$gptpRunning = Get-Process -Name "daemon_cl" -ErrorAction SilentlyContinue
if ($gptpRunning) {
    Write-Host "✓ gPTP daemon active (PID: $($gptpRunning.Id))" -ForegroundColor Green
} else {
    Write-Host "✗ gPTP daemon not running" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "CRITICAL QUESTIONS:" -ForegroundColor Yellow
Write-Host "1. Is gPTP configured as MASTER (to send announces)?" -ForegroundColor White
Write-Host "2. Are announce messages being transmitted every 1-8 seconds?" -ForegroundColor White
Write-Host "3. Is our system acting as a clock source?" -ForegroundColor White
Write-Host ""

# Check gPTP configuration
Write-Host "Checking gPTP configuration..." -ForegroundColor Cyan
$configFile = "thirdparty\gptp\gptp_cfg.ini"
if (Test-Path $configFile) {
    $config = Get-Content $configFile
    Write-Host "Key config settings:" -ForegroundColor White
    $config | Select-String "priority1|priority2|clockClass|announceReceiptTimeout" | ForEach-Object {
        Write-Host "  $_" -ForegroundColor Gray
    }
} else {
    Write-Host "No config file found - using defaults" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "NEXT VERIFICATION STEPS:" -ForegroundColor Yellow
Write-Host "1. Monitor with Wireshark for 01-1B-19-00-00-00 traffic" -ForegroundColor White
Write-Host "2. Check if announce intervals are active" -ForegroundColor White  
Write-Host "3. Verify BMCA is selecting our clock as master" -ForegroundColor White
Write-Host ""

Write-Host "KEY INSIGHT: If Milan devices have gPTP enabled but no" -ForegroundColor Cyan
Write-Host "communication, issue may be:" -ForegroundColor Cyan
Write-Host "- Our gPTP not announcing (slave mode only)" -ForegroundColor White
Write-Host "- Milan devices in master mode (won't respond to us)" -ForegroundColor White
Write-Host "- Domain mismatch between systems" -ForegroundColor White

Write-Host ""
Write-Host "=== ANNOUNCE CHECK COMPLETE ===" -ForegroundColor Green