Write-Host "=== Debug Intel Adapter Detection ===" -ForegroundColor Cyan
Write-Host ""

Write-Host "All network adapters:" -ForegroundColor Yellow
Get-NetAdapter | ForEach-Object {
    Write-Host "Name: '$($_.Name)'" -ForegroundColor White
    Write-Host "Description: '$($_.InterfaceDescription)'" -ForegroundColor White
    Write-Host "Status: '$($_.Status)'" -ForegroundColor White
    Write-Host "---" -ForegroundColor Gray
}

Write-Host ""
Write-Host "Testing exact match for your adapter:" -ForegroundColor Yellow

$yourAdapter = Get-NetAdapter | Where-Object { $_.Name -eq "Ethernet" }
if ($yourAdapter) {
    Write-Host "Found your 'Ethernet' adapter!" -ForegroundColor Green
    Write-Host "Description: '$($yourAdapter.InterfaceDescription)'" -ForegroundColor White
    Write-Host "Status: '$($yourAdapter.Status)'" -ForegroundColor White
    
    # Test if it contains Intel
    if ($yourAdapter.InterfaceDescription -like "*Intel*") {
        Write-Host "YES - Contains 'Intel'!" -ForegroundColor Green
    } else {
        Write-Host "NO - Does not contain 'Intel'!" -ForegroundColor Red
    }
    
    # Test if it contains I219
    if ($yourAdapter.InterfaceDescription -like "*I219*") {
        Write-Host "YES - Contains 'I219'!" -ForegroundColor Green
    } else {
        Write-Host "NO - Does not contain 'I219'!" -ForegroundColor Red
    }
    
    # Manual check character by character
    Write-Host ""
    Write-Host "Character analysis of description:" -ForegroundColor Yellow
    $description = $yourAdapter.InterfaceDescription
    for ($i = 0; $i -lt $description.Length; $i++) {
        $char = $description[$i]
        Write-Host "[$i]: '$char' (ASCII: $([int][char]$char))" -ForegroundColor Gray
    }
}

Write-Host ""
Write-Host "=== Debug Complete ===" -ForegroundColor Cyan
