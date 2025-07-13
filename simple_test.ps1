# DAW02 OpenAvnu Test Suite (Simple Version)
# Tests compilation fixes for WinSock headers and Intel HAL integration

$ErrorActionPreference = "Continue"

Write-Host "Starting DAW02 OpenAvnu Test Suite..." -ForegroundColor Green
Write-Host "Testing WinSock header fixes and Intel HAL integration"
Write-Host ""

# Create build directory if it doesn't exist
if (!(Test-Path "build")) {
    Write-Host "Creating build directory..." -ForegroundColor Yellow
    New-Item -ItemType Directory -Path "build" | Out-Null
}

# Test 1: CMake Configuration
Write-Host "Test 1: CMake Configuration" -ForegroundColor Blue
Set-Location "build"
$configResult = & cmake .. -G "Visual Studio 17 2022" 2>&1
$configSuccess = $LASTEXITCODE -eq 0

if ($configSuccess) {
    Write-Host "  [PASSED] CMake configuration successful" -ForegroundColor Green
} else {
    Write-Host "  [FAILED] CMake configuration failed" -ForegroundColor Red
    Write-Host "Error output:"
    $configResult | Where-Object { $_ -match "error|Error|ERROR" }
}

Write-Host ""

# Test 2: Build MAAP Daemon
Write-Host "Test 2: MAAP Daemon Build" -ForegroundColor Blue
$maapResult = & cmake --build . --config Release --target maap_daemon 2>&1
$maapSuccess = $LASTEXITCODE -eq 0

if ($maapSuccess) {
    Write-Host "  [PASSED] MAAP daemon build successful" -ForegroundColor Green
} else {
    Write-Host "  [FAILED] MAAP daemon build failed" -ForegroundColor Red
    Write-Host "Error output:"
    $maapResult | Where-Object { $_ -match "error|Error|ERROR" } | Select-Object -First 10
}

Write-Host ""

# Test 3: Build MRPD Daemon  
Write-Host "Test 3: MRPD Daemon Build" -ForegroundColor Blue
$mrpdResult = & cmake --build . --config Release --target mrpd 2>&1
$mrpdSuccess = $LASTEXITCODE -eq 0

if ($mrpdSuccess) {
    Write-Host "  [PASSED] MRPD daemon build successful" -ForegroundColor Green
} else {
    Write-Host "  [FAILED] MRPD daemon build failed" -ForegroundColor Red
    Write-Host "Error output:"
    $mrpdResult | Where-Object { $_ -match "error|Error|ERROR" } | Select-Object -First 10
}

Write-Host ""

# Test 4: Build Shaper
Write-Host "Test 4: Shaper Build" -ForegroundColor Blue
$shaperResult = & cmake --build . --config Release --target shaper 2>&1
$shaperSuccess = $LASTEXITCODE -eq 0

if ($shaperSuccess) {
    Write-Host "  [PASSED] Shaper build successful" -ForegroundColor Green
} else {
    Write-Host "  [FAILED] Shaper build failed" -ForegroundColor Red
    Write-Host "Error output:"
    $shaperResult | Where-Object { $_ -match "error|Error|ERROR" } | Select-Object -First 10
}

Write-Host ""

# Test 5: Build Intel HAL Validation Test
Write-Host "Test 5: Intel HAL Validation Test Build" -ForegroundColor Blue
$halResult = & cmake --build . --config Release --target intel_hal_validation_test 2>&1
$halSuccess = $LASTEXITCODE -eq 0

if ($halSuccess) {
    Write-Host "  [PASSED] Intel HAL validation test build successful" -ForegroundColor Green
} else {
    Write-Host "  [FAILED] Intel HAL validation test build failed" -ForegroundColor Red  
    Write-Host "Error output:"
    $halResult | Where-Object { $_ -match "error|Error|ERROR" } | Select-Object -First 10
}

Write-Host ""

# Summary
$totalTests = 5
$passedTests = @($configSuccess, $maapSuccess, $mrpdSuccess, $shaperSuccess, $halSuccess) | Where-Object { $_ } | Measure-Object | Select-Object -ExpandProperty Count

Write-Host "=== TEST SUMMARY ===" -ForegroundColor Cyan
Write-Host "Total Tests: $totalTests"
Write-Host "Passed: $passedTests" -ForegroundColor Green
Write-Host "Failed: $($totalTests - $passedTests)" -ForegroundColor Red
Write-Host "Success Rate: $([math]::Round(($passedTests / $totalTests) * 100, 1))%"

if ($passedTests -eq $totalTests) {
    Write-Host ""
    Write-Host "ALL TESTS PASSED! WinSock headers and Intel HAL integration fixes successful!" -ForegroundColor Green
    exit 0
} else {
    Write-Host ""
    Write-Host "Some tests failed. Build issues remain." -ForegroundColor Red
    exit 1
}
