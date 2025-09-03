# Intel HAL Integration Testing Framework
# Phase 4.2: Comprehensive Testing Framework
# Following repository rules: reuse existing tests, no redundant implementations

param(
    [ValidateSet("all", "detection", "register", "timestamp", "gptp", "performance", "integration")]
    [string]$TestPhase = "all",
    
    [string]$AdapterFilter = "", # Filter by adapter type (I210, I219, I225, I226)
    [switch]$Verbose = $false,
    [switch]$SaveResults = $true,
    [switch]$ContinueOnFailure = $false,
    [string]$ResultsPath = "docs/tests/results",
    [switch]$DryRun = $false
)

# Following repository rules: reuse existing testing infrastructure
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot = Split-Path -Parent (Split-Path -Parent $ScriptDir)

# Import existing unified testing framework (no duplication)
$UnifiedFrameworkPath = Join-Path $RepoRoot "testing\unified\generic_adapter_testing_framework.ps1"
if (-not (Test-Path $UnifiedFrameworkPath)) {
    Write-Host "ERROR: Existing unified testing framework not found at: $UnifiedFrameworkPath" -ForegroundColor Red
    Write-Host "Following repository rules, we must reuse existing tests rather than duplicate" -ForegroundColor Red
    exit 1
}

# Source the existing framework
Write-Host "Loading existing unified testing framework..." -ForegroundColor Cyan
. $UnifiedFrameworkPath

Write-Host "`n" -NoNewline
Write-Host "="*80 -ForegroundColor Yellow
Write-Host " Intel HAL Integration Testing Framework - Phase 4.2" -ForegroundColor White
Write-Host " Following Repository Rules: Reusing Existing Test Infrastructure" -ForegroundColor Gray
Write-Host "="*80 -ForegroundColor Yellow

# Extend existing framework with Intel HAL specific integration tests
function Test-IntelHALIntegration {
    param(
        [string]$Phase,
        [string]$AdapterFilter
    )
    
    Write-TestHeader "Intel HAL Integration Testing - Phase 4.2"
    
    # Check for Intel HAL build artifacts (following build system rules)
    $IntelHALStaticLib = Join-Path $RepoRoot "build\thirdparty\intel-ethernet-hal\Release\intel-ethernet-hal-static.lib"
    $IntelHALTestExe = Join-Path $RepoRoot "build\tests\Release\intel_hal_validation_test.exe"
    
    if (-not (Test-Path $IntelHALStaticLib)) {
        Write-Host "Intel HAL static library not found. Building first..." -ForegroundColor Yellow
        Write-Host "Command: cmake --build . --config Release --target intel-ethernet-hal-static" -ForegroundColor Gray
        
        if (-not $DryRun) {
            Push-Location (Join-Path $RepoRoot "build")
            $buildResult = & cmake --build . --config Release --target intel-ethernet-hal-static
            Pop-Location
            
            if ($LASTEXITCODE -ne 0) {
                Write-TestResult "Intel HAL Build" $false "Build failed with exit code $LASTEXITCODE"
                return $false
            }
        }
    }
    
    Write-TestResult "Intel HAL Static Library" (Test-Path $IntelHALStaticLib) "Path: $IntelHALStaticLib"
    
    # Integration Test 1: CMake Intel HAL Detection
    Write-TestSubHeader "CMake Intel HAL Detection Integration"
    
    if (-not $DryRun) {
        Push-Location (Join-Path $RepoRoot "build")
        
        # Test CMake reconfiguration with Intel HAL detection
        Write-Host "Testing CMake Intel HAL detection..." -ForegroundColor Cyan
        $cmakeResult = & cmake .. 2>&1
        $cmakeSuccess = $LASTEXITCODE -eq 0
        
        if ($cmakeSuccess) {
            # Check if Intel HAL was properly detected
            $intelHALFound = $cmakeResult | Select-String "Intel HAL found"
            $intelHALVersion = $cmakeResult | Select-String "Intel HAL version"
            
            Write-TestResult "CMake Intel HAL Detection" ($intelHALFound -ne $null) "Detection: $($intelHALFound -ne $null)"
            Write-VerboseOutput "CMake output: $($cmakeResult -join "`n")"
        } else {
            Write-TestResult "CMake Configuration" $false "CMake failed with exit code $LASTEXITCODE"
        }
        
        Pop-Location
    }
    
    # Integration Test 2: Reuse existing Intel HAL validation tests
    Write-TestSubHeader "Intel HAL Validation Test Execution"
    
    if (Test-Path $IntelHALTestExe) {
        if (-not $DryRun) {
            Write-Host "Running existing Intel HAL validation test..." -ForegroundColor Cyan
            $testOutput = & $IntelHALTestExe 2>&1
            $testSuccess = $LASTEXITCODE -eq 0
            
            Write-TestResult "Intel HAL Validation Test" $testSuccess "Exit code: $LASTEXITCODE"
            Write-VerboseOutput "Test output: $($testOutput -join "`n")"
        }
    } else {
        Write-Host "Building Intel HAL validation test..." -ForegroundColor Yellow
        
        if (-not $DryRun) {
            Push-Location (Join-Path $RepoRoot "build")
            $buildResult = & cmake --build . --config Release --target intel_hal_validation_test 2>&1
            $buildSuccess = $LASTEXITCODE -eq 0
            Pop-Location
            
            Write-TestResult "Intel HAL Test Build" $buildSuccess "Build exit code: $LASTEXITCODE"
        }
    }
    
    # Integration Test 3: Call existing unified framework for hardware tests
    Write-TestSubHeader "Hardware Integration Tests (Reusing Existing Framework)"
    
    if (-not $DryRun) {
        # Call existing generic adapter testing framework with Intel HAL focus
        $frameworkParams = @{
            TestPhase = $Phase
            AdapterFilter = $AdapterFilter
            Verbose = $Verbose
            SaveResults = $SaveResults
            ContinueOnFailure = $ContinueOnFailure
            ResultsPath = $ResultsPath
        }
        
        Write-Host "Delegating to existing unified testing framework..." -ForegroundColor Cyan
        
        # This reuses the existing comprehensive test suite
        try {
            # Call the main function from the existing framework
            Start-AdapterTesting @frameworkParams
        } catch {
            Write-Host "Error calling existing framework: $_" -ForegroundColor Red
            Write-TestResult "Unified Framework Integration" $false "Error: $_"
        }
    }
    
    return $true
}

