# ============================================================================
# Complete AVDECC Entity Startup Script
# ============================================================================
# This script runs the complete OpenAvnu system with AVDECC integration

param(
    [switch]$StopFirst,
    [string]$EntityName = "OpenAvnu Audio Device"
)

Write-Host ""
Write-Host "🎵 OpenAvnu Complete AVDECC System Startup 🎵" -ForegroundColor Magenta
Write-Host "=============================================" -ForegroundColor Magenta
Write-Host ""

# Stop existing processes if requested
if ($StopFirst) {
    Write-Host "⏹️ Stopping existing OpenAvnu processes..." -ForegroundColor Yellow
    Get-Process -Name "mrpd","maap_daemon","simple_asio_listener","simple_avdecc_entity" -ErrorAction SilentlyContinue | Stop-Process -Force
    Start-Sleep -Seconds 2
}

# Check current status
Write-Host "🔍 Checking system status..." -ForegroundColor Yellow

# Check Virtual Audio Cable
$audioDevices = Get-PnpDevice -Class AudioEndpoint | Where-Object {$_.FriendlyName -like "*CABLE*" -and $_.Status -eq 'OK'}
if ($audioDevices) {
    Write-Host "✅ Virtual Audio Cable: Available" -ForegroundColor Green
    $audioDevices | ForEach-Object { Write-Host "   - $($_.FriendlyName)" -ForegroundColor Gray }
} else {
    Write-Host "❌ Virtual Audio Cable: Not found" -ForegroundColor Red
}

# Check if executables exist
$executables = @{
    "MRPD" = "build\daemons\mrpd\Release\mrpd.exe"
    "MAAP" = "build\daemons\maap\Release\maap_daemon.exe"
    "ASIO Listener" = "build\examples\Release\simple_asio_listener.exe"
    "AVDECC Entity" = "build\examples\Release\simple_avdecc_entity.exe"
}

$allReady = $true
foreach ($name in $executables.Keys) {
    if (Test-Path $executables[$name]) {
        Write-Host "✅ $name : Ready" -ForegroundColor Green
    } else {
        Write-Host "❌ $name : Missing" -ForegroundColor Red
        $allReady = $false
    }
}

if (-not $allReady) {
    Write-Host ""
    Write-Host "❌ Some components are missing. Please build OpenAvnu first:" -ForegroundColor Red
    Write-Host "   cmake --build build --config Release" -ForegroundColor Cyan
    exit 1
}

Write-Host ""
Write-Host "🚀 Starting Complete OpenAvnu AVDECC System..." -ForegroundColor Green
Write-Host ""

# Start OpenAvnu daemons using the existing script
Write-Host "📡 Starting OpenAvnu daemons..." -ForegroundColor Cyan
$daemonResult = & powershell.exe -ExecutionPolicy Bypass -File ".\test_asio_multiadapter_clean.ps1" 2>&1

# Check if daemons started successfully
Start-Sleep -Seconds 3
$daemons = Get-Process -Name "mrpd","maap_daemon" -ErrorAction SilentlyContinue
if ($daemons) {
    Write-Host "✅ OpenAvnu daemons started successfully" -ForegroundColor Green
    $daemons | ForEach-Object { Write-Host "   - $($_.ProcessName) (PID: $($_.Id))" -ForegroundColor Gray }
} else {
    Write-Host "❌ Failed to start OpenAvnu daemons" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "🎵 Starting AVDECC Entity..." -ForegroundColor Cyan
Write-Host "Entity Name: $EntityName" -ForegroundColor Gray
Write-Host ""

# Start AVDECC entity in background
$avdeccJob = Start-Job -ScriptBlock {
    param($exePath, $entityName)
    & $exePath
} -ArgumentList ".\build\examples\Release\simple_avdecc_entity.exe", $EntityName

Start-Sleep -Seconds 2

# Check if AVDECC entity started
$avdeccProcess = Get-Process -Name "simple_avdecc_entity" -ErrorAction SilentlyContinue
if ($avdeccProcess) {
    Write-Host "✅ AVDECC Entity started (PID: $($avdeccProcess.Id))" -ForegroundColor Green
} else {
    Write-Host "❌ Failed to start AVDECC entity" -ForegroundColor Red
}

Write-Host ""
Write-Host "🎯 Complete OpenAvnu AVDECC System Status:" -ForegroundColor Magenta
Write-Host ""

# Show final status
$allProcesses = Get-Process -Name "mrpd","maap_daemon","simple_asio_listener","simple_avdecc_entity" -ErrorAction SilentlyContinue
if ($allProcesses) {
    Write-Host "✅ Active OpenAvnu Components:" -ForegroundColor Green
    $allProcesses | Select-Object ProcessName, Id, @{N='Memory(MB)';E={[math]::Round($_.WorkingSet/1MB,1)}} | Format-Table -AutoSize
} else {
    Write-Host "❌ No OpenAvnu processes running" -ForegroundColor Red
}

Write-Host ""
Write-Host "📋 Next Steps for Hive-AVDECC Discovery:" -ForegroundColor Cyan
Write-Host ""
Write-Host "1. Open Hive-AVDECC application" -ForegroundColor White
Write-Host "2. Click 'Refresh' or 'Discover Entities'" -ForegroundColor White
Write-Host "3. Look for 'OpenAvnu Audio Device' in the entity list" -ForegroundColor White
Write-Host "4. Your system should appear as an AVDECC entity" -ForegroundColor White
Write-Host ""
Write-Host "💡 System Information:" -ForegroundColor Yellow
Write-Host "   Network Interface: Intel I219" -ForegroundColor Gray
Write-Host "   AVDECC Entity ID: 0x0012345678901234" -ForegroundColor Gray
Write-Host "   Virtual Audio: CABLE Input/Output available" -ForegroundColor Gray
Write-Host "   Protocol Stack: OpenAvnu + AVDECC + Virtual Audio" -ForegroundColor Gray
Write-Host ""
Write-Host "🔄 To stop all components:" -ForegroundColor Yellow
Write-Host "   Get-Process mrpd,maap_daemon,simple_asio_listener,simple_avdecc_entity | Stop-Process" -ForegroundColor Cyan
Write-Host ""

# Keep monitoring
Write-Host "⚡ System is running... Press Ctrl+C to stop monitoring" -ForegroundColor Green
try {
    while ($true) {
        Start-Sleep -Seconds 10
        $runningProcesses = Get-Process -Name "mrpd","maap_daemon","simple_avdecc_entity" -ErrorAction SilentlyContinue
        if ($runningProcesses.Count -lt 3) {
            Write-Host "⚠️ Some components stopped - check system status" -ForegroundColor Yellow
            break
        }
    }
} catch {
    Write-Host ""
    Write-Host "⏹️ Monitoring stopped" -ForegroundColor Yellow
}
