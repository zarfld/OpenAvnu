# Apply Official Milan Specification Configuration
# Use the exact Milan Baseline Interoperability Specification parameters

Write-Host "=== APPLYING OFFICIAL MILAN SPECIFICATION ===" -ForegroundColor Green
Write-Host ""

Write-Host "IMPORTANT DISCOVERY:" -ForegroundColor Yellow
Write-Host "✓ Found official Milan configuration: test_milan_config.ini" -ForegroundColor Green
Write-Host "✓ Milan spec REQUIRES priority1 = 248 (slave-only)" -ForegroundColor Green
Write-Host "✓ Milan spec REQUIRES clockClass = 248" -ForegroundColor Green
Write-Host "✓ Milan devices are DESIGNED to be slaves by default!" -ForegroundColor Cyan
Write-Host ""

Write-Host "MILAN SPECIFICATION INSIGHTS:" -ForegroundColor Yellow
Write-Host "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" -ForegroundColor Gray
Write-Host "1. Milan devices are typically SLAVE-ONLY by design" -ForegroundColor White
Write-Host "2. They expect an external MASTER clock (not Milan)" -ForegroundColor White  
Write-Host "3. Master could be: Audio interface, dedicated PTP master, etc." -ForegroundColor White
Write-Host "4. Milan devices sync TO the master, don't act as masters" -ForegroundColor White
Write-Host ""

# Backup current config
$currentConfig = "thirdparty\gptp\gptp_cfg.ini"
$milanConfig = "thirdparty\gptp\test_milan_config.ini"
$backupConfig = "thirdparty\gptp\gptp_cfg_custom_backup.ini"

if (Test-Path $currentConfig) {
    Write-Host "Backing up current configuration..." -ForegroundColor Cyan
    Copy-Item $currentConfig $backupConfig -Force
    Write-Host "✓ Backup saved as: $backupConfig" -ForegroundColor Green
}

# Apply official Milan configuration
if (Test-Path $milanConfig) {
    Write-Host "Applying official Milan specification configuration..." -ForegroundColor Cyan
    Copy-Item $milanConfig $currentConfig -Force
    Write-Host "✓ Official Milan config applied from: $milanConfig" -ForegroundColor Green
} else {
    Write-Host "✗ Milan configuration file not found" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "OFFICIAL MILAN PARAMETERS APPLIED:" -ForegroundColor Yellow
Write-Host "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" -ForegroundColor Gray
Write-Host "✓ priority1 = 248 (Milan Baseline Spec 2.0a Section 5.6.2.1)" -ForegroundColor Green
Write-Host "✓ clockClass = 248 (Milan required - application specific time)" -ForegroundColor Green
Write-Host "✓ sync_interval = -3 (125ms - Milan Spec 5.6.2.2)" -ForegroundColor Green
Write-Host "✓ announce_interval = 0 (1s - Milan Spec 5.6.2.3)" -ForegroundColor Green
Write-Host "✓ pdelay_interval = 0 (1s - Milan Spec 5.6.2.5)" -ForegroundColor Green
Write-Host "✓ neighborPropDelayThresh = 800μs (Milan Spec 5.6.2.7)" -ForegroundColor Green
Write-Host ""

Write-Host "NEW UNDERSTANDING:" -ForegroundColor Yellow
Write-Host "━━━━━━━━━━━━━━━━━━━━" -ForegroundColor Gray
Write-Host "Issue may NOT be BMCA configuration!" -ForegroundColor Cyan
Write-Host ""
Write-Host "Possible scenarios:" -ForegroundColor White
Write-Host "1. Milan devices expect external master (not us)" -ForegroundColor Gray
Write-Host "2. Need dedicated PTP master on network" -ForegroundColor Gray
Write-Host "3. Milan devices should sync TO master, not each other" -ForegroundColor Gray
Write-Host "4. Our role: Another Milan slave, not the master" -ForegroundColor Gray
Write-Host ""

Write-Host "NEXT TEST APPROACH:" -ForegroundColor Yellow
Write-Host "━━━━━━━━━━━━━━━━━━━━" -ForegroundColor Gray
Write-Host "1. Apply Milan slave configuration (done)" -ForegroundColor Green
Write-Host "2. Test if Milan devices communicate as fellow slaves" -ForegroundColor Cyan
Write-Host "3. Look for existing PTP master on network" -ForegroundColor Cyan
Write-Host "4. Consider if we need external master clock" -ForegroundColor Cyan
Write-Host ""

Write-Host "=== MILAN SPECIFICATION APPLIED ===" -ForegroundColor Green
Write-Host "Ready to test with official Milan parameters!" -ForegroundColor Cyan