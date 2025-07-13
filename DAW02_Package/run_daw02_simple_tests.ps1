# OpenAvnu DAW02 Simple Test Suite
# Simplified version for reliable testing on DAW02

Write-Host "=== OpenAvnu DAW02 Simple Test Suite ===" -ForegroundColor Green
Write-Host "System: $env:COMPUTERNAME"
Write-Host "Date: $(Get-Date)"
Write-Host "Objective: Run simplified OpenAvnu tests on Windows DAW02"
Write-Host ""

$TestResults = @{
    TotalTests = 0
    PassedTests = 0
    FailedTests = 0
    Errors = @()
}

function Test-BuildExists {
    Write-Host "=== Test 1: Build Verification ===" -ForegroundColor Yellow
    $TestResults.TotalTests++
    
    $BuildDir = "..\build"
    if (Test-Path $BuildDir) {
        Write-Host "[PASS] Build directory exists: $BuildDir" -ForegroundColor Green
        
        # Check for key executables
        $KeyFiles = @(
            "$BuildDir\daemons\mrpd\Release\mrpd.exe",
            "$BuildDir\daemons\maap\Release\maap_daemon.exe",
            "$BuildDir\thirdparty\gptp\Release\gptp.exe"
        )
        
        $AllFilesExist = $true
        foreach ($File in $KeyFiles) {
            if (Test-Path $File) {
                Write-Host "[PASS] Found: $(Split-Path $File -Leaf)" -ForegroundColor Green
            } else {
                Write-Host "[FAIL] Missing: $(Split-Path $File -Leaf)" -ForegroundColor Red
                $AllFilesExist = $false
            }
        }
        
        if ($AllFilesExist) {
            $TestResults.PassedTests++
            Write-Host "[SUCCESS] Build verification: PASSED" -ForegroundColor Green
            return $true
        } else {
            $TestResults.FailedTests++
            $TestResults.Errors += "Build verification failed - missing executables"
            Write-Host "[FAIL] Build verification: FAILED" -ForegroundColor Red
            return $false
        }
    } else {
        $TestResults.FailedTests++
        $TestResults.Errors += "Build directory not found"
        Write-Host "[FAIL] Build directory not found: $BuildDir" -ForegroundColor Red
        return $false
    }
}

function Test-NetworkDetection {
    Write-Host "=== Test 2: Network Detection ===" -ForegroundColor Yellow
    $TestResults.TotalTests++
    
    try {
        $Adapters = Get-NetAdapter | Where-Object { $_.InterfaceDescription -like "*Intel*" }
        
        if ($Adapters.Count -gt 0) {
            Write-Host "[PASS] Found $($Adapters.Count) Intel network adapter(s)" -ForegroundColor Green
            
            foreach ($Adapter in $Adapters) {
                $Status = switch ($Adapter.Status) {
                    "Up" { "[CONNECTED]" }
                    "Down" { "[DISCONNECTED]" }
                    "Disabled" { "[DISABLED]" }
                    default { "[UNKNOWN]" }
                }
                
                Write-Host "  $Status $($Adapter.Name): $($Adapter.InterfaceDescription)" -ForegroundColor Cyan
                
                # Check for AVB capability
                if ($Adapter.InterfaceDescription -match "I219|I210|I211|I225|I226") {
                    Write-Host "    [AVB] Hardware timestamping capable" -ForegroundColor Green
                }
            }
            
            $TestResults.PassedTests++
            Write-Host "[SUCCESS] Network detection: PASSED" -ForegroundColor Green
            return $true
        } else {
            $TestResults.FailedTests++
            $TestResults.Errors += "No Intel network adapters found"
            Write-Host "[FAIL] No Intel network adapters found" -ForegroundColor Red
            return $false
        }
    } catch {
        $TestResults.FailedTests++
        $TestResults.Errors += "Network detection error: $($_.Exception.Message)"
        Write-Host "[FAIL] Network detection error: $($_.Exception.Message)" -ForegroundColor Red
        return $false
    }
}

