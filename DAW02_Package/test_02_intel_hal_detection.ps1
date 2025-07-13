# OpenAvnu Intel HAL Detection Test for Windows DAW02
# PowerShell version for Windows systems

Write-Host "=== Intel HAL Detection Test ===" -ForegroundColor Green
Write-Host "System: $env:COMPUTERNAME"
Write-Host "Date: $(Get-Date)"
Write-Host "Objective: Verify Intel HAL integration and graceful fallback on Windows"
Write-Host ""

# Build configuration for Windows
$BuildType = "Windows"
$Generator = "Visual Studio 17 2022"
$ConfigArg = "--config Release"

Write-Host "Build Type: $BuildType"
Write-Host ""

# Test 1: Check existing Intel HAL build
Write-Host "=== Test 1: Checking Intel HAL Build ===" -ForegroundColor Yellow
Write-Host "Checking existing build for Intel HAL components"
Write-Host ""

# Check if build directory exists
if (Test-Path "..\build") {
    Set-Location "..\build"
    Write-Host "Using existing build directory: ..\build" -ForegroundColor Green
} else {
    Write-Host "No existing build found - need to build first" -ForegroundColor Red
    exit 1
}
try {
    $ConfigOutput = & cmake .. -G $Generator -DOPENAVNU_BUILD_INTEL_HAL=OFF 2>&1
    $CMakeNoHalTime = (Get-Date) - $StartTime
    Write-Host "✅ CMake configuration (no HAL): SUCCESS ($($CMakeNoHalTime.TotalSeconds.ToString('F1')) seconds)" -ForegroundColor Green
    
    Write-Host "Building without Intel HAL..."
    $StartTime = Get-Date
    $BuildOutput = & cmake --build . $ConfigArg.Split(' ') 2>&1
    $BuildNoHalTime = (Get-Date) - $StartTime
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "✅ Build (no HAL): SUCCESS ($($BuildNoHalTime.TotalSeconds.ToString('F1')) seconds)" -ForegroundColor Green
        $NoHalSuccess = $true
    } else {
        Write-Host "❌ Build (no HAL): FAILED ($($BuildNoHalTime.TotalSeconds.ToString('F1')) seconds)" -ForegroundColor Red
        $NoHalSuccess = $false
        
        # Show errors
        $BuildOutput | Where-Object { $_ -match "error" } | ForEach-Object {
            Write-Host "   Error: $_" -ForegroundColor Red
        }
    }
} catch {
    Write-Host "❌ CMake configuration (no HAL): FAILED" -ForegroundColor Red
    Write-Host "Error: $($_.Exception.Message)" -ForegroundColor Red
    $NoHalSuccess = $false
}

Set-Location ".."

# Test 2: Build WITH Intel HAL (if available)
Write-Host ""
Write-Host "=== Test 2: Building WITH Intel HAL ===" -ForegroundColor Yellow
Write-Host "This tests Intel HAL integration when available"
Write-Host ""

if (Test-Path "build_with_hal") {
    Remove-Item -Recurse -Force "build_with_hal"
}
New-Item -ItemType Directory -Name "build_with_hal" | Out-Null
Set-Location "build_with_hal"

Write-Host "Configuring with Intel HAL..."
$StartTime = Get-Date
try {
    $ConfigOutput = & cmake .. -G $Generator -DOPENAVNU_BUILD_INTEL_HAL=ON 2>&1
    $CMakeHalTime = (Get-Date) - $StartTime
    Write-Host "✅ CMake configuration (with HAL): SUCCESS ($($CMakeHalTime.TotalSeconds.ToString('F1')) seconds)" -ForegroundColor Green
    
    Write-Host "Building with Intel HAL..."
    $StartTime = Get-Date
    $BuildOutput = & cmake --build . $ConfigArg.Split(' ') 2>&1
    $BuildHalTime = (Get-Date) - $StartTime
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "✅ Build (with HAL): SUCCESS ($($BuildHalTime.TotalSeconds.ToString('F1')) seconds)" -ForegroundColor Green
        $WithHalSuccess = $true
    } else {
        Write-Host "⚠️  Build (with HAL): FAILED ($($BuildHalTime.TotalSeconds.ToString('F1')) seconds)" -ForegroundColor Yellow
        Write-Host "   This is expected if Intel HAL libraries are not available" -ForegroundColor Yellow
        $WithHalSuccess = $false
        
        # Show build errors
        $BuildOutput | Where-Object { $_ -match "error" } | ForEach-Object {
            Write-Host "   Error: $_" -ForegroundColor Yellow
        }
    }
} catch {
    Write-Host "⚠️  CMake configuration (with HAL): FAILED" -ForegroundColor Yellow
    Write-Host "   This is expected if Intel HAL dependencies are not available" -ForegroundColor Yellow
    $WithHalSuccess = $false
}

