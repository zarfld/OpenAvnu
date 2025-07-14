# ============================================================================
# ASIO Listener Test with Multi-Adapter Auto-Detection
# ============================================================================

Write-Host ""
Write-Host "ASIO Listener Test (Multi-Adapter)" -ForegroundColor Magenta
Write-Host "=====================================" -ForegroundColor Magenta
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
Write-Host "⏳ Waiting for daemon stabilization..." -ForegroundColor Yellow
Start-Sleep -Seconds 3

# Verify daemons started
$daemons = Get-Process -Name "mrpd","maap_daemon" -ErrorAction SilentlyContinue
if ($daemons) {
    Write-Host "✅ Daemons running:" -ForegroundColor Green
    $daemons | Select-Object ProcessName, Id | Format-Table -AutoSize
} else {
    Write-Host "❌ Daemons failed to start - check output above" -ForegroundColor Red
    exit 1
}

# Phase 2: Build ASIO Listener
Write-Host ""
Write-Host "[PHASE 2] Building ASIO Audio Listener..." -ForegroundColor Cyan
Set-Location "examples\asio-listener"

# Check what build options are available
Write-Host "🔍 Checking build options..." -ForegroundColor White

$buildSuccess = $false

# Try simple build first
Write-Host "🔨 Trying simple build (build_simple.bat)..." -ForegroundColor Yellow
try {
    & ".\build_simple.bat"
    if (Test-Path "asio_listener.exe") {
        Write-Host "✅ Simple build successful" -ForegroundColor Green
        $buildSuccess = $true
    }
} catch {
    Write-Host "⚠️  Simple build failed: $($_.Exception.Message)" -ForegroundColor Yellow
}

# Try CMake build if simple build didn't work
if (-not $buildSuccess) {
    Write-Host "🔨 Attempting CMake build..." -ForegroundColor Yellow
    $cmakeResult = & cmake --build "..\..\build" --config Release 2>&1
    if ($LASTEXITCODE -eq 0) {
        Write-Host "✅ CMake build successful" -ForegroundColor Green
        $buildSuccess = $true
    } else {
        Write-Host "⚠️  CMake build had issues:" -ForegroundColor Yellow
        Write-Host $cmakeResult -ForegroundColor Gray
    }
}

# If still no success, try manual compilation
if (-not $buildSuccess) {
    Write-Host "🔨 Trying manual compilation..." -ForegroundColor Yellow
    & ".\manual_compile.ps1"
    if (Test-Path "asio_listener.exe") {
        Write-Host "✅ Manual compilation successful" -ForegroundColor Green
        $buildSuccess = $true
    }
}

# If CMake didn't work, try build.bat
if (-not $buildSuccess -and (Test-Path "build.bat")) {
    Write-Host "🔨 Trying build.bat..." -ForegroundColor Yellow
    try {
        & ".\build.bat"
        if (Test-Path "asio_listener.exe") {
            Write-Host "✅ build.bat successful" -ForegroundColor Green
            $buildSuccess = $true
        }
    } catch {
        Write-Host "⚠️  build.bat failed: $($_.Exception.Message)" -ForegroundColor Yellow
    }
}

# If still no success, try simple compilation
if (-not $buildSuccess -and (Test-Path "asio_listener.c")) {
    Write-Host "🔨 Attempting simple gcc compilation..." -ForegroundColor Yellow
    try {
        & gcc -o asio_listener.exe asio_listener.c -lws2_32 2>&1
        if (Test-Path "asio_listener.exe") {
            Write-Host "✅ Simple compilation successful" -ForegroundColor Green
            $buildSuccess = $true
        }
    } catch {
        Write-Host "⚠️  Simple compilation failed" -ForegroundColor Yellow
    }
}

if (-not $buildSuccess) {
    Write-Host "⚠️  Build unsuccessful, checking for existing executable..." -ForegroundColor Yellow
    if (Test-Path "asio_listener.exe") {
        Write-Host "✅ Found existing asio_listener.exe" -ForegroundColor Green
        $buildSuccess = $true
    }
}

# Phase 3: Start ASIO Listener
Write-Host ""
Write-Host "[PHASE 3] Starting ASIO Audio Listener..." -ForegroundColor Cyan

if (Test-Path "asio_listener.exe") {
    Write-Host "🎵 Starting ASIO Audio Listener..." -ForegroundColor Green
    Write-Host "   (Press Ctrl+C to stop)" -ForegroundColor Gray
    
    # Show some info about the executable
    $fileInfo = Get-Item "asio_listener.exe"
    Write-Host "   Executable: $([math]::Round($fileInfo.Length/1KB,2)) KB, Modified: $($fileInfo.LastWriteTime)" -ForegroundColor Gray
    
    try {
        & ".\asio_listener.exe"
    } catch {
        Write-Host "⚠️  ASIO Listener stopped: $($_.Exception.Message)" -ForegroundColor Yellow
    }
} else {
    Write-Host "❌ ASIO Listener executable not found" -ForegroundColor Red
    Write-Host "🔍 Available files:" -ForegroundColor Gray
    Get-ChildItem -Name "*.exe" | ForEach-Object { Write-Host "   $_" -ForegroundColor Gray }
    Get-ChildItem -Name "*.c" | ForEach-Object { Write-Host "   $_ (source)" -ForegroundColor Blue }
}

# Return to workspace root
Set-Location "..\..\"

Write-Host ""
Write-Host "ASIO Listener Test Complete!" -ForegroundColor Magenta