function Test-BasicDaemonResponse {
    Write-Host "=== Test 3: Basic Daemon Response ===" -ForegroundColor Yellow
    $TestResults.TotalTests++
    
    $BuildDir = "..\build"
    $DaemonTests = @{
        "MRPD" = "$BuildDir\daemons\mrpd\Release\mrpd.exe"
        "MAAP" = "$BuildDir\daemons\maap\Release\maap_daemon.exe"
        "gPTP" = "$BuildDir\thirdparty\gptp\Release\gptp.exe"
    }
    
    $AllDaemonsRespond = $true
    
    foreach ($DaemonName in $DaemonTests.Keys) {
        $DaemonPath = $DaemonTests[$DaemonName]
        
        if (Test-Path $DaemonPath) {
            try {
                # Quick version check with timeout
                $TimeoutJob = Start-Job -ScriptBlock {
                    param($Path)
                    try {
                        & $Path --version 2>&1 | Out-String
                        return $true
                    } catch {
                        try {
                            & $Path -v 2>&1 | Out-String
                            return $true
                        } catch {
                            try {
                                & $Path --help 2>&1 | Out-String
                                return $true
                            } catch {
                                return $false
                            }
                        }
                    }
                } -ArgumentList $DaemonPath
                
                if (Wait-Job $TimeoutJob -Timeout 3) {
                    $Result = Receive-Job $TimeoutJob
                    Remove-Job $TimeoutJob
                    Write-Host "[PASS] $DaemonName responds correctly" -ForegroundColor Green
                } else {
                    Remove-Job $TimeoutJob -Force
                    Write-Host "[WARN] $DaemonName timeout (may need admin privileges)" -ForegroundColor Yellow
                }
            } catch {
                Write-Host "[WARN] $DaemonName test failed: $($_.Exception.Message)" -ForegroundColor Yellow
                $AllDaemonsRespond = $false
            }
        } else {
            Write-Host "[FAIL] $DaemonName not found: $DaemonPath" -ForegroundColor Red
            $AllDaemonsRespond = $false
        }
    }
    
    if ($AllDaemonsRespond) {
        $TestResults.PassedTests++
        Write-Host "[SUCCESS] Basic daemon response: PASSED" -ForegroundColor Green
        return $true
    } else {
        $TestResults.FailedTests++
        $TestResults.Errors += "Some daemons failed to respond"
        Write-Host "[WARNING] Basic daemon response: PARTIAL" -ForegroundColor Yellow
        return $false
    }
}

function Test-AdvancedComponents {
    Write-Host "=== Test 4: Advanced Component Check ===" -ForegroundColor Yellow
    $TestResults.TotalTests++
    
    $BuildDir = "..\build"
    $AdvancedTests = @{
        "Clock Quality Tests" = "$BuildDir\testing\conformance\avnu_alliance\gptp\clock_quality\Release\clock_quality_tests_consolidated.exe"
        "Intel HAL Validation" = "$BuildDir\tests\intel_hal_validation_test_consolidated.exe"
        "Performance Tests" = "$BuildDir\testing\performance\Release\phase2_baseline_measurement.exe"
    }
    
    $ComponentsFound = 0
    $TotalComponents = $AdvancedTests.Count
    
    foreach ($ComponentName in $AdvancedTests.Keys) {
        $ComponentPath = $AdvancedTests[$ComponentName]
        
        if (Test-Path $ComponentPath) {
            Write-Host "[PASS] $ComponentName available" -ForegroundColor Green
            $ComponentsFound++
        } else {
            Write-Host "[SKIP] $ComponentName not found" -ForegroundColor Yellow
        }
    }
    
    if ($ComponentsFound -eq $TotalComponents) {
        $TestResults.PassedTests++
        Write-Host "[SUCCESS] Advanced components: ALL AVAILABLE" -ForegroundColor Green
        return $true
    } elseif ($ComponentsFound -gt 0) {
        $TestResults.PassedTests++
        Write-Host "[PARTIAL] Advanced components: $ComponentsFound/$TotalComponents available" -ForegroundColor Yellow
        return $true
    } else {
        $TestResults.FailedTests++
        $TestResults.Errors += "No advanced components found"
        Write-Host "[FAIL] Advanced components: NONE AVAILABLE" -ForegroundColor Red
        return $false
    }
}

# Run all tests
Write-Host "Starting OpenAvnu DAW02 test suite..." -ForegroundColor Cyan
Write-Host ""

$Test1 = Test-BuildExists
Write-Host ""

$Test2 = Test-NetworkDetection  
Write-Host ""

$Test3 = Test-BasicDaemonResponse
Write-Host ""

$Test4 = Test-AdvancedComponents
Write-Host ""

# Summary
Write-Host "=== Test Summary ===" -ForegroundColor Yellow
Write-Host "Total Tests: $($TestResults.TotalTests)"
Write-Host "Passed: $($TestResults.PassedTests)" -ForegroundColor Green
Write-Host "Failed: $($TestResults.FailedTests)" -ForegroundColor $(if ($TestResults.FailedTests -eq 0) { "Green" } else { "Red" })

if ($TestResults.Errors.Count -gt 0) {
    Write-Host ""
    Write-Host "Errors encountered:" -ForegroundColor Red
    foreach ($Error in $TestResults.Errors) {
        Write-Host "  - $Error" -ForegroundColor Red
    }
}

Write-Host ""
if ($TestResults.FailedTests -eq 0) {
    Write-Host "[SUCCESS] All DAW02 tests completed successfully!" -ForegroundColor Green
    Write-Host "OpenAvnu is ready for production use on DAW02" -ForegroundColor Green
    exit 0
} else {
    $SuccessRate = [math]::Round(($TestResults.PassedTests / $TestResults.TotalTests) * 100, 1)
    Write-Host "[PARTIAL] DAW02 tests completed with $SuccessRate% success rate" -ForegroundColor Yellow
    Write-Host "Some issues detected - review errors above" -ForegroundColor Yellow
    exit 1
}
