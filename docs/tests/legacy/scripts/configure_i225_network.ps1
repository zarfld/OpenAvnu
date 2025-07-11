# I225 gPTP Network Configuration Script
# Requires Administrator privileges

Write-Host "========================================" -ForegroundColor Green
Write-Host "🔧 I225 gPTP Network Configuration" -ForegroundColor Green  
Write-Host "========================================" -ForegroundColor Green
Write-Host ""

# Check if running as Administrator
$isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")

if (-not $isAdmin) {
    Write-Host "❌ This script requires Administrator privileges!" -ForegroundColor Red
    Write-Host "   Please run PowerShell as Administrator and execute this script again." -ForegroundColor Yellow
    Write-Host ""
    Write-Host "To run as Administrator:" -ForegroundColor Cyan
    Write-Host "1. Right-click PowerShell" -ForegroundColor White
    Write-Host "2. Select 'Run as Administrator'" -ForegroundColor White
    Write-Host "3. Navigate to: $PSScriptRoot" -ForegroundColor White
    Write-Host "4. Run: .\configure_i225_network.ps1" -ForegroundColor White
    exit 1
}

Write-Host "✅ Running with Administrator privileges" -ForegroundColor Green
Write-Host ""

# Display current I225 adapter status
Write-Host "📋 Current I225 Adapter Status:" -ForegroundColor Cyan
Get-NetAdapter | Where-Object {$_.InterfaceDescription -like "*I225*"} | Format-Table Name, InterfaceDescription, LinkSpeed, Status

Write-Host ""
Write-Host "🔧 Configuring I225 adapters for gPTP Master-Slave test..." -ForegroundColor Yellow

# Configure Master (Ethernet 2)
Write-Host "   Configuring Master (Ethernet 2): 192.168.100.1/24" -ForegroundColor White
netsh interface ip set address "Ethernet 2" static 192.168.100.1 255.255.255.0

if ($LASTEXITCODE -eq 0) {
    Write-Host "   ✅ Master configured successfully" -ForegroundColor Green
} else {
    Write-Host "   ❌ Master configuration failed" -ForegroundColor Red
}

Start-Sleep -Seconds 2

# Configure Slave (Ethernet 5)
Write-Host "   Configuring Slave (Ethernet 5): 192.168.100.2/24" -ForegroundColor White
netsh interface ip set address "Ethernet 5" static 192.168.100.2 255.255.255.0

if ($LASTEXITCODE -eq 0) {
    Write-Host "   ✅ Slave configured successfully" -ForegroundColor Green
} else {
    Write-Host "   ❌ Slave configuration failed" -ForegroundColor Red
}

Write-Host ""
Write-Host "📊 Updated I225 Adapter Status:" -ForegroundColor Cyan
Get-NetAdapter | Where-Object {$_.InterfaceDescription -like "*I225*"} | Format-Table Name, InterfaceDescription, LinkSpeed, Status

Write-Host ""
Write-Host "🔌 Next Steps:" -ForegroundColor Yellow
Write-Host "1. Connect Ethernet cable between both I225 adapters" -ForegroundColor White
Write-Host "2. Wait for link establishment (should show 'Up' status)" -ForegroundColor White
Write-Host "3. Test connectivity: ping 192.168.100.2" -ForegroundColor White
Write-Host "4. Start gPTP Master-Slave test" -ForegroundColor White

Write-Host ""
Write-Host "🎯 Test Commands:" -ForegroundColor Cyan
Write-Host "   ping 192.168.100.2  # Test connectivity" -ForegroundColor White
Write-Host "   ping 192.168.100.1  # Reverse test" -ForegroundColor White

Write-Host ""
Write-Host "✅ I225 network configuration complete!" -ForegroundColor Green
Write-Host "Ready for gPTP Master-Slave testing." -ForegroundColor Green
