# OpenAvnu DAW02 Master Test Runner for Windows
# PowerShell version for comprehensive testing suite

param(
    [string]$OutputPath = "test_results_$(Get-Date -Format 'yyyyMMdd_HHmmss')"
)

# Configuration
$TestSession = "OpenAvnu_DAW02_$(Get-Date -Format 'yyyyMMdd_HHmmss')"
$ReportDir = $OutputPath
$MasterReport = "$ReportDir\OpenAvnu_DAW02_Master_Report.md"

# Create results directory
New-Item -ItemType Directory -Path $ReportDir -Force | Out-Null

Write-Host "[TEST] OpenAvnu DAW02 Testing Suite Starting..." -ForegroundColor Green
Write-Host "Test Session: $TestSession"
Write-Host "Results Directory: $ReportDir"
Write-Host ""

# Initialize master report
@"
# OpenAvnu DAW02 Comprehensive Testing Report

**Test Session:** $TestSession  
**System:** $env:COMPUTERNAME  
**Date:** $(Get-Date)  
**OS:** $(Get-WmiObject -Class Win32_OperatingSystem | Select-Object -ExpandProperty Caption)  
**User:** $env:USERNAME  

## Executive Summary

This report contains comprehensive testing results for the OpenAvnu daemon modernization on DAW02.

"@ | Out-File -FilePath $MasterReport -Encoding UTF8

# Function to run test and capture results
function Run-Test {
    param(
        [string]$TestName,
        [string]$TestScript,
        [string]$TestDescription
    )
    
    Write-Host "[RUN] Running: $TestName" -ForegroundColor Blue
    Write-Host "   $TestDescription"
    
    $LogFile = "$ReportDir\${TestName}_$(Get-Date -Format 'HHmmss').log"
    
    if (Test-Path $TestScript) {
        @"
=== $TestName ===
Description: $TestDescription
Started: $(Get-Date)

"@ | Out-File -FilePath $LogFile -Encoding UTF8
        
        try {
            if ($TestScript.EndsWith(".ps1")) {
                & powershell.exe -ExecutionPolicy Bypass -File $TestScript *>&1 | Out-File -FilePath $LogFile -Append -Encoding UTF8
            } else {
                & bash $TestScript *>&1 | Out-File -FilePath $LogFile -Append -Encoding UTF8
            }
            
            if ($LASTEXITCODE -eq 0) {
                Write-Host "   [PASS] PASSED" -ForegroundColor Green
                "Completed: $(Get-Date)" | Out-File -FilePath $LogFile -Append -Encoding UTF8
                
                # Add to master report
                @"
### [PASS] $TestName - PASSED

$TestDescription

``````
$(Get-Content $LogFile -Tail 20 | Out-String)
``````

"@ | Out-File -FilePath $MasterReport -Append -Encoding UTF8
            } else {
                Write-Host "   [FAIL] FAILED" -ForegroundColor Red
                "Failed: $(Get-Date)" | Out-File -FilePath $LogFile -Append -Encoding UTF8
                
                # Add to master report
                @"
### [FAIL] $TestName - FAILED

$TestDescription

``````
$(Get-Content $LogFile -Tail 30 | Out-String)
``````

"@ | Out-File -FilePath $MasterReport -Append -Encoding UTF8
            }
        } catch {
            Write-Host "   [FAIL] FAILED (Exception)" -ForegroundColor Red
            "Exception: $($_.Exception.Message)" | Out-File -FilePath $LogFile -Append -Encoding UTF8
        }
    } else {
        Write-Host "   [SKIP] SKIPPED (script not found)" -ForegroundColor Yellow
        @"
### [SKIP] $TestName - SKIPPED

Test script '$TestScript' not found.

"@ | Out-File -FilePath $MasterReport -Append -Encoding UTF8
    }
    
    Write-Host ""
}

# System Information Collection
Write-Host "[INFO] Collecting System Information..." -ForegroundColor Yellow
$SystemInfoFile = "$ReportDir\system_info.txt"

@"
=== DAW02 System Information ===
Hostname: $env:COMPUTERNAME
Date: $(Get-Date)
OS: $(Get-WmiObject -Class Win32_OperatingSystem | Select-Object -ExpandProperty Caption)
Version: $(Get-WmiObject -Class Win32_OperatingSystem | Select-Object -ExpandProperty Version)
Architecture: $(Get-WmiObject -Class Win32_OperatingSystem | Select-Object -ExpandProperty OSArchitecture)

=== Hardware Information ===
"@ | Out-File -FilePath $SystemInfoFile -Encoding UTF8

# CPU Information
"CPU Information:" | Out-File -FilePath $SystemInfoFile -Append -Encoding UTF8
Get-WmiObject -Class Win32_Processor | Select-Object Name, NumberOfCores, NumberOfLogicalProcessors | Out-File -FilePath $SystemInfoFile -Append -Encoding UTF8

# Memory Information
"Memory Information:" | Out-File -FilePath $SystemInfoFile -Append -Encoding UTF8
$Memory = Get-WmiObject -Class Win32_ComputerSystem
"Total Physical Memory: $([math]::Round($Memory.TotalPhysicalMemory/1GB, 2)) GB" | Out-File -FilePath $SystemInfoFile -Append -Encoding UTF8

