# Simple I225 Network Configuration
# Run as Administrator

Write-Host "I225 gPTP Network Configuration" -ForegroundColor Green
Write-Host "===============================" -ForegroundColor Green

# Check admin privileges
$isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")

if (-not $isAdmin) {
    Write-Host "ERROR: Administrator privileges required!" -ForegroundColor Red
    Write-Host "Please run PowerShell as Administrator" -ForegroundColor Yellow
    exit 1
}

Write-Host "Running with Administrator privileges" -ForegroundColor Green
Write-Host ""

# Configure Master
Write-Host "Configuring I225 Master (Ethernet 2)..." -ForegroundColor Yellow
netsh interface ip set address "Ethernet 2" static 192.168.100.1 255.255.255.0

# Configure Slave  
Write-Host "Configuring I225 Slave (Ethernet 5)..." -ForegroundColor Yellow
netsh interface ip set address "Ethernet 5" static 192.168.100.2 255.255.255.0

Write-Host ""
Write-Host "Configuration complete!" -ForegroundColor Green
Write-Host "Next: Connect Ethernet cable between both I225 adapters" -ForegroundColor Cyan
Write-Host "Test: ping 192.168.100.2" -ForegroundColor Cyan
