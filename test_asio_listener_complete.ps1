# ============================================================================
# Complete ASIO Listener Test Workflow with Auto-Daemon Startup
# ============================================================================

Write-Host ""
Write-Host "🎵 ASIO Listener Complete Test Workflow 🎵" -ForegroundColor Magenta
Write-Host "=============================================" -ForegroundColor Magenta
Write-Host ""

# Phase 1: Environment Verification
Write-Host "[PHASE 1] Environment Verification" -ForegroundColor Cyan
Write-Host "──────────────────────────────────" -ForegroundColor Cyan

# Check Intel adapters
Write-Host "🔍 Detecting Intel Network Adapters..." -ForegroundColor White
$intelAdapters = Get-NetAdapter | Where-Object { $_.InterfaceDescription -like '*Intel*' }
if ($intelAdapters) {
    $intelAdapters | Select-Object Name, InterfaceDescription, Status, LinkSpeed | Format-Table -AutoSize
    Write-Host "✅ Intel adapters found: $($intelAdapters.Count)" -ForegroundColor Green
} else {
    Write-Host "❌ No Intel adapters found" -ForegroundColor Red
    exit 1
}

# Check OpenAvnu executables
Write-Host "🔍 Checking OpenAvnu executables..." -ForegroundColor White
$daemons = @{
    "MRPD" = "daemons\mrpd\mrpd.exe"
    "MAAP" = "daemons\maap\daemon\maap_daemon.exe"
}

$missingDaemons = 0
foreach ($daemon in $daemons.GetEnumerator()) {
    if (Test-Path $daemon.Value) {
        Write-Host "✅ $($daemon.Key): $($daemon.Value)" -ForegroundColor Green
    } else {
        Write-Host "❌ $($daemon.Key): $($daemon.Value) [MISSING]" -ForegroundColor Red
        $missingDaemons++
    }
}

if ($missingDaemons -gt 0) {
    Write-Host "⚠️  Building missing daemons..." -ForegroundColor Yellow
    & cmake --build build --config Release
    Start-Sleep -Seconds 2
}

# Check PCAP environment
Write-Host "🔍 Checking PCAP environment..." -ForegroundColor White
if ($env:NPCAP_DIR -or $env:NPCAP_SDK_DIR -or $env:WPCAP_DIR) {
    Write-Host "✅ PCAP environment configured" -ForegroundColor Green
    if ($env:NPCAP_DIR) { Write-Host "   NPCAP_DIR: $env:NPCAP_DIR" -ForegroundColor Gray }
    if ($env:NPCAP_SDK_DIR) { Write-Host "   NPCAP_SDK_DIR: $env:NPCAP_SDK_DIR" -ForegroundColor Gray }
    if ($env:WPCAP_DIR) { Write-Host "   WPCAP_DIR: $env:WPCAP_DIR" -ForegroundColor Gray }
} else {
    Write-Host "⚠️  PCAP environment not explicitly set (may still work with auto-detection)" -ForegroundColor Yellow
}

Write-Host ""
Start-Sleep -Seconds 2

# Phase 2: Daemon Startup
Write-Host "[PHASE 2] Starting OpenAvnu Daemons" -ForegroundColor Cyan
Write-Host "───────────────────────────────────" -ForegroundColor Cyan

Write-Host "🚀 Starting daemons with multi-adapter detection..." -ForegroundColor White
& ".\start_openavnu_final.ps1"

Write-Host "⏳ Waiting for daemon stabilization..." -ForegroundColor White
Start-Sleep -Seconds 5

# Verify daemons are running
Write-Host "🔍 Verifying daemon status..." -ForegroundColor White
$runningDaemons = Get-Process -Name "mrpd","maap_daemon" -ErrorAction SilentlyContinue
if ($runningDaemons) {
    $runningDaemons | Select-Object ProcessName, Id, @{Name='Memory(MB)';Expression={[math]::Round($_.WorkingSet/1MB,2)}} | Format-Table -AutoSize
    Write-Host "✅ Daemons running successfully" -ForegroundColor Green
} else {
    Write-Host "❌ Daemons failed to start" -ForegroundColor Red
    exit 1
}

