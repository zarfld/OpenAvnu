# ============================================================================
# ASIO Listener Test with Multi-Adapter Auto-Detection (Clean Version)
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

# Phase 2: Build ASIO Listener
Write-Host ""
Write-Host "[PHASE 2] Building ASIO Audio Listener..." -ForegroundColor Cyan

$buildSuccess = $false
$originalLocation = Get-Location

# Try CMake build first (preferred method)
Write-Host "Attempting CMake build (recommended)..." -ForegroundColor Yellow
try {
    Set-Location "..\.."
    if (Test-Path "build") {
        $cmakeOutput = & cmake --build .\build --config Release --target simple_asio_listener 2>&1
        Write-Host "CMake build output:" -ForegroundColor Gray
        Write-Host $cmakeOutput -ForegroundColor Gray
        
        if (Test-Path "build\examples\Release\simple_asio_listener.exe") {
            Write-Host "CMake build successful" -ForegroundColor Green
            $asioExecutable = "build\examples\Release\simple_asio_listener.exe"
            $buildSuccess = $true
        } else {
            Write-Host "CMake build failed - executable not found" -ForegroundColor Yellow
        }
    } else {
        Write-Host "Build directory not found - need to configure CMake first" -ForegroundColor Yellow
    }
} catch {
    Write-Host "CMake build failed: $($_.Exception.Message)" -ForegroundColor Yellow
}

# Fallback to old build system if CMake failed
if (-not $buildSuccess) {
    Write-Host "Falling back to legacy build system..." -ForegroundColor Yellow
    Set-Location $originalLocation
    Set-Location "examples\asio-listener"
    
    # Check what build options are available
    Write-Host "Checking legacy build options..." -ForegroundColor White
    
    # Try simple build first
    Write-Host "Trying simple build (build_simple.bat)..." -ForegroundColor Yellow
    try {
        & ".\build_simple.bat"
        if (Test-Path "asio_listener.exe") {
            Write-Host "Simple build successful" -ForegroundColor Green
            $buildSuccess = $true
        }
    } catch {
        Write-Host "Simple build failed: $($_.Exception.Message)" -ForegroundColor Yellow
    }
}

# Try manual compilation if simple build didn't work
if (-not $buildSuccess) {
    Write-Host "Trying manual compilation..." -ForegroundColor Yellow
    if (Test-Path "manual_compile_clean.ps1") {
        & ".\manual_compile_clean.ps1"
        if (Test-Path "asio_listener.exe") {
            Write-Host "Manual compilation successful" -ForegroundColor Green
            $buildSuccess = $true
        }
    }
}

# Try direct gcc compilation first
if (-not $buildSuccess -and (Test-Path "simple_asio_listener.c")) {
    Write-Host "Trying direct gcc compilation..." -ForegroundColor Yellow
    try {
        $gccOutput = & gcc -o asio_listener.exe simple_asio_listener.c -lws2_32 2>&1
        if (Test-Path "asio_listener.exe") {
            Write-Host "Direct gcc compilation successful" -ForegroundColor Green
            $buildSuccess = $true
        } else {
            Write-Host "gcc output: $gccOutput" -ForegroundColor Gray
        }
    } catch {
        Write-Host "gcc not available: $($_.Exception.Message)" -ForegroundColor Yellow
    }
}

# Try CMake build if direct compilation didn't work
if (-not $buildSuccess) {
    Write-Host "Attempting CMake build..." -ForegroundColor Yellow
    if (Test-Path "..\..\build") {
        $cmakeResult = & cmake --build "..\..\build" --config Release 2>&1
        if ($LASTEXITCODE -eq 0) {
            Write-Host "CMake build successful" -ForegroundColor Green
            # Look for created executables
            Get-ChildItem "..\..\build" -Recurse -Name "*.exe" | ForEach-Object {
                Write-Host "Found executable: $_" -ForegroundColor Gray
            }
            $buildSuccess = $true
        } else {
            Write-Host "CMake build had issues:" -ForegroundColor Yellow
            Write-Host $cmakeResult -ForegroundColor Gray
        }
    } else {
        Write-Host "Build directory not found" -ForegroundColor Yellow
    }
}

if (-not $buildSuccess) {
    Write-Host "Build unsuccessful, checking for existing executable..." -ForegroundColor Yellow
    if (Test-Path "asio_listener.exe") {
        Write-Host "Found existing asio_listener.exe" -ForegroundColor Green
        $buildSuccess = $true
    }
}

# Phase 3: Start ASIO Listener
Write-Host ""
Write-Host "[PHASE 3] Starting ASIO Audio Listener..." -ForegroundColor Cyan

# Check for CMake-built executable first
if ($buildSuccess -and (Test-Path "build\examples\Release\simple_asio_listener.exe")) {
    Write-Host "Starting CMake-built ASIO Audio Listener..." -ForegroundColor Green
    Write-Host "(Press Ctrl+C to stop)" -ForegroundColor Gray
    
    # Show some info about the executable
    $fileInfo = Get-Item "build\examples\Release\simple_asio_listener.exe"
    Write-Host "Executable: $([math]::Round($fileInfo.Length/1KB,2)) KB, Modified: $($fileInfo.LastWriteTime)" -ForegroundColor Gray
    
    try {
        & ".\build\examples\Release\simple_asio_listener.exe"
    } catch {
        Write-Host "ASIO Listener stopped: $($_.Exception.Message)" -ForegroundColor Yellow
    }
} elseif (Test-Path "examples\asio-listener\asio_listener.exe") {
    # Fallback to legacy executable
    Write-Host "Starting legacy ASIO Audio Listener..." -ForegroundColor Green
    Write-Host "(Press Ctrl+C to stop)" -ForegroundColor Gray
    
    Set-Location "examples\asio-listener"
    try {
        & ".\asio_listener.exe"
    } catch {
        Write-Host "ASIO Listener stopped: $($_.Exception.Message)" -ForegroundColor Yellow
    }
    Set-Location "..\..\"
} else {
    Write-Host "ASIO Listener executable not found" -ForegroundColor Red
    Write-Host "Available files:" -ForegroundColor Gray
    if (Test-Path "build\examples\Release") {
        Get-ChildItem "build\examples\Release" -Name "*.exe" | ForEach-Object { Write-Host "  build\examples\Release\$_" -ForegroundColor Gray }
    }
    if (Test-Path "examples\asio-listener") {
        Get-ChildItem "examples\asio-listener" -Name "*.exe" | ForEach-Object { Write-Host "  examples\asio-listener\$_" -ForegroundColor Gray }
        Get-ChildItem "examples\asio-listener" -Name "*.c" | ForEach-Object { Write-Host "  $_ (source)" -ForegroundColor Blue }
    }
}

# Return to workspace root
Set-Location "..\..\"

Write-Host ""
Write-Host "ASIO Listener Test Complete!" -ForegroundColor Magenta
