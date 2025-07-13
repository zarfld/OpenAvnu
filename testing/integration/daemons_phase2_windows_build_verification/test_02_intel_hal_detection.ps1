# OpenAvnu Intel HAL Detection Test - Fixed Version
# Tests: Intel hardware detection, HAL integration, software fallback

param(
    [string]$BuildDir = "build",
    [switch]$Verbose = $false
)

Write-Host ""
Write-Host "=============================================" -ForegroundColor Cyan
Write-Host "  OpenAvnu Intel HAL Detection Test"
Write-Host "=============================================" -ForegroundColor Cyan
Write-Host ""

# Navigate to OpenAvnu root directory
$OpenAvnuRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
while (-not (Test-Path (Join-Path $OpenAvnuRoot "CMakeLists.txt"))) {
    $Parent = Split-Path -Parent $OpenAvnuRoot
    if ($Parent -eq $OpenAvnuRoot) {
        Write-Host "ERROR: Could not find OpenAvnu root directory" -ForegroundColor Red
        exit 1
    }
    $OpenAvnuRoot = $Parent
}

Set-Location $OpenAvnuRoot
Write-Host "OpenAvnu Root: $OpenAvnuRoot" -ForegroundColor Yellow

$BuildPath = Join-Path $OpenAvnuRoot $BuildDir
if (-not (Test-Path $BuildPath)) {
    Write-Host "ERROR: Build directory not found. Run build verification first." -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "Step 1: Intel Hardware Detection" -ForegroundColor White
Write-Host "--------------------------------"

# Check for Intel network adapters
try {
    $IntelAdapters = Get-WmiObject -Class Win32_NetworkAdapter | Where-Object { 
        $_.Name -like "*Intel*" -and $_.Name -notlike "*Virtual*" -and $_.Name -notlike "*Bluetooth*"
    }
    
    if ($IntelAdapters) {
        Write-Host "DETECTED: Intel network adapters found:" -ForegroundColor Green
        foreach ($Adapter in $IntelAdapters) {
            Write-Host "  - $($Adapter.Name)" -ForegroundColor Gray
        }
        $IntelHardwarePresent = $true
    } else {
        Write-Host "INFO: No Intel network adapters detected" -ForegroundColor Yellow
        $IntelHardwarePresent = $false
    }
} catch {
    Write-Host "WARNING: Could not scan for Intel hardware - $($_.Exception.Message)" -ForegroundColor Yellow
    $IntelHardwarePresent = $false
}

Write-Host ""
Write-Host "Step 2: Intel HAL Library Detection" -ForegroundColor White
Write-Host "-----------------------------------"

# Check for Intel HAL libraries
$IntelHALPaths = @(
    "${env:ProgramFiles}\Intel\Ethernet HAL",
    "${env:ProgramFiles(x86)}\Intel\Ethernet HAL",
    "${env:INTEL_HAL_ROOT}",
    "C:\Intel\Ethernet HAL"
)

$IntelHALFound = $false
foreach ($Path in $IntelHALPaths) {
    if ($Path -and (Test-Path $Path)) {
        Write-Host "DETECTED: Intel HAL libraries found at $Path" -ForegroundColor Green
        $IntelHALFound = $true
        break
    }
}

if (-not $IntelHALFound) {
    Write-Host "INFO: Intel HAL libraries not found (using software fallback)" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "Step 3: Software Fallback Verification" -ForegroundColor White
Write-Host "--------------------------------------"

Set-Location $BuildPath

# Test build without Intel HAL (software fallback)
Write-Host "Testing software fallback build..." -ForegroundColor Yellow

try {
    # Clean configure without Intel HAL
    & cmake .. -G "Visual Studio 17 2022" -DINTEL_HAL_ENABLED=OFF 2>&1 | Out-Null
    if ($LASTEXITCODE -eq 0) {
        Write-Host "PASSED: CMake configuration without Intel HAL successful" -ForegroundColor Green
        
        # Test build one daemon to verify software fallback
        & cmake --build . --config Release --target mrpd 2>&1 | Out-Null
        if ($LASTEXITCODE -eq 0) {
            Write-Host "PASSED: Software fallback build successful" -ForegroundColor Green
            $SoftwareFallback = "PASSED"
        } else {
            Write-Host "FAILED: Software fallback build failed" -ForegroundColor Red
            $SoftwareFallback = "FAILED"
        }
    } else {
        Write-Host "FAILED: CMake configuration without Intel HAL failed" -ForegroundColor Red
        $SoftwareFallback = "FAILED"
    }
} catch {
    Write-Host "ERROR: Software fallback test failed - $($_.Exception.Message)" -ForegroundColor Red
    $SoftwareFallback = "ERROR"
}

Write-Host ""
Write-Host "Step 4: Intel HAL Integration Test" -ForegroundColor White
Write-Host "----------------------------------"

if ($IntelHALFound) {
    Write-Host "Testing Intel HAL integration build..." -ForegroundColor Yellow
    
    try {
        # Configure with Intel HAL enabled
        & cmake .. -G "Visual Studio 17 2022" -DINTEL_HAL_ENABLED=ON 2>&1 | Out-Null
        if ($LASTEXITCODE -eq 0) {
            Write-Host "PASSED: CMake configuration with Intel HAL successful" -ForegroundColor Green
            
            # Test build with Intel HAL
            & cmake --build . --config Release --target mrpd 2>&1 | Out-Null
            if ($LASTEXITCODE -eq 0) {
                Write-Host "PASSED: Intel HAL integration build successful" -ForegroundColor Green
                $IntelHALIntegration = "PASSED"
            } else {
                Write-Host "FAILED: Intel HAL integration build failed" -ForegroundColor Red
                $IntelHALIntegration = "FAILED"
            }
        } else {
            Write-Host "FAILED: CMake configuration with Intel HAL failed" -ForegroundColor Red
            $IntelHALIntegration = "FAILED"
        }
    } catch {
        Write-Host "ERROR: Intel HAL integration test failed - $($_.Exception.Message)" -ForegroundColor Red
        $IntelHALIntegration = "ERROR"
    }
} else {
    Write-Host "SKIPPED: Intel HAL libraries not available" -ForegroundColor Yellow
    $IntelHALIntegration = "SKIPPED"
}

Write-Host ""
Write-Host "Step 5: Network Interface Analysis" -ForegroundColor White
Write-Host "----------------------------------"

try {
    $NetworkAdapters = Get-WmiObject -Class Win32_NetworkAdapterConfiguration | Where-Object { $_.IPEnabled -eq $true }
    Write-Host "Active network interfaces:" -ForegroundColor Yellow
    
    foreach ($Adapter in $NetworkAdapters) {
        $AdapterInfo = Get-WmiObject -Class Win32_NetworkAdapter | Where-Object { $_.Index -eq $Adapter.Index }
        Write-Host "  - $($AdapterInfo.Name)" -ForegroundColor Gray
        if ($AdapterInfo.Name -like "*Intel*") {
            Write-Host "    * Intel adapter - TSN features may be available" -ForegroundColor Green
        }
    }
} catch {
    Write-Host "WARNING: Could not analyze network interfaces - $($_.Exception.Message)" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "=============================================" -ForegroundColor Cyan
Write-Host "  INTEL HAL DETECTION SUMMARY"
Write-Host "=============================================" -ForegroundColor Cyan

Write-Host ""
Write-Host "Hardware Detection:" -ForegroundColor White
Write-Host "  Intel Network Adapters: $(if ($IntelHardwarePresent) { 'DETECTED' } else { 'NOT FOUND' })" -ForegroundColor $(if ($IntelHardwarePresent) { 'Green' } else { 'Yellow' })
Write-Host "  Intel HAL Libraries: $(if ($IntelHALFound) { 'AVAILABLE' } else { 'NOT AVAILABLE' })" -ForegroundColor $(if ($IntelHALFound) { 'Green' } else { 'Yellow' })

Write-Host ""
Write-Host "Integration Testing:" -ForegroundColor White
$SoftwareColor = if ($SoftwareFallback -eq "PASSED") { "Green" } elseif ($SoftwareFallback -eq "FAILED") { "Red" } else { "Yellow" }
Write-Host "  Software Fallback: $SoftwareFallback" -ForegroundColor $SoftwareColor

$HALColor = if ($IntelHALIntegration -eq "PASSED") { "Green" } elseif ($IntelHALIntegration -eq "FAILED") { "Red" } else { "Yellow" }
Write-Host "  Intel HAL Integration: $IntelHALIntegration" -ForegroundColor $HALColor

Write-Host ""
Write-Host "Deployment Recommendation:" -ForegroundColor White
if ($SoftwareFallback -eq "PASSED") {
    if ($IntelHALIntegration -eq "PASSED") {
        Write-Host "EXCELLENT: Full Intel HAL support with software fallback" -ForegroundColor Green
        Write-Host "OpenAvnu can utilize Intel TSN features when available" -ForegroundColor Green
    } else {
        Write-Host "GOOD: Software fallback working, Intel features unavailable" -ForegroundColor Yellow
        Write-Host "OpenAvnu will work but without Intel TSN acceleration" -ForegroundColor Yellow
    }
    exit 0
} else {
    Write-Host "ISSUE: Software fallback failed - check dependencies" -ForegroundColor Red
    exit 1
}
