Write-Host "=== HIVE DISCOVERY DEBUG ===" -ForegroundColor Magenta

$Interface = "3DC822E6-8C68-424C-9798-63CFBF52994B"
$entityPath = ".\build\examples\Release\raw_ethernet_avdecc_entity.exe"

Write-Host "Checking entity executable..." -ForegroundColor Cyan
if (Test-Path $entityPath) {
    Write-Host "✓ Entity found: $entityPath" -ForegroundColor Green
} else {
    Write-Host "✗ Entity not found: $entityPath" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "Testing entity with 15-second run..." -ForegroundColor Cyan
Write-Host "Interface GUID: $Interface" -ForegroundColor Gray

$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$logFile = "debug_output_$timestamp.log"

Write-Host ""
Write-Host "Running entity test..." -ForegroundColor Yellow

# Run the entity for 15 seconds and capture all output
$output = & $entityPath --interface $Interface --duration 15 2>&1

# Save output to file
$output | Out-File -FilePath $logFile

# Count important events
$txLines = $output | Where-Object { $_ -like "*Raw Ethernet TX*" }
$rxLines = $output | Where-Object { $_ -like "*Raw Ethernet RX*" }
$adpLines = $output | Where-Object { $_ -like "*ADP Entity Available*" }
$errorLines = $output | Where-Object { $_ -like "*ERROR*" -or $_ -like "*Failed*" }

Write-Host ""
Write-Host "=== RESULTS ===" -ForegroundColor Cyan
Write-Host "Transmitted packets: $($txLines.Count)" -ForegroundColor White
Write-Host "Received packets: $($rxLines.Count)" -ForegroundColor White  
Write-Host "ADP advertisements: $($adpLines.Count)" -ForegroundColor White
Write-Host "Errors: $($errorLines.Count)" -ForegroundColor White

if ($txLines.Count -eq 0) {
    Write-Host ""
    Write-Host "🚨 PROBLEM FOUND: No packets transmitted!" -ForegroundColor Red
    Write-Host "This is why Hive cannot see the entity." -ForegroundColor Red
    Write-Host ""
    Write-Host "Troubleshooting steps:" -ForegroundColor Yellow
    Write-Host "1. Check if running as Administrator" -ForegroundColor White
    Write-Host "2. Verify Npcap is installed and running" -ForegroundColor White
    Write-Host "3. Check network interface GUID is correct" -ForegroundColor White
} else {
    Write-Host ""
    Write-Host "✓ Entity is transmitting - problem is elsewhere" -ForegroundColor Green
    
    if ($rxLines.Count -gt 0) {
        Write-Host "✓ Entity sees other AVDECC traffic" -ForegroundColor Green
    }
    
    Write-Host ""
    Write-Host "Hive troubleshooting:" -ForegroundColor Yellow
    Write-Host "1. Ensure Hive uses same network interface" -ForegroundColor White
    Write-Host "2. Try separate machines for Hive and entity" -ForegroundColor White
    Write-Host "3. Check Hive interface selection settings" -ForegroundColor White
}

if ($errorLines.Count -gt 0) {
    Write-Host ""
    Write-Host "Errors found:" -ForegroundColor Red
    foreach ($errLine in $errorLines) {
        Write-Host "  $errLine" -ForegroundColor Red
    }
}

Write-Host ""
Write-Host "Full output saved to: $logFile" -ForegroundColor Gray
Write-Host "=== DEBUG COMPLETE ===" -ForegroundColor Magenta