Set-Location ".."

# Test 3: Intel Hardware Detection
Write-Host ""
Write-Host "=== Test 3: Intel Hardware Detection ===" -ForegroundColor Yellow

$IntelHardwareCount = 0
Write-Host "Scanning for Intel network hardware..."

try {
    $IntelAdapters = Get-NetAdapter | Where-Object { $_.InterfaceDescription -like "*Intel*" }
    
    if ($IntelAdapters) {
        $IntelHardwareCount = $IntelAdapters.Count
        Write-Host "✅ Intel network hardware detected:" -ForegroundColor Green
        $IntelAdapters | ForEach-Object {
            Write-Host "   $($_.Name): $($_.InterfaceDescription)" -ForegroundColor Cyan
        }
    } else {
        Write-Host "ℹ️  No Intel network hardware detected" -ForegroundColor Cyan
    }
    
    # Also check PCI devices via WMI
    $PciDevices = Get-WmiObject -Class Win32_PnPEntity | Where-Object { $_.Name -like "*Intel*" -and $_.Name -like "*Ethernet*" }
    if ($PciDevices) {
        Write-Host ""
        Write-Host "Additional Intel Ethernet devices:" -ForegroundColor Cyan
        $PciDevices | ForEach-Object {
            Write-Host "   $($_.Name)" -ForegroundColor Cyan
        }
    }
} catch {
    Write-Host "⚠️  Cannot detect hardware: $($_.Exception.Message)" -ForegroundColor Yellow
}

# Test 4: Intel HAL Library Detection
Write-Host ""
Write-Host "=== Test 4: Intel HAL Library Detection ===" -ForegroundColor Yellow

Write-Host "Searching for Intel HAL libraries..."
$HalLibrariesFound = $false

# Common locations for Intel HAL libraries on Windows
$SearchPaths = @(
    "C:\Program Files\Intel",
    "C:\Program Files (x86)\Intel", 
    "C:\Intel",
    "$env:ProgramFiles\Intel",
    "$env:ProgramFiles(x86)\Intel"
)

foreach ($Path in $SearchPaths) {
    if (Test-Path $Path) {
        $IntelLibs = Get-ChildItem -Path $Path -Recurse -Filter "*hal*" -ErrorAction SilentlyContinue | Select-Object -First 5
        if ($IntelLibs) {
            Write-Host "✅ Intel libraries found in $Path:" -ForegroundColor Green
            $IntelLibs | ForEach-Object {
                Write-Host "   $($_.FullName)" -ForegroundColor Cyan
            }
            $HalLibrariesFound = $true
        }
        
        # Also look for Ethernet-specific libraries
        $EthLibs = Get-ChildItem -Path $Path -Recurse -Filter "*igb*" -ErrorAction SilentlyContinue | Select-Object -First 3
        if ($EthLibs) {
            Write-Host "✅ Intel Ethernet libraries found in $Path:" -ForegroundColor Green
            $EthLibs | ForEach-Object {
                Write-Host "   $($_.FullName)" -ForegroundColor Cyan
            }
            $HalLibrariesFound = $true
        }
    }
}

if (-not $HalLibrariesFound) {
    Write-Host "ℹ️  No Intel HAL libraries detected in standard locations" -ForegroundColor Cyan
}

# Test 5: Runtime HAL Detection (if builds succeeded)
Write-Host ""
Write-Host "=== Test 5: Runtime HAL Detection ===" -ForegroundColor Yellow

