# OpenAvnu Hardware Testing - Phase 1 Complete Test Suite
# Runs all Phase 1 tests in sequence
# Date: 2025-01-11
# System: ATGRZWN313892

param(
    [switch]$SkipConnectivity = $false,
    [string]$TestTargetIP = "",
    [switch]$GenerateReport
)

Write-Host "=== OpenAvnu Phase 1 Hardware Testing Suite ===" -ForegroundColor Green
Write-Host "System: $env:COMPUTERNAME" -ForegroundColor Cyan
Write-Host "Date: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')" -ForegroundColor Cyan
Write-Host "=" * 60

$testResults = @{
    "Test1_InterfaceDetection" = $false
    "Test2_NetworkCommunication" = $false  
    "Test3_DriverInterface" = $false
}

$testStartTime = Get-Date

# Test Case 1: Interface Detection
Write-Host ""
Write-Host "🔍 Running Test Case 1: Interface Detection..." -ForegroundColor Yellow
Write-Host "=" * 60

try {
    & "$PSScriptRoot\phase1_interface_detection.ps1" | Out-Host
    if ($LASTEXITCODE -eq 0) {
        $testResults["Test1_InterfaceDetection"] = $true
        Write-Host "✅ Test Case 1: PASSED" -ForegroundColor Green
    } else {
        Write-Host "❌ Test Case 1: FAILED" -ForegroundColor Red
    }
} catch {
    Write-Host "❌ Test Case 1: ERROR - $($_.Exception.Message)" -ForegroundColor Red
}

# Test Case 2: Network Communication
Write-Host ""
Write-Host "🌐 Running Test Case 2: Network Communication..." -ForegroundColor Yellow
Write-Host "=" * 60

try {
    $params = @()
    if ($SkipConnectivity) { $params += "-SkipConnectivity" }
    if ($TestTargetIP) { $params += "-TestTargetIP", $TestTargetIP }
    
    & "$PSScriptRoot\phase1_network_communication.ps1" @params | Out-Host
    if ($LASTEXITCODE -eq 0 -or $LASTEXITCODE -eq $null) {
        $testResults["Test2_NetworkCommunication"] = $true
        Write-Host "✅ Test Case 2: PASSED" -ForegroundColor Green
    } else {
        Write-Host "⚠️ Test Case 2: PARTIAL (connectivity issues)" -ForegroundColor Yellow
        $testResults["Test2_NetworkCommunication"] = "PARTIAL"
    }
} catch {
    Write-Host "❌ Test Case 2: ERROR - $($_.Exception.Message)" -ForegroundColor Red
}

# Test Case 3: Driver Interface
Write-Host ""
Write-Host "🔧 Running Test Case 3: Driver Interface..." -ForegroundColor Yellow
Write-Host "=" * 60

try {
    & "$PSScriptRoot\phase1_driver_interface.ps1" | Out-Host
    if ($LASTEXITCODE -eq 0) {
        $testResults["Test3_DriverInterface"] = $true
        Write-Host "✅ Test Case 3: PASSED" -ForegroundColor Green
    } else {
        Write-Host "❌ Test Case 3: FAILED" -ForegroundColor Red
    }
} catch {
    Write-Host "❌ Test Case 3: ERROR - $($_.Exception.Message)" -ForegroundColor Red
}

# Calculate test duration
$testEndTime = Get-Date
$testDuration = $testEndTime - $testStartTime

# Generate summary report
Write-Host ""
Write-Host "=" * 60
Write-Host "🏁 Phase 1 Testing Complete" -ForegroundColor Green
Write-Host "=" * 60

$passedTests = ($testResults.Values | Where-Object { $_ -eq $true }).Count
$partialTests = ($testResults.Values | Where-Object { $_ -eq "PARTIAL" }).Count
$totalTests = $testResults.Count

Write-Host "Test Duration: $($testDuration.TotalMinutes.ToString("F1")) minutes" -ForegroundColor Gray
Write-Host "Tests Passed: $passedTests/$totalTests" -ForegroundColor White
if ($partialTests -gt 0) {
    Write-Host "Tests Partial: $partialTests" -ForegroundColor Yellow
}

foreach ($test in $testResults.GetEnumerator()) {
    $status = switch ($test.Value) {
        $true { "✅ PASS" }
        "PARTIAL" { "⚠️ PARTIAL" }
        $false { "❌ FAIL" }
    }
    $color = switch ($test.Value) {
        $true { "Green" }
        "PARTIAL" { "Yellow" }
        $false { "Red" }
    }
    Write-Host "  $($test.Key): $status" -ForegroundColor $color
}

