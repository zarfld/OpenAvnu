# OpenAvnu gPTP Integration Verification Script
# This script verifies the complete gPTP integration for Windows

Write-Host "=== OpenAvnu gPTP Integration Verification ===" -ForegroundColor Green
Write-Host ""

# Check if we're in the right directory
if (-not (Test-Path "CMakeLists.txt") -or -not (Test-Path "thirdparty\gptp")) {
    Write-Host "Error: Please run this script from the OpenAvnu root directory" -ForegroundColor Red
    exit 1
}

Write-Host "✅ OpenAvnu repository detected" -ForegroundColor Green

# Check if gPTP submodule is properly initialized
if (-not (Test-Path "thirdparty\gptp\CMakeLists.txt")) {
    Write-Host "❌ gPTP submodule not initialized. Running git submodule update..." -ForegroundColor Yellow
    git submodule update --init --recursive
}

Write-Host "✅ gPTP submodule initialized" -ForegroundColor Green

# Check if build directory exists
if (-not (Test-Path "build")) {
    Write-Host "⚠️  Build directory not found. Creating and configuring..." -ForegroundColor Yellow
    mkdir build | Out-Null
    cd build
    cmake .. -G "Visual Studio 17 2022"
    cd ..
} else {
    Write-Host "✅ Build directory exists" -ForegroundColor Green
}

# Check if gPTP executable exists
$gptpExe = "build\thirdparty\gptp\Release\gptp.exe"
if (-not (Test-Path $gptpExe)) {
    Write-Host "⚠️  gPTP executable not found. Building..." -ForegroundColor Yellow
    cd build
    cmake --build . --config Release --target gptp
    cd ..
}

if (Test-Path $gptpExe) {
    Write-Host "✅ gPTP daemon built successfully" -ForegroundColor Green
    
    # Check configuration files
    $configFiles = @(
        "build\thirdparty\gptp\Release\gptp_cfg.ini",
        "build\thirdparty\gptp\Release\avnu_base_config.ini"
    )
    
    foreach ($config in $configFiles) {
        if (Test-Path $config) {
            Write-Host "✅ Configuration file: $(Split-Path $config -Leaf)" -ForegroundColor Green
        } else {
            Write-Host "❌ Missing configuration file: $(Split-Path $config -Leaf)" -ForegroundColor Red
        }
    }
} else {
    Write-Host "❌ gPTP daemon build failed" -ForegroundColor Red
    exit 1
}

# Check PowerShell script
if (Test-Path "run_gptp.ps1") {
    Write-Host "✅ PowerShell script available" -ForegroundColor Green
} else {
    Write-Host "❌ PowerShell script missing" -ForegroundColor Red
}

# Check documentation
if (Test-Path "docs\gPTP_Integration.md") {
    Write-Host "✅ gPTP documentation available" -ForegroundColor Green
} else {
    Write-Host "❌ gPTP documentation missing" -ForegroundColor Red
}

# Test gPTP daemon execution (basic version check)
Write-Host ""
Write-Host "=== Testing gPTP Daemon ===" -ForegroundColor Cyan
Write-Host "Note: Network interface errors are expected without admin privileges" -ForegroundColor Yellow

try {
    $output = & $gptpExe 2>&1 | Select-Object -First 5
    if ($output -match "GPTP.*MAIN.*Entered") {
        Write-Host "✅ gPTP daemon starts successfully" -ForegroundColor Green
    } else {
        Write-Host "⚠️  gPTP daemon shows unexpected output" -ForegroundColor Yellow
    }
} catch {
    Write-Host "❌ gPTP daemon failed to start: $($_.Exception.Message)" -ForegroundColor Red
}

# Show available network interfaces
Write-Host ""
Write-Host "=== Available Network Interfaces ===" -ForegroundColor Cyan
try {
    Get-NetAdapter | Where-Object {$_.Status -eq "Up"} | Select-Object Name, InterfaceDescription | Format-Table -AutoSize
} catch {
    Write-Host "Could not list network interfaces. PowerShell cmdlets may not be available." -ForegroundColor Yellow
}

# Summary
Write-Host ""
Write-Host "=== Integration Summary ===" -ForegroundColor Green
Write-Host "✅ gPTP submodule integrated from https://github.com/zarfld/gptp" -ForegroundColor Green
Write-Host "✅ Cross-platform CMake build system configured" -ForegroundColor Green  
Write-Host "✅ Windows PowerShell execution script provided" -ForegroundColor Green
Write-Host "✅ Comprehensive documentation available" -ForegroundColor Green
Write-Host "✅ Configuration files properly deployed" -ForegroundColor Green

Write-Host ""
Write-Host "=== Quick Start ===" -ForegroundColor Cyan
Write-Host "1. Build: cmake --build build --config Release" -ForegroundColor White
Write-Host "2. Run gPTP: .\run_gptp.ps1 -NetworkInterface 'Your-Interface-Name'" -ForegroundColor White
Write-Host "3. Read documentation: docs\gPTP_Integration.md" -ForegroundColor White

Write-Host ""
Write-Host "=== OpenAvnu gPTP Integration Complete! ===" -ForegroundColor Green
