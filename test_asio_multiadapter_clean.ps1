# ============================================================================
# ASIO Listener Test with Multi-Adapter Auto-Detection (Clean CMake Version)
# ============================================================================

Write-Host ""
Write-Host "ASIO Listener Test (Multi-Adapter)" -ForegroundColor Magenta
Write-Host "====================================" -ForegroundColor Magenta
Write-Host ""

# Phase 1: Start Daemons
Write-Host "[PHASE 1] Starting OpenAvnu Daemons with Multi-Adapter Detection..." -ForegroundColor Cyan

# Check if start_openavnu_final.ps1 exists in different locations
$scriptLocations = @(
    ".\start_openavnu_final.ps1",
    "examples\asio-listener\start_openavnu_final.ps1"
)

$scriptFound = $false
foreach ($location in $scriptLocations) {
    if (Test-Path $location) {
        Write-Host "Using daemon script: $location" -ForegroundColor White
        & $location
        $scriptFound = $true
        break
    }
}

if (-not $scriptFound) {
    Write-Host "Daemon start script not found, checking if daemons are already running..." -ForegroundColor Yellow
}

Write-Host ""
Write-Host "Waiting for daemon stabilization..." -ForegroundColor Yellow
Start-Sleep -Seconds 3

# Verify daemons started
$daemons = Get-Process -Name "mrpd","maap_daemon" -ErrorAction SilentlyContinue
if ($daemons) {
    Write-Host "Daemons running:" -ForegroundColor Green
    $daemons | Select-Object ProcessName, Id | Format-Table -AutoSize
} else {
    Write-Host "Daemons failed to start - check output above" -ForegroundColor Red
    exit 1
}

# Phase 2: Build ASIO Listener using CMake
Write-Host ""
Write-Host "[PHASE 2] Building ASIO Audio Listener with CMake..." -ForegroundColor Cyan

$buildSuccess = $false
$asioExecutable = $null

# Try CMake build (preferred method)
try {
    Write-Host "Building with CMake system..." -ForegroundColor Yellow
    if (Test-Path "build") {
        $cmakeOutput = & cmake --build .\build --config Release --target simple_asio_listener 2>&1
        Write-Host "CMake output:" -ForegroundColor Gray
        $cmakeOutput | ForEach-Object { Write-Host "  $_" -ForegroundColor Gray }
        
        if (Test-Path "build\examples\Release\simple_asio_listener.exe") {
            Write-Host "SUCCESS: CMake build completed!" -ForegroundColor Green
            $asioExecutable = "build\examples\Release\simple_asio_listener.exe"
            $buildSuccess = $true
        } else {
            Write-Host "CMake build failed - executable not found" -ForegroundColor Red
        }
    } else {
        Write-Host "Build directory not found - need to configure CMake first" -ForegroundColor Red
        Write-Host "Run: cmake .. -G \"Visual Studio 17 2022\" in build directory" -ForegroundColor Yellow
    }
} catch {
    Write-Host "CMake build failed: $($_.Exception.Message)" -ForegroundColor Red
}

if (-not $buildSuccess) {
    Write-Host "Build failed - cannot continue with ASIO listener test" -ForegroundColor Red
    Write-Host "Please ensure CMake is properly configured and build directory exists" -ForegroundColor Yellow
    exit 1
}

# Phase 3: Start ASIO Listener
Write-Host ""
Write-Host "[PHASE 3] Starting ASIO Audio Listener..." -ForegroundColor Cyan

if ($asioExecutable -and (Test-Path $asioExecutable)) {
    Write-Host "Starting CMake-built ASIO Audio Listener..." -ForegroundColor Green
    Write-Host "(Press Ctrl+C to stop)" -ForegroundColor Gray
    
    # Show some info about the executable
    $fileInfo = Get-Item $asioExecutable
    Write-Host "Executable: $asioExecutable ($([math]::Round($fileInfo.Length/1KB,2)) KB)" -ForegroundColor Gray
    Write-Host "Modified: $($fileInfo.LastWriteTime)" -ForegroundColor Gray
    Write-Host ""
    
    try {
        & $asioExecutable
    } catch {
        Write-Host "ASIO Listener stopped: $($_.Exception.Message)" -ForegroundColor Yellow
    }
} else {
    Write-Host "ASIO Listener executable not found at: $asioExecutable" -ForegroundColor Red
}

Write-Host ""
Write-Host "ASIO Listener Test Complete!" -ForegroundColor Magenta
Write-Host "Daemons are still running - use 'Get-Process mrpd,maap_daemon | Stop-Process' to stop them" -ForegroundColor Gray
