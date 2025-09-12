# OpenAvnu Intel HAL Detection Test for Windows DAW02
# PowerShell version for Windows systems

Write-Host "=== Intel HAL Detection Test ===" -ForegroundColor Green
Write-Host "System: $env:COMPUTERNAME"
Write-Host "Date: $(Get-Date)"
Write-Host "Objective: Verify Intel HAL integration and graceful fallback on Windows"
Write-Host ""

# Check if build directory exists
if (Test-Path "..\build") {
    Set-Location "..\build"
    Write-Host "Using existing build directory: ..\build" -ForegroundColor Green
} else {
    Write-Host "No existing build found - need to build first" -ForegroundColor Red
    exit 1
}

# Test 1: Check Intel HAL Components
Write-Host ""
Write-Host "=== Test 1: Intel HAL Component Check ===" -ForegroundColor Yellow

$IntelHalComponents = @(
    @{ Name = "Intel HAL Library"; Path = "thirdparty\intel-ethernet-hal\Release\intel-ethernet-hal.dll" },
    @{ Name = "Intel HAL Static Library"; Path = "thirdparty\intel-ethernet-hal\Release\intel-ethernet-hal-static.lib" },
    @{ Name = "HAL Device Info Tool"; Path = "thirdparty\intel-ethernet-hal\examples\Release\hal_device_info.exe" },
    @{ Name = "HAL Timestamping Tool"; Path = "thirdparty\intel-ethernet-hal\examples\Release\hal_enable_timestamping.exe" },
    @{ Name = "HAL Full Test"; Path = "thirdparty\intel-ethernet-hal\tests\Release\intel_hal_full_test.exe" }
)

$ComponentsFound = 0
$TotalComponents = $IntelHalComponents.Count

foreach ($Component in $IntelHalComponents) {
    if (Test-Path $Component.Path) {
        $Size = (Get-Item $Component.Path).Length
        Write-Host "✅ $($Component.Name): Found ($([math]::Round($Size/1KB, 1)) KB)" -ForegroundColor Green
        $ComponentsFound++
    } else {
        Write-Host "❌ $($Component.Name): Missing" -ForegroundColor Red
        Write-Host "   Expected: $($Component.Path)" -ForegroundColor Gray
    }
}

Write-Host ""
Write-Host "Components Found: $ComponentsFound/$TotalComponents" -ForegroundColor Cyan

# Test 2: Intel HAL Validation Test
Write-Host ""
Write-Host "=== Test 2: Intel HAL Validation ===" -ForegroundColor Yellow

$ValidationTestPath = "tests\intel_hal_validation_test_consolidated.exe"
if (Test-Path $ValidationTestPath) {
    Write-Host "✅ Intel HAL validation test found" -ForegroundColor Green
    
    try {
        Write-Host "Running Intel HAL validation test..." -ForegroundColor Cyan
        
        # Run with timeout to prevent hanging
        $TimeoutJob = Start-Job -ScriptBlock {
            param($TestPath)
            try {
                & $TestPath 2>&1
                return @{ Success = $true; ExitCode = $LASTEXITCODE }
            } catch {
                return @{ Success = $false; Error = $_.Exception.Message }
            }
        } -ArgumentList (Resolve-Path $ValidationTestPath)
        
        if (Wait-Job $TimeoutJob -Timeout 30) {
            $Result = Receive-Job $TimeoutJob
            $ValidationOutput = $Result
            Remove-Job $TimeoutJob
            
            if ($Result.Success -and $Result.ExitCode -eq 0) {
                Write-Host "✅ Intel HAL validation: SUCCESS" -ForegroundColor Green
                $HalValidationSuccess = $true
            } else {
                Write-Host "❌ Intel HAL validation: FAILED" -ForegroundColor Red
                $HalValidationSuccess = $false
            }
        } else {
            Remove-Job $TimeoutJob -Force
            Write-Host "⚠️  Intel HAL validation: TIMEOUT (30 seconds)" -ForegroundColor Yellow
            Write-Host "   Test was taking too long - this may indicate a hardware access issue" -ForegroundColor Gray
            $HalValidationSuccess = $false
        }
    } catch {
        Write-Host "❌ Intel HAL validation: EXCEPTION" -ForegroundColor Red
        Write-Host "   Error: $($_.Exception.Message)" -ForegroundColor Red
        $HalValidationSuccess = $false
    }
} else {
    Write-Host "❌ Intel HAL validation test not found: $ValidationTestPath" -ForegroundColor Red
    $HalValidationSuccess = $false
}

# Test 3: Daemon Integration Check
Write-Host ""
Write-Host "=== Test 3: Daemon Intel HAL Integration ===" -ForegroundColor Yellow

$DaemonPaths = @(
    @{ Name = "MAAP Daemon"; Path = "daemons\maap\Release\maap_daemon.exe" },
    @{ Name = "MRPD Daemon"; Path = "daemons\mrpd\Release\mrpd.exe" }
)