# Network Adapters
"Network Interfaces:" | Out-File -FilePath $SystemInfoFile -Append -Encoding UTF8
Get-NetAdapter | Select-Object Name, InterfaceDescription, Status | Out-File -FilePath $SystemInfoFile -Append -Encoding UTF8

# Intel Hardware Detection
"Intel Network Hardware:" | Out-File -FilePath $SystemInfoFile -Append -Encoding UTF8
$IntelAdapters = Get-NetAdapter | Where-Object { $_.InterfaceDescription -like "*Intel*" }
if ($IntelAdapters) {
    $IntelAdapters | Select-Object Name, InterfaceDescription | Out-File -FilePath $SystemInfoFile -Append -Encoding UTF8
} else {
    "No Intel network hardware found" | Out-File -FilePath $SystemInfoFile -Append -Encoding UTF8
}

# Development Tools
"Development Tools:" | Out-File -FilePath $SystemInfoFile -Append -Encoding UTF8
try { 
    $cmakeVersion = cmake --version 2>$null | Select-Object -First 1
    "CMake: $cmakeVersion" | Out-File -FilePath $SystemInfoFile -Append -Encoding UTF8
} catch { 
    "CMake: Not found" | Out-File -FilePath $SystemInfoFile -Append -Encoding UTF8
}
try { 
    $vsVersion = Get-ChildItem 'HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\*' | Get-ItemProperty | Where-Object { $_.DisplayName -like '*Visual Studio*' } | Select-Object -First 1 -ExpandProperty DisplayName
    "Visual Studio: $vsVersion" | Out-File -FilePath $SystemInfoFile -Append -Encoding UTF8
} catch { 
    "Visual Studio: Not detected" | Out-File -FilePath $SystemInfoFile -Append -Encoding UTF8
}

# Add system info to master report
@"
## System Information

``````
$(Get-Content $SystemInfoFile | Out-String)
``````

## Test Results

"@ | Out-File -FilePath $MasterReport -Append -Encoding UTF8

# Run test suite
Run-Test "01_BuildVerification" "test_01_build_verification.ps1" "Verify all daemons compile successfully on Windows DAW02"
Run-Test "02_IntelHALDetection" "test_02_intel_hal_detection_quick.ps1" "Test Intel HAL integration and graceful fallback on Windows"
Run-Test "03_RuntimeBasic" "test_03_runtime_basic.ps1" "Verify basic daemon startup and help functionality"
Run-Test "04_NetworkDetection" "test_04_network_detection.ps1" "Test network interface detection capabilities on Windows"

# Generate final summary
$TotalTests = 4  # We know we have 4 defined tests
$PassedTests = (Select-String "\[PASS\] PASSED" $MasterReport -ErrorAction SilentlyContinue).Count
$FailedTests = (Select-String "\[FAIL\] FAILED" $MasterReport -ErrorAction SilentlyContinue).Count  
$SkippedTests = (Select-String "\[SKIP\] SKIPPED" $MasterReport -ErrorAction SilentlyContinue).Count

# Ensure counts are not null
if ($PassedTests -eq $null) { $PassedTests = 0 }
if ($FailedTests -eq $null) { $FailedTests = 0 }
if ($SkippedTests -eq $null) { $SkippedTests = 0 }

@"
## Test Summary

- **Total Tests:** $TotalTests
- **Passed:** $PassedTests [PASS]
- **Failed:** $FailedTests [FAIL]  
- **Skipped:** $SkippedTests [SKIP]

### Recommendations

"@ | Out-File -FilePath $MasterReport -Append -Encoding UTF8

if ($FailedTests -eq 0) {
    @"
[SUCCESS] **All tests passed!** The OpenAvnu daemon modernization is fully compatible with DAW02.

[PASS] **Ready for production deployment**
"@ | Out-File -FilePath $MasterReport -Append -Encoding UTF8
} else {
    @"
[WARNING] **Some tests failed.** Review failed test details above for resolution steps.

[ACTION] **Requires investigation before production deployment**
"@ | Out-File -FilePath $MasterReport -Append -Encoding UTF8
}

@"

---
*Report generated by OpenAvnu automated testing suite*  
*Test session: $TestSession*
"@ | Out-File -FilePath $MasterReport -Append -Encoding UTF8

# Final output
Write-Host "[COMPLETE] Testing Complete!" -ForegroundColor Green
Write-Host ""
Write-Host "[REPORT] Master Report: $MasterReport"
Write-Host "[FILES] All Results: $ReportDir\"
Write-Host ""

if ($FailedTests -eq 0 -and $SkippedTests -eq 0) {
    Write-Host "[SUCCESS] All tests passed! OpenAvnu is ready for production on DAW02." -ForegroundColor Green
} elseif ($FailedTests -gt 0) {
    Write-Host "[WARNING] $FailedTests test(s) failed. Review the master report for details." -ForegroundColor Yellow
} else {
    Write-Host "[INFO] $SkippedTests test(s) skipped. Some tests need implementation." -ForegroundColor Cyan
}

Write-Host ""
Write-Host "Next steps:"
Write-Host "1. Review the master report: $MasterReport"
Write-Host "2. Archive results: Compress-Archive -Path $ReportDir -DestinationPath ${TestSession}_results.zip"
Write-Host "3. Transfer results back to development system for analysis"
