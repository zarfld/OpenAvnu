# Phase 4.2 Intel HAL Comprehensive Testing Framework
# FOLLOWS REPOSITORY RULES: Reuses existing tests, no redundant implementations

param(
    [string]$TestSuite = "all",        # all, detection, register, timestamp, gptp, performance, integration
    [string[]]$AdapterFilter = @(),    # Empty = test all adapters
    [string]$ConfigFile = "intel_hal_test_config.json",
    [switch]$Verbose,
    [switch]$SaveResults,
    [switch]$ContinueOnFailure,
    [string]$ResultsPath = "test_results_$(Get-Date -Format 'yyyyMMdd_HHmmss').json",
    [switch]$ListTests,
    [switch]$DryRun
)

# Repository Rules Compliance
Write-Host "Phase 4.2 Intel HAL Comprehensive Testing Framework" -ForegroundColor Cyan
Write-Host "Following Repository Rules: Reusing existing tests, no redundant implementations" -ForegroundColor Yellow

# REUSE: Import existing unified testing framework
$UnifiedFrameworkPath = Join-Path $PSScriptRoot "unified\generic_adapter_testing_framework.ps1"
$IntelHALTestPath = Join-Path $PSScriptRoot "unified\intel_hal_integration_test.ps1"

if (-not (Test-Path $UnifiedFrameworkPath)) {
    Write-Host "ERROR: Required unified framework not found: $UnifiedFrameworkPath" -ForegroundColor Red
    Write-Host "Repository Rule: Reuse existing tests - unified framework required" -ForegroundColor Yellow
    exit 1
}

# Test Suite Registry - REUSES existing test implementations
$TestRegistry = @{
    # Phase 1: Detection (REUSES existing generic adapter framework)
    "detection" = @{
        "script" = $UnifiedFrameworkPath
        "params" = @("-TestPhase", "detection")
        "description" = "Intel adapter detection and enumeration"
        "dependencies" = @()
    }
    
    # Phase 2: Register Access (REUSES existing Intel HAL tests)  
    "register" = @{
        "script" = "$PSScriptRoot\..\thirdparty\intel-ethernet-hal\tests\intel_hal_full_test.exe"
        "params" = @()
        "description" = "Intel HAL register access validation"
        "dependencies" = @("detection")
    }
    
    # Phase 3: Timestamping (REUSES existing performance tests)
    "timestamp" = @{
        "script" = "$PSScriptRoot\..\testing\performance\phase2_baseline_measurement.exe" 
        "params" = @("-all-supported-adapters")
        "description" = "Hardware timestamping capability testing"
        "dependencies" = @("detection", "register")
    }
    
    # Phase 4: gPTP Integration (REUSES existing gPTP tests)
    "gptp" = @{
        "script" = "$PSScriptRoot\..\thirdparty\gptp\test_profile_validation.exe"
        "params" = @()
        "description" = "gPTP Intel HAL integration testing"
        "dependencies" = @("detection", "register", "timestamp")
    }
    
    # Phase 5: Performance Testing (REUSES existing frameworks)
    "performance" = @{
        "script" = $UnifiedFrameworkPath
        "params" = @("-TestPhase", "performance")
        "description" = "Intel HAL performance validation"
        "dependencies" = @("detection", "register", "timestamp", "gptp")
    }
    
    # Integration Testing (REUSES existing integration tests)
    "integration" = @{
        "script" = "$PSScriptRoot\..\testing\integration\avtp_phase3_production_integration_test.exe"
        "params" = @()
        "description" = "End-to-end Intel HAL integration"
        "dependencies" = @("detection", "register", "timestamp", "gptp")
    }
    
    # TSN Feature Testing (REUSES existing TSN test)
    "tsn" = @{
        "script" = "$PSScriptRoot\..\test_tsn_fixed.exe"
        "params" = @()
        "description" = "Intel HAL TSN feature validation"
        "dependencies" = @("detection", "register")
    }
    
    # Open1722 Integration (REUSES existing Open1722 unit tests)
    "open1722" = @{
        "script" = "$PSScriptRoot\..\thirdparty\open1722\unit\test-avtp.exe"
        "params" = @()
        "description" = "Open1722 AVTP integration testing"  
        "dependencies" = @("detection")
    }
}

function Show-AvailableTests {
    Write-Host "`n📋 Available Test Suites (Reusing Existing Implementations):" -ForegroundColor Cyan
    foreach ($testName in $TestRegistry.Keys | Sort-Object) {
        $test = $TestRegistry[$testName]
        $deps = if ($test.dependencies.Count -gt 0) { " (deps: $($test.dependencies -join ', '))" } else { "" }
        Write-Host "  • $testName - $($test.description)$deps" -ForegroundColor White
    }
    Write-Host "`n🔧 Special Suites:" -ForegroundColor Cyan
    Write-Host "  • all - Run all tests in dependency order" -ForegroundColor White
    Write-Host "  • core - Run detection, register, timestamp" -ForegroundColor White
    Write-Host "  • advanced - Run gptp, performance, integration" -ForegroundColor White
}

function Get-TestExecutionOrder {
    param([string[]]$RequestedTests)
    
    # Build dependency graph and resolve execution order
    $allTests = @()
    $visited = @{}
    
    function Add-TestWithDeps {
        param([string]$TestName)
        
        if ($visited[$TestName]) { return }
        $visited[$TestName] = $true
        
        if (-not $TestRegistry.ContainsKey($TestName)) {
            Write-Host "⚠️  Unknown test: $TestName" -ForegroundColor Yellow
            return
        }
        
        $test = $TestRegistry[$TestName]
        foreach ($dep in $test.dependencies) {
            Add-TestWithDeps -TestName $dep
        }
        
        $allTests += $TestName
    }
    
    foreach ($test in $RequestedTests) {
        Add-TestWithDeps -TestName $test
    }
    
    return $allTests
}