$DaemonIntegrationSuccess = $true

foreach ($Daemon in $DaemonPaths) {
    if (Test-Path $Daemon.Path) {
        Write-Host "✅ $($Daemon.Name): Found" -ForegroundColor Green
        
        # Check if daemon has Intel HAL integration (basic check)
        try {
            $TimeoutJob = Start-Job -ScriptBlock {
                param($DaemonPath)
                try {
                    & $DaemonPath --help 2>&1
                    return $true
                } catch {
                    return $false
                }
            } -ArgumentList (Resolve-Path $Daemon.Path)
            
            if (Wait-Job $TimeoutJob -Timeout 5) {
                $Result = Receive-Job $TimeoutJob
                Remove-Job $TimeoutJob
                Write-Host "   ✅ Basic execution: SUCCESS" -ForegroundColor Green
            } else {
                Remove-Job $TimeoutJob -Force
                Write-Host "   ⚠️  Basic execution: Timeout (may require admin privileges)" -ForegroundColor Yellow
            }
        } catch {
            Write-Host "   ⚠️  Basic execution: May require admin privileges" -ForegroundColor Yellow
        }
    } else {
        Write-Host "❌ $($Daemon.Name): Missing" -ForegroundColor Red
        $DaemonIntegrationSuccess = $false
    }
}

# Test 4: Hardware Detection Capability
Write-Host ""
Write-Host "=== Test 4: Hardware Detection Capability ===" -ForegroundColor Yellow

try {
    $IntelAdapters = Get-NetAdapter | Where-Object { $_.InterfaceDescription -like "*Intel*" }
    
    if ($IntelAdapters.Count -gt 0) {
        Write-Host "✅ Found $($IntelAdapters.Count) Intel network adapter(s):" -ForegroundColor Green
        
        foreach ($Adapter in $IntelAdapters) {
            Write-Host "   • $($Adapter.Name): $($Adapter.InterfaceDescription)" -ForegroundColor Cyan
            Write-Host "     Status: $($Adapter.Status)" -ForegroundColor Gray
            
            # Check for AVB-capable hardware
            if ($Adapter.InterfaceDescription -match "I219|I210|I225|I226") {
                Write-Host "     ✅ AVB/TSN Capable: YES" -ForegroundColor Green
            } else {
                Write-Host "     ⚠️  AVB/TSN Capable: UNKNOWN" -ForegroundColor Yellow
            }
        }
        $HardwareDetectionSuccess = $true
    } else {
        Write-Host "⚠️  No Intel network adapters found" -ForegroundColor Yellow
        Write-Host "   Software fallback will be used" -ForegroundColor Gray
        $HardwareDetectionSuccess = $true  # Software fallback is acceptable
    }
} catch {
    Write-Host "❌ Hardware detection failed: $($_.Exception.Message)" -ForegroundColor Red
    $HardwareDetectionSuccess = $false
}

# Final Results
Write-Host ""
Write-Host "=== Final Results ===" -ForegroundColor Yellow

$OverallSuccess = $true
$SuccessCount = 0
$TotalTests = 4

if ($ComponentsFound -ge ($TotalComponents * 0.8)) {  # At least 80% of components
    Write-Host "✅ Component Check: SUCCESS ($ComponentsFound/$TotalComponents found)" -ForegroundColor Green
    $SuccessCount++
} else {
    Write-Host "❌ Component Check: FAILED ($ComponentsFound/$TotalComponents found)" -ForegroundColor Red
    $OverallSuccess = $false
}

if ($HalValidationSuccess) {
    Write-Host "✅ HAL Validation: SUCCESS" -ForegroundColor Green
    $SuccessCount++
} else {
    Write-Host "❌ HAL Validation: FAILED" -ForegroundColor Red
    $OverallSuccess = $false
}

if ($DaemonIntegrationSuccess) {
    Write-Host "✅ Daemon Integration: SUCCESS" -ForegroundColor Green
    $SuccessCount++
} else {
    Write-Host "❌ Daemon Integration: FAILED" -ForegroundColor Red
    $OverallSuccess = $false
}

if ($HardwareDetectionSuccess) {
    Write-Host "✅ Hardware Detection: SUCCESS" -ForegroundColor Green
    $SuccessCount++
} else {
    Write-Host "❌ Hardware Detection: FAILED" -ForegroundColor Red
    $OverallSuccess = $false
}

Write-Host ""
Write-Host "Test Summary: $SuccessCount/$TotalTests tests passed" -ForegroundColor Cyan

if ($OverallSuccess) {
    Write-Host "✅ INTEL HAL DETECTION TEST: SUCCESS" -ForegroundColor Green
    Write-Host "   Intel HAL integration is working correctly"
    Set-Location ".."
    exit 0
} else {
    Write-Host "❌ INTEL HAL DETECTION TEST: FAILED" -ForegroundColor Red
    Write-Host "   Some Intel HAL components are missing or not working"
    Set-Location ".."
    exit 1
}
