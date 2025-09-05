#!/usr/bin/env powershell
<#
.SYNOPSIS
    Milan Hardware Integration Test - Build and Run
    
.DESCRIPTION
    Builds and runs the Milan hardware integration test that can be deployed
    to different machines for real hardware testing with Intel NICs.
    
.PARAMETER Clean
    Clean build directory before building
    
.PARAMETER ConfigOnly
    Only configure CMake, don't build
    
.PARAMETER TestOnly
    Only run the test, don't build
    
.EXAMPLE
    .\test_milan_hardware_on_different_machine.ps1
    
.EXAMPLE
    .\test_milan_hardware_on_different_machine.ps1 -Clean
#>

param(
    [switch]$Clean,
    [switch]$ConfigOnly,
    [switch]$TestOnly
)

# Error handling
$ErrorActionPreference = "Stop"

# Configuration
$PROJECT_ROOT = Get-Location
$BUILD_DIR = Join-Path $PROJECT_ROOT "build"
$TEST_EXECUTABLE = Join-Path $BUILD_DIR "Release\test_milan_hardware_integration.exe"

function Write-Header {
    param([string]$Text)
    Write-Host ""
    Write-Host "=== $Text ===" -ForegroundColor Cyan
    Write-Host ""
}

function Write-Success {
    param([string]$Text)
    Write-Host "✅ $Text" -ForegroundColor Green
}

function Write-Info {
    param([string]$Text)
    Write-Host "ℹ️  $Text" -ForegroundColor Blue
}

function Write-Warning {
    param([string]$Text)
    Write-Host "⚠️  $Text" -ForegroundColor Yellow
}

function Write-Error-Custom {
    param([string]$Text)
    Write-Host "❌ $Text" -ForegroundColor Red
}

Write-Header "Milan Hardware Integration Test"
Write-Host "Testing hardware integration framework for deployment to different machines" -ForegroundColor White
Write-Host "This test validates hardware detection and Milan professional audio capabilities" -ForegroundColor Gray

# Clean if requested
if ($Clean) {
    Write-Header "Cleaning Build Directory"
    if (Test-Path $BUILD_DIR) {
        Remove-Item -Recurse -Force $BUILD_DIR
        Write-Success "Build directory cleaned"
    } else {
        Write-Info "Build directory already clean"
    }
}

# Create build directory
Write-Header "Setting Up Build Environment"
if (!(Test-Path $BUILD_DIR)) {
    New-Item -ItemType Directory -Path $BUILD_DIR | Out-Null
    Write-Success "Created build directory"
} else {
    Write-Info "Build directory exists"
}

# CMake Configuration
if (!$TestOnly) {
    Write-Header "Configuring CMake"
    
    Push-Location $BUILD_DIR
    try {
        Write-Info "Running CMake configuration with Milan hardware test enabled..."
        
        $cmake_args = @(
            ".."
            "-G", "Visual Studio 17 2022"
            "-DBUILD_MILAN_HARDWARE_TEST=ON"
            "-DOPENAVNU_BUILD_MILAN_V1_2=ON"
            "-DOPENAVNU_BUILD_INTEL_HAL=ON"
        )
        
        & cmake @cmake_args
        
        if ($LASTEXITCODE -ne 0) {
            throw "CMake configuration failed with exit code $LASTEXITCODE"
        }
        
        Write-Success "CMake configuration completed"
        
    } catch {
        Write-Error-Custom "CMake configuration failed: $($_.Exception.Message)"
        Pop-Location
        exit 1
    } finally {
        Pop-Location
    }
}

# Stop here if config only
if ($ConfigOnly) {
    Write-Success "Configuration completed. Use -TestOnly to run tests."
    exit 0
}

# Build
if (!$TestOnly) {
    Write-Header "Building Milan Hardware Integration Test"
    
    Push-Location $BUILD_DIR
    try {
        Write-Info "Building test executable..."
        
        & cmake --build . --config Release --target test_milan_hardware_integration
        
        if ($LASTEXITCODE -ne 0) {
            throw "Build failed with exit code $LASTEXITCODE"
        }
        
        Write-Success "Build completed successfully"
        
        # Verify executable exists
        if (!(Test-Path $TEST_EXECUTABLE)) {
            throw "Test executable not found at expected location: $TEST_EXECUTABLE"
        }
        
        Write-Success "Test executable ready: $TEST_EXECUTABLE"
        
    } catch {
        Write-Error-Custom "Build failed: $($_.Exception.Message)"
        Pop-Location
        exit 1
    } finally {
        Pop-Location
    }
}

# Run Hardware Test
Write-Header "Running Milan Hardware Integration Test"

if (!(Test-Path $TEST_EXECUTABLE)) {
    Write-Error-Custom "Test executable not found. Run without -TestOnly to build first."
    exit 1
}

try {
    Write-Info "Starting hardware integration test..."
    Write-Info "This test will:"
    Write-Host "  • Detect available network interfaces" -ForegroundColor Gray
    Write-Host "  • Test basic network capabilities" -ForegroundColor Gray
    Write-Host "  • Simulate Milan timing requirements" -ForegroundColor Gray
    Write-Host "  • Validate professional audio formats" -ForegroundColor Gray
    Write-Host "  • Test stream redundancy simulation" -ForegroundColor Gray
    Write-Host "  • Perform end-to-end system validation" -ForegroundColor Gray
    Write-Host ""
    
    # Run the test
    & $TEST_EXECUTABLE
    
    if ($LASTEXITCODE -eq 0) {
        Write-Header "Hardware Integration Test Results"
        Write-Success "ALL TESTS PASSED!"
        Write-Success "Hardware integration framework validated"
        Write-Success "Ready for deployment to Intel NIC hardware"
        
        Write-Host ""
        Write-Host "📋 Deployment Instructions:" -ForegroundColor Magenta
        Write-Host "1. Copy test_milan_hardware_integration.exe to target machine" -ForegroundColor White
        Write-Host "2. Ensure Intel NIC drivers are installed (I219/I225/I226)" -ForegroundColor White
        Write-Host "3. Run as Administrator for hardware access" -ForegroundColor White
        Write-Host "4. Test will detect Intel NICs and validate Milan capabilities" -ForegroundColor White
        
    } else {
        Write-Error-Custom "Hardware integration test failed with exit code $LASTEXITCODE"
        exit 1
    }
    
} catch {
    Write-Error-Custom "Test execution failed: $($_.Exception.Message)"
    exit 1
}

Write-Header "Milan Hardware Integration Test Complete"
Write-Success "Hardware integration framework ready for deployment!"
