# OpenAvnu DAW02 Testing Suite - Fixed Version
# Author: OpenAvnu Modernization Team
# Purpose: Comprehensive testing of modernized OpenAvnu daemons on DAW02

param(
    [string]$TestResultsDir = "test_results_$(Get-Date -Format 'yyyyMMdd_HHmmss')",
    [switch]$SkipBuild = $false,
    [switch]$Verbose = $false
)

# Create test results directory
New-Item -ItemType Directory -Path $TestResultsDir -Force | Out-Null
$MasterLogFile = Join-Path $TestResultsDir "master_test.log"
$MasterReportFile = Join-Path $TestResultsDir "OpenAvnu_DAW02_Master_Report.md"

Write-Host ""
Write-Host "=================================================" -ForegroundColor Cyan
Write-Host "  OpenAvnu DAW02 Testing Suite - Fixed Version"
Write-Host "=================================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Test Results Directory: $TestResultsDir" -ForegroundColor Yellow
Write-Host ""

# Initialize master report
@"
# OpenAvnu DAW02 Testing Report
Generated: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')
System: $env:COMPUTERNAME
User: $env:USERNAME

## Executive Summary

"@ | Out-File -FilePath $MasterReportFile -Encoding UTF8

# Function to run individual tests
function Run-Test {
    param(
        [string]$TestName,
        [string]$TestScript,
        [string]$Description
    )
    
    Write-Host "Running: $TestName" -ForegroundColor White
    Write-Host "Description: $Description" -ForegroundColor Gray
    
    $LogFile = Join-Path $TestResultsDir "$TestName.log"
    $StartTime = Get-Date
    
    try {
        if ($TestScript.EndsWith(".ps1")) {
            & powershell.exe -ExecutionPolicy Bypass -File $TestScript *>&1 | Out-File -FilePath $LogFile -Append -Encoding UTF8
        } else {
            & bash $TestScript *>&1 | Out-File -FilePath $LogFile -Append -Encoding UTF8
        }
        
        if ($LASTEXITCODE -eq 0) {
            Write-Host "   PASSED" -ForegroundColor Green
            $Status = "PASSED"
        } else {
            Write-Host "   FAILED (Exit Code: $LASTEXITCODE)" -ForegroundColor Red
            $Status = "FAILED"
        }
    } catch {
        Write-Host "   ERROR: $($_.Exception.Message)" -ForegroundColor Red
        $Status = "ERROR"
    }
    
    $EndTime = Get-Date
    $Duration = $EndTime - $StartTime
    
    Write-Host "   Duration: $($Duration.TotalSeconds) seconds" -ForegroundColor Gray
    Write-Host ""
    
    # Add to master report
    @"
### $TestName
- **Status:** $Status
- **Duration:** $($Duration.TotalSeconds) seconds
- **Description:** $Description
- **Log File:** $TestName.log

"@ | Out-File -FilePath $MasterReportFile -Append -Encoding UTF8

    return $Status -eq "PASSED"
}

# Collect System Information
Write-Host "Collecting System Information..." -ForegroundColor Yellow
$SystemInfoFile = Join-Path $TestResultsDir "system_info.txt"

@"
OpenAvnu DAW02 System Information
Generated: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')

=== SYSTEM OVERVIEW ===
Computer Name: $env:COMPUTERNAME
User: $env:USERNAME
OS: $(Get-WmiObject -Class Win32_OperatingSystem | Select-Object -ExpandProperty Caption)
Architecture: $env:PROCESSOR_ARCHITECTURE
Processors: $(Get-WmiObject -Class Win32_Processor | Select-Object -ExpandProperty Name)
Total Memory: $([math]::Round((Get-WmiObject -Class Win32_ComputerSystem).TotalPhysicalMemory / 1GB, 2)) GB

=== DEVELOPMENT TOOLS ===
"@ | Out-File -FilePath $SystemInfoFile -Encoding UTF8

# Check for development tools
try { 
    $cmakeVersion = & cmake --version 2>$null | Select-Object -First 1
    "CMake: $cmakeVersion" | Out-File -FilePath $SystemInfoFile -Append -Encoding UTF8
} catch { 
    "CMake: Not found" | Out-File -FilePath $SystemInfoFile -Append -Encoding UTF8
}