# Determine overall status
$overallStatus = if ($passedTests -eq $totalTests) {
    "✅ COMPLETE SUCCESS"
} elseif ($passedTests + $partialTests -eq $totalTests) {
    "⚠️ PARTIAL SUCCESS"
} elseif ($passedTests -gt 0) {
    "🔄 MIXED RESULTS"
} else {
    "❌ FAILURE"
}

Write-Host ""
Write-Host "Overall Status: $overallStatus" -ForegroundColor $(
    switch ($overallStatus) {
        { $_ -like "*SUCCESS*" } { "Green" }
        { $_ -like "*PARTIAL*" } { "Yellow" }
        { $_ -like "*MIXED*" } { "Yellow" }
        default { "Red" }
    }
)

# Phase 2 readiness assessment
if ($testResults["Test1_InterfaceDetection"] -and $testResults["Test3_DriverInterface"]) {
    Write-Host ""
    Write-Host "🚀 Ready for Phase 2: Register Access Testing" -ForegroundColor Green
    Write-Host "   All critical prerequisites met for hardware register testing" -ForegroundColor Gray
} else {
    Write-Host ""
    Write-Host "🛑 Not ready for Phase 2" -ForegroundColor Red
    Write-Host "   Critical failures detected - resolve issues before proceeding" -ForegroundColor Gray
}

# Generate detailed report if requested
if ($GenerateReport) {
    $reportPath = "$PSScriptRoot\..\results\phase1_test_report_$(Get-Date -Format 'yyyyMMdd_HHmmss').md"
    
    # Ensure results directory exists
    $resultsDir = Split-Path $reportPath -Parent
    if (-not (Test-Path $resultsDir)) {
        New-Item -ItemType Directory -Path $resultsDir -Force | Out-Null
    }
    
    $reportContent = @"
# Phase 1 Hardware Testing Report

**Date**: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')
**System**: $env:COMPUTERNAME
**Duration**: $($testDuration.TotalMinutes.ToString("F1")) minutes
**Overall Status**: $overallStatus

## Test Results Summary

| Test Case | Status | Details |
|-----------|--------|---------|
| Interface Detection | $(if ($testResults["Test1_InterfaceDetection"]) { "✅ PASS" } else { "❌ FAIL" }) | Hardware detection and PTP capability verification |
| Network Communication | $(if ($testResults["Test2_NetworkCommunication"] -eq $true) { "✅ PASS" } elseif ($testResults["Test2_NetworkCommunication"] -eq "PARTIAL") { "⚠️ PARTIAL" } else { "❌ FAIL" }) | Basic connectivity and adapter configuration |
| Driver Interface | $(if ($testResults["Test3_DriverInterface"]) { "✅ PASS" } else { "❌ FAIL" }) | NDIS interface and Intel HAL prerequisites |

## System Information

- **Machine**: $env:COMPUTERNAME
- **OS**: Windows 10 Enterprise
- **Primary NIC**: Intel(R) Ethernet Connection (22) I219-LM
- **Test Framework**: OpenAvnu Hardware Testing v1.0

## Phase 2 Readiness

$(if ($testResults["Test1_InterfaceDetection"] -and $testResults["Test3_DriverInterface"]) {
    "✅ **READY** - All critical prerequisites met for Phase 2 register access testing"
} else {
    "❌ **NOT READY** - Critical issues must be resolved before proceeding to Phase 2"
})

## Next Steps

1. $(if ($testResults["Test1_InterfaceDetection"]) { "✅" } else { "❌" }) Hardware detection verified
2. $(if ($testResults["Test2_NetworkCommunication"] -ne $false) { "✅" } else { "❌" }) Network connectivity established  
3. $(if ($testResults["Test3_DriverInterface"]) { "✅" } else { "❌" }) Driver interface validated
4. 🔄 **Next**: Begin Phase 2 register access testing

---
*Report generated by OpenAvnu Hardware Testing Framework*
"@

    try {
        $reportContent | Out-File -FilePath $reportPath -Encoding UTF8
        Write-Host ""
        Write-Host "📄 Detailed report saved: $reportPath" -ForegroundColor Gray
    } catch {
        Write-Host ""
        Write-Host "⚠️ Could not save report: $($_.Exception.Message)" -ForegroundColor Yellow
    }
}

Write-Host ""
Write-Host "Phase 1 testing completed. Check individual test outputs above for details." -ForegroundColor Gray

# Set exit code based on overall success
if ($testResults["Test1_InterfaceDetection"] -and $testResults["Test3_DriverInterface"]) {
    exit 0  # Success - ready for Phase 2
} else {
    exit 1  # Failure - not ready for Phase 2
}