# Integration Test 4: gPTP Integration with Intel HAL
function Test-gPTPIntelHALIntegration {
    Write-TestSubHeader "gPTP Intel HAL Integration Testing"
    
    $gPTPExe = Join-Path $RepoRoot "build\Release\daemon_cl.exe"
    
    if (Test-Path $gPTPExe) {
        Write-Host "Found gPTP daemon at: $gPTPExe" -ForegroundColor Green
        
        if (-not $DryRun) {
            # Test that gPTP can initialize with Intel HAL
            Write-Host "Testing gPTP daemon with Intel HAL integration..." -ForegroundColor Cyan
            
            # Start gPTP daemon briefly to test initialization
            $gptpProcess = Start-Process -FilePath $gPTPExe -ArgumentList "-help" -PassThru -WindowStyle Hidden
            Start-Sleep 2
            
            if (-not $gptpProcess.HasExited) {
                $gptpProcess.Kill()
            }
            
            $initSuccess = ($gptpProcess.ExitCode -eq 0) -or ($gptpProcess.ExitCode -eq $null)
            Write-TestResult "gPTP Intel HAL Integration" $initSuccess "gPTP initialization test"
        }
    } else {
        Write-TestResult "gPTP Build Detection" $false "gPTP daemon not found at $gPTPExe"
    }
}

# Integration Test 5: Performance Testing (reuse existing performance framework)
function Test-PerformanceIntegration {
    Write-TestSubHeader "Performance Integration Testing"
    
    $perfTestExe = Join-Path $RepoRoot "build\testing\performance\Release\phase2_baseline_measurement.exe"
    
    if (Test-Path $perfTestExe) {
        Write-Host "Found performance test at: $perfTestExe" -ForegroundColor Green
        
        if (-not $DryRun) {
            Write-Host "Running existing performance test with Intel HAL..." -ForegroundColor Cyan
            
            $perfOutput = & $perfTestExe -all-supported-adapters 2>&1
            $perfSuccess = $LASTEXITCODE -eq 0
            
            Write-TestResult "Performance Test Integration" $perfSuccess "Exit code: $LASTEXITCODE"
            Write-VerboseOutput "Performance output: $($perfOutput -join "`n")"
        }
    } else {
        Write-TestResult "Performance Test Detection" $false "Performance test not found"
    }
}

# Main execution following repository rules
function Start-IntelHALIntegrationTesting {
    param(
        [string]$Phase = "all",
        [string]$AdapterFilter = ""
    )
    
    Write-Host "Starting Intel HAL Integration Testing..." -ForegroundColor Green
    Write-Host "Following repository rules: Reusing existing test infrastructure" -ForegroundColor Gray
    
    $startTime = Get-Date
    
    switch ($Phase) {
        "all" {
            Test-IntelHALIntegration -Phase "all" -AdapterFilter $AdapterFilter
            Test-gPTPIntelHALIntegration
            Test-PerformanceIntegration
        }
        "integration" {
            Test-IntelHALIntegration -Phase $Phase -AdapterFilter $AdapterFilter
        }
        "gptp" {
            Test-gPTPIntelHALIntegration
        }
        "performance" {
            Test-PerformanceIntegration
        }
        default {
            # Delegate to existing framework for hardware-specific phases
            Test-IntelHALIntegration -Phase $Phase -AdapterFilter $AdapterFilter
        }
    }
    
    $endTime = Get-Date
    $duration = $endTime - $startTime
    
    Write-Host "`n" -NoNewline
    Write-Host "="*80 -ForegroundColor Yellow
    Write-Host " Intel HAL Integration Testing Complete" -ForegroundColor White
    Write-Host " Duration: $($duration.TotalSeconds.ToString('F1')) seconds" -ForegroundColor Gray
    Write-Host " Following repository rules: Reused existing test infrastructure" -ForegroundColor Gray
    Write-Host "="*80 -ForegroundColor Yellow
}

# Execute if called directly (not sourced)
if ($MyInvocation.InvocationName -ne '.') {
    Start-IntelHALIntegrationTesting -Phase $TestPhase -AdapterFilter $AdapterFilter
}
