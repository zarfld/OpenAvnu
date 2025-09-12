# gPTP BMCA Configuration Analysis
# Check if our gPTP can participate in BMCA and send announce messages

Write-Host "=== GPTP BMCA CONFIGURATION ANALYSIS ===" -ForegroundColor Green
Write-Host "Analyzing Best Master Clock Algorithm configuration" -ForegroundColor Cyan
Write-Host ""

Write-Host "[BMCA FUNDAMENTALS]" -ForegroundColor Yellow
Write-Host "==================" -ForegroundColor Gray
Write-Host "BMCA determines which device becomes the master clock:" -ForegroundColor Cyan
Write-Host "1. Lower priority1 = better master candidate" -ForegroundColor White
Write-Host "2. Lower clockClass = better clock quality" -ForegroundColor White
Write-Host "3. All devices send announce messages for comparison" -ForegroundColor White
Write-Host "4. Milan requires BMCA participation for compliance" -ForegroundColor White
Write-Host ""

Write-Host "[CURRENT GPTP CONFIGURATION]" -ForegroundColor Yellow
Write-Host "============================" -ForegroundColor Gray

$configFile = "thirdparty\gptp\gptp_cfg.ini"
if (Test-Path $configFile) {
    Write-Host "✓ Found gPTP configuration file" -ForegroundColor Green
    $config = Get-Content $configFile
    
    # Extract BMCA-critical parameters
    $priority1 = ($config | Select-String "priority1") -split "=" | ForEach-Object { $_.Trim() } | Where-Object { $_ -match "^\d+" }
    $priority2 = ($config | Select-String "priority2") -split "=" | ForEach-Object { $_.Trim() } | Where-Object { $_ -match "^\d+" }
    $clockClass = ($config | Select-String "clockClass") -split "=" | ForEach-Object { $_.Trim() } | Where-Object { $_ -match "^\d+" }
    $ptpProfile = ($config | Select-String "profile") -split "=" | ForEach-Object { $_.Trim() } | Where-Object { $_ -notmatch "^#" -and $_ -ne "" }
    
    Write-Host "Current BMCA Configuration:" -ForegroundColor Cyan
    Write-Host "  Profile: $($ptpProfile -join '')" -ForegroundColor White
    Write-Host "  Priority1: $($priority1 -join '')" -ForegroundColor White
    Write-Host "  Priority2: $($priority2 -join '')" -ForegroundColor White  
    Write-Host "  ClockClass: $($clockClass -join '')" -ForegroundColor White
    
    Write-Host ""
    Write-Host "BMCA Analysis:" -ForegroundColor Cyan
    
    $priority1Val = [int]($priority1 -join '')
    $clockClassVal = [int]($clockClass -join '')
    
    if ($priority1Val -ge 248) {
        Write-Host "  ✗ ISSUE: Priority1 = $priority1Val (slave-only configuration)" -ForegroundColor Red
        Write-Host "    → Will NOT send announce messages" -ForegroundColor Red
        Write-Host "    → Cannot participate as master in BMCA" -ForegroundColor Red
    } elseif ($priority1Val -ge 128) {
        Write-Host "  ⚠️ WARNING: Priority1 = $priority1Val (low master priority)" -ForegroundColor Yellow
        Write-Host "    → Will send announces but unlikely to be selected" -ForegroundColor Yellow
    } else {
        Write-Host "  ✓ GOOD: Priority1 = $priority1Val (can be master)" -ForegroundColor Green
    }
    
    if ($clockClassVal -ge 248) {
        Write-Host "  ✗ ISSUE: ClockClass = $clockClassVal (slave-only)" -ForegroundColor Red
        Write-Host "    → Declares itself as slave clock" -ForegroundColor Red
    } elseif ($clockClassVal -ge 187) {
        Write-Host "  ⚠️ WARNING: ClockClass = $clockClassVal (poor quality)" -ForegroundColor Yellow
    } else {
        Write-Host "  ✓ GOOD: ClockClass = $clockClassVal (can be master)" -ForegroundColor Green
    }
    
} else {
    Write-Host "✗ No gPTP configuration file found" -ForegroundColor Red
}

Write-Host ""
Write-Host "[MILAN BMCA REQUIREMENTS]" -ForegroundColor Yellow
Write-Host "=========================" -ForegroundColor Gray
Write-Host "For proper Milan operation:" -ForegroundColor Cyan
Write-Host "1. ALL devices must send announce messages" -ForegroundColor White
Write-Host "2. BMCA must select the best available master" -ForegroundColor White
Write-Host "3. Other devices become slaves and sync to master" -ForegroundColor White
Write-Host "4. If master fails, BMCA selects new master" -ForegroundColor White
Write-Host ""

Write-Host "[RECOMMENDED BMCA CONFIGURATION]" -ForegroundColor Yellow
Write-Host "===============================" -ForegroundColor Gray
Write-Host "For Milan compliance testing:" -ForegroundColor Cyan
Write-Host "  Priority1: 64-127 (can be master, but not aggressive)" -ForegroundColor White
Write-Host "  Priority2: 1" -ForegroundColor White
Write-Host "  ClockClass: 6-13 (ordinary clock quality)" -ForegroundColor White
Write-Host "  Profile: milan" -ForegroundColor White
Write-Host ""

Write-Host "[CURRENT ISSUE DIAGNOSIS]" -ForegroundColor Yellow
Write-Host "========================" -ForegroundColor Gray

if ($priority1Val -ge 248 -or $clockClassVal -ge 248) {
    Write-Host "PROBLEM IDENTIFIED:" -ForegroundColor Red
    Write-Host "  ✗ Our gPTP is configured as SLAVE-ONLY" -ForegroundColor Red
    Write-Host "  ✗ Will NOT send announce messages" -ForegroundColor Red
    Write-Host "  ✗ Cannot participate in BMCA as master candidate" -ForegroundColor Red
    Write-Host "  ✗ Milan devices don't see us as potential master" -ForegroundColor Red
    Write-Host ""
    Write-Host "SOLUTION:" -ForegroundColor Green
    Write-Host "  1. Change priority1 to 64-127 (enable master capability)" -ForegroundColor White
    Write-Host "  2. Change clockClass to 6-13 (ordinary clock)" -ForegroundColor White
    Write-Host "  3. Restart gPTP daemon" -ForegroundColor White
    Write-Host "  4. Verify announce messages are transmitted" -ForegroundColor White
} else {
    Write-Host "CONFIGURATION STATUS: BMCA-capable" -ForegroundColor Green
}

Write-Host ""
Write-Host "[NEXT STEPS]" -ForegroundColor Yellow
Write-Host "============" -ForegroundColor Gray
Write-Host "1. Fix BMCA configuration to enable announce transmission" -ForegroundColor Cyan
Write-Host "2. Restart gPTP with new configuration" -ForegroundColor Cyan  
Write-Host "3. Monitor for announce message transmission" -ForegroundColor Cyan
Write-Host "4. Verify Milan devices respond to our announces" -ForegroundColor Cyan
Write-Host "5. Confirm BMCA selects appropriate master" -ForegroundColor Cyan
Write-Host ""

Write-Host "=== BMCA ANALYSIS COMPLETE ===" -ForegroundColor Green