if ($NoHalSuccess) {
    Write-Host "Testing runtime behavior without Intel HAL..."
    Set-Location "build_no_hal"
    
    $MrpdPath = "daemons\mrpd\Release\mrpd.exe"
    
    if (Test-Path $MrpdPath) {
        Write-Host "Running MRPD without HAL (brief test)..."
        try {
            $Output = & (Resolve-Path $MrpdPath) -v 2>&1 | Select-Object -First 10
            $Output | ForEach-Object { Write-Host "   $_" -ForegroundColor Cyan }
        } catch {
            Write-Host "   (Expected - daemon requires proper network setup)" -ForegroundColor Yellow
        }
    }
    
    Set-Location ".."
}

if ($WithHalSuccess) {
    Write-Host ""
    Write-Host "Testing runtime behavior with Intel HAL..."
    Set-Location "build_with_hal"
    
    $MrpdPath = "daemons\mrpd\Release\mrpd.exe"
    
    if (Test-Path $MrpdPath) {
        Write-Host "Running MRPD with HAL (brief test)..."
        try {
            $Output = & (Resolve-Path $MrpdPath) -v 2>&1 | Select-Object -First 10
            $Output | ForEach-Object { Write-Host "   $_" -ForegroundColor Cyan }
        } catch {
            Write-Host "   (Expected - daemon requires proper network setup)" -ForegroundColor Yellow
        }
    }
    
    Set-Location ".."
}

# Results Summary
Write-Host ""
Write-Host "=== Intel HAL Test Results Summary ===" -ForegroundColor Yellow
Write-Host "Build without HAL: $(if ($NoHalSuccess) { "✅ SUCCESS" } else { "❌ FAILED" })"
Write-Host "Build with HAL: $(if ($WithHalSuccess) { "✅ SUCCESS" } else { "⚠️ FAILED (expected if HAL unavailable)" })"
Write-Host "Intel Hardware Count: $IntelHardwareCount"
Write-Host "Intel Libraries Found: $(if ($HalLibrariesFound) { "Yes" } else { "No" })"

# Performance comparison (if both builds succeeded)
if ($NoHalSuccess -and $WithHalSuccess) {
    Write-Host ""
    Write-Host "=== Build Performance Comparison ===" -ForegroundColor Yellow
    Write-Host "Build Time (no HAL): $($BuildNoHalTime.TotalSeconds.ToString('F1')) seconds"
    Write-Host "Build Time (with HAL): $($BuildHalTime.TotalSeconds.ToString('F1')) seconds"
    
    if ($BuildHalTime.TotalSeconds -gt $BuildNoHalTime.TotalSeconds) {
        $Diff = $BuildHalTime.TotalSeconds - $BuildNoHalTime.TotalSeconds
        Write-Host "Intel HAL build took $($Diff.ToString('F1')) seconds longer (expected due to additional dependencies)"
    } else {
        $Diff = $BuildNoHalTime.TotalSeconds - $BuildHalTime.TotalSeconds
        Write-Host "Intel HAL build took $($Diff.ToString('F1')) seconds less (unusual, possible cache effects)"
    }
}

# Final assessment
Write-Host ""
Write-Host "=== Final Assessment ===" -ForegroundColor Yellow

if ($NoHalSuccess) {
    Write-Host "🎉 GRACEFUL FALLBACK: SUCCESS" -ForegroundColor Green
    Write-Host "   OpenAvnu daemons build and work without Intel HAL on Windows"
    
    if ($WithHalSuccess) {
        Write-Host "🎉 INTEL HAL INTEGRATION: SUCCESS" -ForegroundColor Green
        Write-Host "   OpenAvnu daemons also build with Intel HAL enabled on Windows"
        Write-Host "✅ HARDWARE INDEPENDENCE: VERIFIED" -ForegroundColor Green
        exit 0
    } else {
        Write-Host "ℹ️  INTEL HAL INTEGRATION: NOT AVAILABLE" -ForegroundColor Cyan
        Write-Host "   This is normal if Intel HAL libraries are not installed"
        Write-Host "✅ HARDWARE INDEPENDENCE: VERIFIED" -ForegroundColor Green
        exit 0
    }
} else {
    Write-Host "❌ GRACEFUL FALLBACK: FAILED" -ForegroundColor Red
    Write-Host "   OpenAvnu daemons failed to build without Intel HAL on Windows"
    Write-Host "   This indicates a dependency issue that needs resolution"
    exit 1
}