try { 
    $vsInfo = Get-WmiObject -Class Win32_Product | Where-Object { $_.Name -like "*Visual Studio*" } | Select-Object -ExpandProperty DisplayName -First 1
    "Visual Studio: $vsInfo" | Out-File -FilePath $SystemInfoFile -Append -Encoding UTF8
} catch { 
    "Visual Studio: Not detected" | Out-File -FilePath $SystemInfoFile -Append -Encoding UTF8
}

try { 
    $gitVersion = & git --version 2>$null
    "Git: $gitVersion" | Out-File -FilePath $SystemInfoFile -Append -Encoding UTF8
} catch { 
    "Git: Not found" | Out-File -FilePath $SystemInfoFile -Append -Encoding UTF8
}

Write-Host "System information collected." -ForegroundColor Green
Write-Host ""

# Define test scripts to run
$TestScripts = @(
    @{
        Name = "Build_Verification"
        Script = "test_01_build_verification.ps1"
        Description = "Verify all OpenAvnu daemons build successfully"
    },
    @{
        Name = "Intel_HAL_Detection"
        Script = "test_02_intel_hal_detection.ps1"
        Description = "Test Intel HAL integration and hardware detection"
    }
)

# Run all tests
$TestResults = @()
foreach ($Test in $TestScripts) {
    if (Test-Path $Test.Script) {
        $Result = Run-Test -TestName $Test.Name -TestScript $Test.Script -Description $Test.Description
        $TestResults += @{ Name = $Test.Name; Passed = $Result }
    } else {
        Write-Host "WARNING: Test script $($Test.Script) not found, skipping..." -ForegroundColor Yellow
        $TestResults += @{ Name = $Test.Name; Passed = $false }
    }
}

# Generate final summary
Write-Host "=================================================" -ForegroundColor Cyan
Write-Host "  TEST SUMMARY"
Write-Host "=================================================" -ForegroundColor Cyan

$PassedTests = ($TestResults | Where-Object { $_.Passed }).Count
$TotalTests = $TestResults.Count

Write-Host ""
Write-Host "Total Tests: $TotalTests" -ForegroundColor White
Write-Host "Passed: $PassedTests" -ForegroundColor Green
Write-Host "Failed: $($TotalTests - $PassedTests)" -ForegroundColor Red
Write-Host ""

# Update master report with summary
@"

## Final Summary

- **Total Tests:** $TotalTests
- **Passed:** $PassedTests
- **Failed:** $($TotalTests - $PassedTests)

### Test Results Details

"@ | Out-File -FilePath $MasterReportFile -Append -Encoding UTF8

foreach ($Result in $TestResults) {
    $StatusIcon = if ($Result.Passed) { "PASS" } else { "FAIL" }
    "- **$($Result.Name):** $StatusIcon" | Out-File -FilePath $MasterReportFile -Append -Encoding UTF8
}

@"

### Recommendations

"@ | Out-File -FilePath $MasterReportFile -Append -Encoding UTF8

if ($PassedTests -eq $TotalTests) {
    Write-Host "ALL TESTS PASSED! OpenAvnu is ready for production on DAW02." -ForegroundColor Green
    "All tests passed successfully. OpenAvnu daemons are ready for production deployment on DAW02." | Out-File -FilePath $MasterReportFile -Append -Encoding UTF8
} elseif ($PassedTests -gt 0) {
    Write-Host "PARTIAL SUCCESS: Some tests passed. Review failed tests for issues." -ForegroundColor Yellow
    "Partial success achieved. Review failed test logs for specific issues and requirements." | Out-File -FilePath $MasterReportFile -Append -Encoding UTF8
} else {
    Write-Host "ALL TESTS FAILED: Review system requirements and dependencies." -ForegroundColor Red
    "All tests failed. Review system requirements, dependencies, and error logs for resolution." | Out-File -FilePath $MasterReportFile -Append -Encoding UTF8
}

Write-Host ""
Write-Host "Results saved to: $TestResultsDir" -ForegroundColor Cyan
Write-Host "Master report: $MasterReportFile" -ForegroundColor Cyan
Write-Host ""
Write-Host "Testing completed!" -ForegroundColor Green
