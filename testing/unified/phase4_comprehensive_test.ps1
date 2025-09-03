# Phase 4.2 Intel HAL Comprehensive Testing Framework
# FOLLOWS REPOSITORY RULES: Reuses existing tests, no redundant implementations

param(
    [string]$TestSuite = "all",
    [switch]$ListTests,
    [switch]$DryRun,
    [switch]$Verbose,
    [switch]$SaveResults,
    [switch]$ContinueOnFailure,
    [string]$ResultsPath = "test_results_$(Get-Date -Format 'yyyyMMdd_HHmmss').json"
)

Write-Host "Phase 4.2 Intel HAL Comprehensive Testing Framework" -ForegroundColor Cyan
Write-Host "Following Repository Rules: Reusing existing tests" -ForegroundColor Yellow

# Test Registry - REUSES existing test implementations
$TestRegistry = @{
    "detection" = @{
        "script" = "testing\unified\generic_adapter_testing_framework.ps1"
        "params" = @("-TestPhase", "detection")
        "description" = "Intel adapter detection and enumeration (reuses existing)"
    }
    "register" = @{
        "script" = "build\thirdparty\intel-ethernet-hal\tests\Release\intel_hal_full_test.exe"
        "params" = @()
        "description" = "Intel HAL register access validation (reuses existing)"
    }
    "timestamp" = @{
        "script" = "build\testing\performance\Release\phase2_baseline_measurement.exe"
        "params" = @("-all-supported-adapters")
        "description" = "Hardware timestamping testing (reuses existing)"
    }
    "tsn" = @{
        "script" = "build\tests\Release\test_tsn_fixed.exe"
        "params" = @()
        "description" = "Intel HAL TSN feature validation (reuses existing)"
    }
    "integration" = @{
        "script" = "build\testing\integration\Release\avtp_phase3_production_integration_test.exe"
        "params" = @()
        "description" = "End-to-end integration testing (reuses existing)"
    }
}

function Show-AvailableTests {
    Write-Host ""
    Write-Host "Available Test Suites (Reusing Existing Implementations):" -ForegroundColor Cyan
    foreach ($testName in $TestRegistry.Keys | Sort-Object) {
        $test = $TestRegistry[$testName]
        Write-Host "  * $testName - $($test.description)" -ForegroundColor White
    }
    Write-Host ""
    Write-Host "Special Suites:" -ForegroundColor Cyan
    Write-Host "  * all - Run all tests in sequence" -ForegroundColor White
    Write-Host "  * core - Run detection, register, timestamp" -ForegroundColor White
}

function Test-ExecutableExists {
    param([string]$Path)
    
    $fullPath = Join-Path $PSScriptRoot "..\.." $Path
    return Test-Path $fullPath
}

function Invoke-TestSuite {
    param(
        [string]$TestName,
        [hashtable]$TestConfig,
        [switch]$DryRun
    )
    
    Write-Host ""
    Write-Host "Running Test Suite: $TestName" -ForegroundColor Cyan
    Write-Host "Description: $($TestConfig.description)" -ForegroundColor White
    
    $testScript = $TestConfig.script
    $testParams = $TestConfig.params
    $fullPath = Join-Path (Join-Path $PSScriptRoot "..\..") $testScript
    
    if (-not (Test-Path $fullPath)) {
        Write-Host "Test not found: $fullPath" -ForegroundColor Red
        Write-Host "Build required or check path" -ForegroundColor Yellow
        return @{ Success = $false; Message = "Test executable not found"; TestName = $TestName }
    }
    
    if ($DryRun) {
        Write-Host "DRY RUN: Would execute: $fullPath" -ForegroundColor Gray
        return @{ Success = $true; Message = "Dry run completed"; TestName = $TestName }
    }
    
    Write-Host "Executing: $fullPath" -ForegroundColor Green
    
    $startTime = Get-Date
    try {
        if ($testScript.EndsWith(".ps1")) {
            $paramString = $testParams -join " "
            $process = Start-Process -FilePath "powershell.exe" -ArgumentList "-ExecutionPolicy", "Bypass", "-File", "`"$fullPath`"", $paramString -Wait -PassThru -NoNewWindow
            $exitCode = $process.ExitCode
        } else {
            & $fullPath $testParams
            $exitCode = $LASTEXITCODE
        }
    } catch {
        $exitCode = 1
        Write-Host "Execution failed: $($_.Exception.Message)" -ForegroundColor Red
    }
    $endTime = Get-Date
    $duration = $endTime - $startTime
    
    $success = ($exitCode -eq 0)
    $status = if ($success) { "PASSED" } else { "FAILED (Exit Code: $exitCode)" }
    
    Write-Host "$status - Duration: $($duration.TotalSeconds.ToString('F2'))s" -ForegroundColor $(if ($success) { "Green" } else { "Red" })
    
    return @{
        Success = $success
        ExitCode = $exitCode
        Duration = $duration.TotalSeconds
        TestName = $TestName
        Message = if ($success) { "Test passed" } else { "Test failed with exit code $exitCode" }
    }
}

# Main execution
if ($ListTests) {
    Show-AvailableTests
    exit 0
}

$testsToRun = switch ($TestSuite.ToLower()) {
    "all" { $TestRegistry.Keys }
    "core" { @("detection", "register", "timestamp") }
    default { @($TestSuite) }
}

Write-Host ""
Write-Host "Test Execution Plan:" -ForegroundColor Cyan
foreach ($test in $testsToRun) {
    if ($TestRegistry.ContainsKey($test)) {
        Write-Host "  * $test - $($TestRegistry[$test].description)" -ForegroundColor White
    }
}

if ($DryRun) {
    Write-Host ""
    Write-Host "DRY RUN MODE - No tests will be executed" -ForegroundColor Gray
}

# Execute tests
$results = @()
$totalTests = $testsToRun.Count
$passedTests = 0

foreach ($testName in $testsToRun) {
    if (-not $TestRegistry.ContainsKey($testName)) {
        Write-Host "Unknown test: $testName" -ForegroundColor Yellow
        continue
    }
    
    $testConfig = $TestRegistry[$testName]
    $result = Invoke-TestSuite -TestName $testName -TestConfig $testConfig -DryRun:$DryRun
    
    $results += $result
    if ($result.Success) { $passedTests++ }
    
    if (-not $result.Success -and -not $ContinueOnFailure -and -not $DryRun) {
        Write-Host ""
        Write-Host "Test failed and ContinueOnFailure not specified. Stopping." -ForegroundColor Red
        break
    }
}

# Summary
Write-Host ""
Write-Host "Test Summary:" -ForegroundColor Cyan
Write-Host "  Total Tests: $totalTests" -ForegroundColor White
Write-Host "  Passed: $passedTests" -ForegroundColor Green
Write-Host "  Failed: $($totalTests - $passedTests)" -ForegroundColor Red
Write-Host "  Success Rate: $(($passedTests / $totalTests * 100).ToString('F1'))%" -ForegroundColor White

$overallSuccess = ($passedTests -eq $totalTests)
Write-Host ""
Write-Host "Overall Result: $(if ($overallSuccess) { 'SUCCESS' } else { 'FAILURE' })" -ForegroundColor $(if ($overallSuccess) { "Green" } else { "Red" })

exit $(if ($overallSuccess) { 0 } else { 1 })
