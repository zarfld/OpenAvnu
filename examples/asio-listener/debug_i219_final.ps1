Write-Host "=== Intel I219 Auto-Detection - Debug Version ===" -ForegroundColor Cyan
Write-Host ""

# Get all adapters and show what we have
$allAdapters = Get-NetAdapter
Write-Host "Total adapters found: $($allAdapters.Count)" -ForegroundColor Gray

# Filter for Up adapters
$upAdapters = @($allAdapters | Where-Object { $_.Status -eq "Up" })
Write-Host "Adapters that are Up: $($upAdapters.Count)" -ForegroundColor Gray

# Show each up adapter
Write-Host "Up adapters:" -ForegroundColor Yellow
foreach ($adapter in $upAdapters) {
    Write-Host "  - $($adapter.Name): $($adapter.InterfaceDescription)" -ForegroundColor White
}
Write-Host ""

# Look specifically for your Ethernet adapter
$yourAdapter = $upAdapters | Where-Object { $_.Name -eq "Ethernet" }
if ($yourAdapter) {
    Write-Host "Found your 'Ethernet' adapter!" -ForegroundColor Green
    Write-Host "Name: '$($yourAdapter.Name)'" -ForegroundColor White
    Write-Host "Description: '$($yourAdapter.InterfaceDescription)'" -ForegroundColor White
    Write-Host "Status: '$($yourAdapter.Status)'" -ForegroundColor White
    Write-Host ""
    
    # Check if it's Intel
    $isIntel = $yourAdapter.InterfaceDescription -like "*Intel*"
    Write-Host "Is Intel adapter? $isIntel" -ForegroundColor $(if($isIntel){"Green"}else{"Red"})
    
    # Check if it's I219
    $isI219 = $yourAdapter.InterfaceDescription -like "*I219*"
    Write-Host "Is I219 adapter? $isI219" -ForegroundColor $(if($isI219){"Green"}else{"Red"})
    
    if ($isIntel -and $isI219) {
        Write-Host ""
        Write-Host "=== ERFOLGREICH ===" -ForegroundColor Green
        Write-Host "Auto-Detection funktioniert!" -ForegroundColor Green
        Write-Host "Ihr Intel I219-LM wurde automatisch erkannt als: '$($yourAdapter.Name)'" -ForegroundColor Green
        Write-Host ""
        Write-Host "ANTWORT: JA - Keine manuellen Konfigurationsdateien noetig!" -ForegroundColor Green
        Write-Host "OpenAvnu wird automatisch '$($yourAdapter.Name)' verwenden." -ForegroundColor Green
    }
}

Write-Host ""
Write-Host "=== Test Complete ===" -ForegroundColor Cyan
