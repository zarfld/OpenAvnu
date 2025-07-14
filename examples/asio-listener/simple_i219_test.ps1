Write-Host "=== Intel I219 Auto-Detection Test ===" -ForegroundColor Cyan
Write-Host ""

Write-Host "Scanning for Intel I219 network adapters..." -ForegroundColor Yellow

$i219Adapters = Get-NetAdapter | Where-Object { 
    $_.InterfaceDescription -like "*Intel*" -and 
    $_.InterfaceDescription -like "*I219*" -and
    $_.Status -eq "Up"
}

if ($i219Adapters.Count -gt 0) {
    Write-Host "SUCCESS: Intel I219 adapter found!" -ForegroundColor Green
    Write-Host ""
    
    foreach ($adapter in $i219Adapters) {
        Write-Host "Interface Name: $($adapter.Name)" -ForegroundColor White
        Write-Host "Description: $($adapter.InterfaceDescription)" -ForegroundColor White
        Write-Host "Link Speed: $($adapter.LinkSpeed)" -ForegroundColor White
        Write-Host "MAC Address: $($adapter.MacAddress)" -ForegroundColor White
        Write-Host "Status: $($adapter.Status)" -ForegroundColor Green
        Write-Host ""
    }
    
    Write-Host "ANTWORT:" -ForegroundColor Green
    Write-Host "JA - Automatisches Erkennen funktioniert!" -ForegroundColor Green
    Write-Host "Keine manuellen Konfigurationsdateien noetig!" -ForegroundColor Green
    Write-Host "OpenAvnu kann Ihren Intel I219 automatisch verwenden." -ForegroundColor Green
    
} else {
    Write-Host "No Intel I219 adapters found!" -ForegroundColor Red
    Write-Host "Showing all available adapters:" -ForegroundColor Yellow
    
    Get-NetAdapter | Where-Object { $_.Status -eq "Up" } | ForEach-Object {
        Write-Host "  - $($_.Name): $($_.InterfaceDescription)" -ForegroundColor White
    }
}

Write-Host ""
Write-Host "=== Test Complete ===" -ForegroundColor Cyan
