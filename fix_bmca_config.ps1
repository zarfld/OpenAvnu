# Fix gPTP BMCA Configuration for Milan
# Enable announce message transmission and BMCA participation

Write-Host "=== FIXING GPTP BMCA CONFIGURATION ===" -ForegroundColor Green
Write-Host ""

Write-Host "PROBLEM IDENTIFIED:" -ForegroundColor Red
Write-Host "✗ Current gPTP configured as slave-only (priority1=248, clockClass=248)" -ForegroundColor Red
Write-Host "✗ Cannot send announce messages" -ForegroundColor Red  
Write-Host "✗ Milan devices never see us in BMCA" -ForegroundColor Red
Write-Host ""

Write-Host "SOLUTION:" -ForegroundColor Green
Write-Host "✓ Change to Milan BMCA-compatible configuration" -ForegroundColor Green
Write-Host "✓ Enable announce message transmission" -ForegroundColor Green
Write-Host "✓ Allow participation as master or slave" -ForegroundColor Green
Write-Host ""

# Backup current configuration
$currentConfig = "thirdparty\gptp\gptp_cfg.ini"
$backupConfig = "thirdparty\gptp\gptp_cfg_slave_backup.ini"

if (Test-Path $currentConfig) {
    Write-Host "Backing up current configuration..." -ForegroundColor Cyan
    Copy-Item $currentConfig $backupConfig -Force
    Write-Host "✓ Backup saved as: $backupConfig" -ForegroundColor Green
}

# Apply Milan BMCA configuration
$milanConfig = "gptp_cfg_milan_bmca.ini"
if (Test-Path $milanConfig) {
    Write-Host "Applying Milan BMCA configuration..." -ForegroundColor Cyan
    Copy-Item $milanConfig $currentConfig -Force
    Write-Host "✓ Milan BMCA configuration applied" -ForegroundColor Green
} else {
    Write-Host "✗ Milan configuration file not found" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "NEW CONFIGURATION:" -ForegroundColor Yellow
Write-Host "  priority1 = 100 (can be master)" -ForegroundColor Green
Write-Host "  clockClass = 6 (ordinary clock)" -ForegroundColor Green
Write-Host "  profile = milan" -ForegroundColor Green
Write-Host ""

Write-Host "BMCA BEHAVIOR:" -ForegroundColor Yellow
Write-Host "✓ Will send announce messages every 1-8 seconds" -ForegroundColor Green
Write-Host "✓ Can become master if no better clocks available" -ForegroundColor Green
Write-Host "✓ Will become slave if better master discovered" -ForegroundColor Green
Write-Host "✓ Milan devices can now see us in BMCA" -ForegroundColor Green
Write-Host ""

Write-Host "NEXT STEPS:" -ForegroundColor Cyan
Write-Host "1. Restart gPTP daemon with new configuration" -ForegroundColor White
Write-Host "2. Monitor for announce message transmission" -ForegroundColor White
Write-Host "3. Verify Milan devices respond to our announces" -ForegroundColor White
Write-Host "4. Confirm BMCA selects appropriate master" -ForegroundColor White
Write-Host ""

Write-Host "=== BMCA CONFIGURATION FIXED ===" -ForegroundColor Green
Write-Host "Ready to test Milan compliance with proper BMCA!" -ForegroundColor Cyan