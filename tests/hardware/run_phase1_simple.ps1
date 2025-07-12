# OpenAvnu Hardware Testing - Phase 1 Simple Test Runner
# Date: 2025-01-11
# System: DEV01

param(
    [switch]$SkipConnectivity
)

Write-Host "=== OpenAvnu Phase 1 Hardware Testing ===" -ForegroundColor Green
Write-Host "System: $env:COMPUTERNAME" -ForegroundColor Cyan
Write-Host "Date: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')" -ForegroundColor Cyan
Write-Host ""

$testResults = @()
$startTime = Get-Date

# Test 1: Interface Detection
Write-Host "Running Test 1: Interface Detection..." -ForegroundColor Yellow
$test1Start = Get-Date
try {
    powershell -ExecutionPolicy Bypass -File "phase1_interface_detection.ps1"
    if ($LASTEXITCODE -eq 0) {
        $testResults += @{ Name = "Interface Detection"; Status = "PASS"; Duration = ((Get-Date) - $test1Start).TotalSeconds }
        Write-Host "✅ Test 1: PASSED" -ForegroundColor Green
    } else {
        $testResults += @{ Name = "Interface Detection"; Status = "FAIL"; Duration = ((Get-Date) - $test1Start).TotalSeconds }
        Write-Host "❌ Test 1: FAILED" -ForegroundColor Red
    }
} catch {
    $testResults += @{ Name = "Interface Detection"; Status = "ERROR"; Duration = ((Get-Date) - $test1Start).TotalSeconds }
    Write-Host "❌ Test 1: ERROR" -ForegroundColor Red
}

Write-Host ""

# Test 2: Network Communication  
Write-Host "Running Test 2: Network Communication..." -ForegroundColor Yellow
$test2Start = Get-Date
try {
    if ($SkipConnectivity) {
        powershell -ExecutionPolicy Bypass -File "phase1_network_communication.ps1" -SkipConnectivity
    } else {
        powershell -ExecutionPolicy Bypass -File "phase1_network_communication.ps1"
    }
    
    if ($LASTEXITCODE -eq 0) {
        $testResults += @{ Name = "Network Communication"; Status = "PASS"; Duration = ((Get-Date) - $test2Start).TotalSeconds }
        Write-Host "✅ Test 2: PASSED" -ForegroundColor Green
    } else {
        $testResults += @{ Name = "Network Communication"; Status = "PARTIAL"; Duration = ((Get-Date) - $test2Start).TotalSeconds }
        Write-Host "⚠️ Test 2: PARTIAL" -ForegroundColor Yellow
    }
} catch {
    $testResults += @{ Name = "Network Communication"; Status = "ERROR"; Duration = ((Get-Date) - $test2Start).TotalSeconds }
    Write-Host "❌ Test 2: ERROR" -ForegroundColor Red
}

Write-Host ""

# Test 3: Driver Interface
Write-Host "Running Test 3: Driver Interface..." -ForegroundColor Yellow
$test3Start = Get-Date
try {
    powershell -ExecutionPolicy Bypass -File "phase1_driver_interface.ps1"
    if ($LASTEXITCODE -eq 0) {
        $testResults += @{ Name = "Driver Interface"; Status = "PASS"; Duration = ((Get-Date) - $test3Start).TotalSeconds }
        Write-Host "✅ Test 3: PASSED" -ForegroundColor Green
    } else {
        $testResults += @{ Name = "Driver Interface"; Status = "FAIL"; Duration = ((Get-Date) - $test3Start).TotalSeconds }
        Write-Host "❌ Test 3: FAILED" -ForegroundColor Red
    }
} catch {
    $testResults += @{ Name = "Driver Interface"; Status = "ERROR"; Duration = ((Get-Date) - $test3Start).TotalSeconds }
    Write-Host "❌ Test 3: ERROR" -ForegroundColor Red
}

# Summary
$totalTime = ((Get-Date) - $startTime).TotalMinutes
$passedTests = ($testResults | Where-Object { $_.Status -eq "PASS" }).Count
$partialTests = ($testResults | Where-Object { $_.Status -eq "PARTIAL" }).Count

Write-Host ""
Write-Host "=== Phase 1 Test Summary ===" -ForegroundColor Green
Write-Host "Total Duration: $($totalTime.ToString('F1')) minutes" -ForegroundColor Gray
Write-Host "Tests Passed: $passedTests/3" -ForegroundColor White

foreach ($test in $testResults) {
    $color = switch ($test.Status) {
        "PASS" { "Green" }
        "PARTIAL" { "Yellow" }
        default { "Red" }
    }
    $icon = switch ($test.Status) {
        "PASS" { "✅" }
        "PARTIAL" { "⚠️" }
        default { "❌" }
    }
    Write-Host "  $($test.Name): $icon $($test.Status)" -ForegroundColor $color
}

# Overall assessment
if ($passedTests -eq 3) {
    Write-Host ""
    Write-Host "🚀 Phase 1 Complete: Ready for Phase 2" -ForegroundColor Green
    exit 0
} elseif ($passedTests -ge 2) {
    Write-Host ""
    Write-Host "⚠️ Phase 1 Mostly Complete: Review issues before Phase 2" -ForegroundColor Yellow
    exit 0
} else {
    Write-Host ""
    Write-Host "❌ Phase 1 Failed: Address critical issues" -ForegroundColor Red
    exit 1
}
