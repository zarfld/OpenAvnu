# Debug Intel I219 Detection
Write-Host "=== Debugging Intel I219 Detection ===" -ForegroundColor Green

# Show all adapters
Write-Host "`nAll Network Adapters:" -ForegroundColor Cyan
Get-NetAdapter | Select-Object Name, InterfaceDescription, Status | Format-Table -AutoSize

# Test Intel filter
Write-Host "`nIntel Adapters:" -ForegroundColor Cyan
$intelAdapters = Get-NetAdapter | Where-Object { $_.InterfaceDescription -like "*Intel*" }
$intelAdapters | Select-Object Name, InterfaceDescription, Status | Format-Table -AutoSize

# Test I219 filter  
Write-Host "`nI219 Adapters:" -ForegroundColor Cyan
$i219Adapters = Get-NetAdapter | Where-Object { 
    $_.InterfaceDescription -like "*Intel*" -and 
    $_.InterfaceDescription -like "*I219*" -and
    $_.Status -eq "Up"
}
$i219Adapters | Select-Object Name, InterfaceDescription, Status | Format-Table -AutoSize

if ($i219Adapters.Count -gt 0) {
    Write-Host "SUCCESS: Found $($i219Adapters.Count) I219 adapter(s)!" -ForegroundColor Green
    $selected = $i219Adapters[0]
    Write-Host "Selected: $($selected.Name)" -ForegroundColor Yellow
    Write-Host "Description: $($selected.InterfaceDescription)" -ForegroundColor Yellow
} else {
    Write-Host "ERROR: I219 adapter not found!" -ForegroundColor Red
    Write-Host "Count: $($i219Adapters.Count)" -ForegroundColor Red
}

Write-Host "`n=== End Debug ===" -ForegroundColor Green