function Invoke-TestSuite {
    param(
        [string]$TestName,
        [hashtable]$TestConfig,
        [switch]$DryRun
    )
    
    Write-Host "`n🧪 Running Test Suite: $TestName" -ForegroundColor Cyan
    Write-Host "📝 Description: $($TestConfig.description)" -ForegroundColor White
    
    $testScript = $TestConfig.script
    $testParams = $TestConfig.params
    
    # Verify test executable/script exists (Repository Rule: no broken code)
    if (-not (Test-Path $testScript)) {
        Write-Host "❌ Test not found: $testScript" -ForegroundColor Red
        Write-Host "💡 Build required components or check path" -ForegroundColor Yellow
        return @{ Success = $false; Message = "Test executable not found"; TestName = $TestName }
    }
    
    if ($DryRun) {
        Write-Host "🔍 DRY RUN: Would execute: $testScript $($testParams -join ' ')" -ForegroundColor Gray
        return @{ Success = $true; Message = "Dry run completed"; TestName = $TestName }
    }
    
    # Execute the test (Repository Rule: reuse existing implementations)
    Write-Host "▶️  Executing: $testScript $($testParams -join ' ')" -ForegroundColor Green
    
    $startTime = Get-Date
    try {
        if ($testScript.EndsWith(".ps1")) {
            $result = & $testScript @testParams
        } else {
            $result = & $testScript $testParams
        }
        $exitCode = $LASTEXITCODE
    } catch {
        $exitCode = 1
        $errorMessage = $_.Exception.Message
        Write-Host "❌ Test execution failed: $errorMessage" -ForegroundColor Red
    }
    $endTime = Get-Date
    $duration = $endTime - $startTime
    
    $success = ($exitCode -eq 0)
    $status = if ($success) { "✅ PASSED" } else { "❌ FAILED (Exit Code: $exitCode)" }
    
    Write-Host "$status - Duration: $($duration.TotalSeconds.ToString('F2'))s" -ForegroundColor $(if ($success) { "Green" } else { "Red" })
    
    return @{
        Success = $success
        ExitCode = $exitCode
        Duration = $duration.TotalSeconds
        TestName = $TestName
        Message = if ($success) { "Test passed" } else { "Test failed with exit code $exitCode" }
    }
}

# Main execution logic
function Main {
    Write-Host "🚀 Phase 4.2 Intel HAL Comprehensive Testing Framework Starting..." -ForegroundColor Magenta
    Write-Host "📋 Repository Rules: Reusing existing test implementations" -ForegroundColor Yellow
    
    if ($ListTests) {
        Show-AvailableTests
        return
    }
    
    # Determine tests to run
    $testsToRun = switch ($TestSuite.ToLower()) {
        "all" { $TestRegistry.Keys }
        "core" { @("detection", "register", "timestamp") }
        "advanced" { @("gptp", "performance", "integration") }
        "tsn" { @("detection", "register", "tsn") }
        default { @($TestSuite) }
    }
    
    # Get execution order respecting dependencies
    $executionOrder = Get-TestExecutionOrder -RequestedTests $testsToRun
    
    Write-Host "`n📋 Test Execution Plan:" -ForegroundColor Cyan
    foreach ($test in $executionOrder) {
        Write-Host "  • $test - $($TestRegistry[$test].description)" -ForegroundColor White
    }
    
    if ($DryRun) {
        Write-Host "`n🔍 DRY RUN MODE - No tests will be executed" -ForegroundColor Gray
    }
    
    # Execute tests
    $results = @()
    $totalTests = $executionOrder.Count
    $passedTests = 0
    
    foreach ($testName in $executionOrder) {
        $testConfig = $TestRegistry[$testName]
        $result = Invoke-TestSuite -TestName $testName -TestConfig $testConfig -DryRun:$DryRun
        
        $results += $result
        if ($result.Success) { $passedTests++ }
        
        if (-not $result.Success -and -not $ContinueOnFailure -and -not $DryRun) {
            Write-Host "`n❌ Test failed and ContinueOnFailure not specified. Stopping." -ForegroundColor Red
            break
        }
    }
    
    # Summary
    Write-Host "`n📊 Test Summary:" -ForegroundColor Cyan
    Write-Host "  • Total Tests: $totalTests" -ForegroundColor White
    Write-Host "  • Passed: $passedTests" -ForegroundColor Green
    Write-Host "  • Failed: $($totalTests - $passedTests)" -ForegroundColor Red
    Write-Host "  • Success Rate: $(($passedTests / $totalTests * 100).ToString('F1'))%" -ForegroundColor White
    
    # Save results if requested
    if ($SaveResults -and -not $DryRun) {
        $testReport = @{
            Timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
            TestSuite = $TestSuite
            TotalTests = $totalTests
            PassedTests = $passedTests
            FailedTests = ($totalTests - $passedTests)
            SuccessRate = ($passedTests / $totalTests * 100)
            Results = $results
            RepositoryCompliance = "Reused existing test implementations per repository rules"
        }
        
        $testReport | ConvertTo-Json -Depth 3 | Out-File -FilePath $ResultsPath -Encoding UTF8
        Write-Host "💾 Results saved to: $ResultsPath" -ForegroundColor Green
    }
    
    $overallSuccess = ($passedTests -eq $totalTests)
    Write-Host "`n🎯 Overall Result: $(if ($overallSuccess) { '✅ SUCCESS' } else { '❌ FAILURE' })" -ForegroundColor $(if ($overallSuccess) { "Green" } else { "Red" })
    
    exit $(if ($overallSuccess) { 0 } else { 1 })
}

# Execute main function
Main