Write-Host ""
Start-Sleep -Seconds 2

# Phase 3: ASIO Listener Build
Write-Host "[PHASE 3] Building ASIO Audio Listener" -ForegroundColor Cyan
Write-Host "──────────────────────────────────────" -ForegroundColor Cyan

Set-Location "examples\asio-listener"

Write-Host "🔨 Checking for existing ASIO Listener..." -ForegroundColor White
if (Test-Path "asio_listener.exe") {
    Write-Host "✅ ASIO Listener already built" -ForegroundColor Green
} else {
    Write-Host "🔨 Building ASIO Listener..." -ForegroundColor White
    
    if (Test-Path "build.bat") {
        Write-Host "   Using build.bat..." -ForegroundColor Gray
        & ".\build.bat"
    } else {
        Write-Host "   Using CMake..." -ForegroundColor Gray
        & cmake --build "..\..\build" --target asio_listener --config Release
    }
    
    if (Test-Path "asio_listener.exe") {
        Write-Host "✅ ASIO Listener built successfully" -ForegroundColor Green
    } else {
        Write-Host "❌ ASIO Listener build failed" -ForegroundColor Red
        Set-Location "..\.."; exit 1
    }
}

Write-Host ""
Start-Sleep -Seconds 2

# Phase 4: ASIO Listener Test
Write-Host "[PHASE 4] Testing ASIO Audio Listener" -ForegroundColor Cyan
Write-Host "─────────────────────────────────────" -ForegroundColor Cyan

Write-Host "🎵 Starting ASIO Audio Listener..." -ForegroundColor White
Write-Host "   (Press Ctrl+C to stop the listener)" -ForegroundColor Gray
Write-Host ""

# Start ASIO Listener with error handling
try {
    if (Test-Path "asio_listener.exe") {
        & ".\asio_listener.exe"
    } else {
        Write-Host "❌ ASIO Listener executable not found" -ForegroundColor Red
    }
} catch {
    Write-Host "⚠️  ASIO Listener stopped or encountered an error: $($_.Exception.Message)" -ForegroundColor Yellow
} finally {
    Write-Host ""
    Write-Host "🔄 Returning to workspace root..." -ForegroundColor White
    Set-Location "..\..\"
}

# Phase 5: Cleanup
Write-Host ""
Write-Host "[PHASE 5] Test Complete - Cleanup Options" -ForegroundColor Cyan
Write-Host "─────────────────────────────────────────" -ForegroundColor Cyan

Write-Host "🔍 Current daemon status:" -ForegroundColor White
$finalDaemons = Get-Process -Name "mrpd","maap_daemon" -ErrorAction SilentlyContinue
if ($finalDaemons) {
    $finalDaemons | Select-Object ProcessName, Id | Format-Table -AutoSize
    
    Write-Host "Would you like to stop the daemons? (y/N): " -NoNewline -ForegroundColor Yellow
    $response = Read-Host
    
    if ($response -eq "y" -or $response -eq "Y") {
        Write-Host "🛑 Stopping daemons..." -ForegroundColor Red
        $finalDaemons | ForEach-Object { 
            Write-Host "   Stopping $($_.ProcessName) (PID: $($_.Id))" -ForegroundColor Gray
            Stop-Process -Id $_.Id -Force 
        }
        Start-Sleep -Seconds 2
        Write-Host "✅ All daemons stopped" -ForegroundColor Green
    } else {
        Write-Host "✅ Daemons left running for continued testing" -ForegroundColor Green
    }
} else {
    Write-Host "ℹ️  No daemons currently running" -ForegroundColor Blue
}

Write-Host ""
Write-Host "🎉 ASIO Listener Test Workflow Complete! 🎉" -ForegroundColor Magenta
Write-Host ""
