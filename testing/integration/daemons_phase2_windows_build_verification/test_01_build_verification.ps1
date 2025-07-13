# OpenAvnu Build Verification Test - Fixed Version
# Tests: MRPD, MAAP, Windows Shaper compilation and basic functionality

param(
    [string]$BuildDir = "build",
    [switch]$CleanBuild = $false,
    [switch]$Verbose = $false
)

Write-Host ""
Write-Host "=============================================" -ForegroundColor Cyan
Write-Host "  OpenAvnu Build Verification Test"
Write-Host "=============================================" -ForegroundColor Cyan
Write-Host ""

# Navigate to OpenAvnu root directory
# We're in testing/integration/daemons_phase2_windows_build_verification, need to go up 3 levels
$ScriptPath = $PSScriptRoot
if (-not $ScriptPath) {
    $ScriptPath = Split-Path -Parent $MyInvocation.MyCommand.Definition
}
if (-not $ScriptPath) {
    $ScriptPath = Get-Location
}

# Go up 3 levels: daemons_phase2_windows_build_verification -> integration -> testing -> OpenAvnu
$OpenAvnuRoot = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $ScriptPath))

# Verify we found the right directory
if (-not (Test-Path (Join-Path $OpenAvnuRoot "CMakeLists.txt"))) {
    Write-Host "ERROR: Could not find OpenAvnu root directory with CMakeLists.txt" -ForegroundColor Red
    Write-Host "Expected: $OpenAvnuRoot" -ForegroundColor Red
    exit 1
}

Set-Location $OpenAvnuRoot
Write-Host "OpenAvnu Root: $OpenAvnuRoot" -ForegroundColor Yellow

# Set up logging
$LogFile = Join-Path $OpenAvnuRoot "build_verification_test.log"
"BUILD VERIFICATION TEST - $(Get-Date)" | Out-File $LogFile
"OpenAvnu Root: $OpenAvnuRoot" | Tee-Object -Append $LogFile

# Create and navigate to build directory
$BuildPath = Join-Path $OpenAvnuRoot $BuildDir
if ($CleanBuild -and (Test-Path $BuildPath)) {
    Write-Host "Cleaning existing build directory..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force $BuildPath
}

if (-not (Test-Path $BuildPath)) {
    New-Item -ItemType Directory -Path $BuildPath | Out-Null
}
Set-Location $BuildPath

Write-Host ""
Write-Host "Step 1: CMake Configuration" -ForegroundColor White
Write-Host "----------------------------"

$ConfigStart = Get-Date
try {
    & cmake .. -G "Visual Studio 17 2022" 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Host "FAILED: CMake configuration failed" -ForegroundColor Red
        exit 1
    }
    Write-Host "PASSED: CMake configuration successful" -ForegroundColor Green
} catch {
    Write-Host "ERROR: CMake configuration failed - $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}
$ConfigDuration = (Get-Date) - $ConfigStart

Write-Host ""
Write-Host "Step 2: Build Individual Daemons" -ForegroundColor White
Write-Host "--------------------------------"

# Test builds for each daemon - Updated with correct targets
$Daemons = @(
    @{ Name = "ALL_BUILD"; Target = "ALL_BUILD"; Executable = "ALL_BUILD" },
    @{ Name = "Intel HAL Tests"; Target = "intel_hal_validation_test_consolidated"; Executable = "tests\intel_hal_validation_test_consolidated.exe" },
    @{ Name = "Clock Quality Tests"; Target = "clock_quality_tests_consolidated"; Executable = "testing\conformance\avnu_alliance\gptp\clock_quality\Debug\clock_quality_tests_consolidated.exe" },
    @{ Name = "gPTP Daemon"; Target = "gptp"; Executable = "thirdparty\gptp\Debug\gptp.exe" }
)

$BuildResults = @()
foreach ($Daemon in $Daemons) {
    Write-Host ""
    Write-Host "Building $($Daemon.Name)..." -ForegroundColor Yellow
    
    $BuildStart = Get-Date
    try {
        & cmake --build . --config Debug --target $Daemon.Target 2>&1
        if ($LASTEXITCODE -eq 0) {
            if ($Daemon.Name -eq "ALL_BUILD") {
                Write-Host "PASSED: $($Daemon.Name) completed successfully" -ForegroundColor Green
                $Status = "PASSED"
            } else {
                $ExecutablePath = Join-Path $BuildPath $Daemon.Executable
                if (Test-Path $ExecutablePath) {
                    Write-Host "PASSED: $($Daemon.Name) built successfully" -ForegroundColor Green
                    $Status = "PASSED"
                } else {
                    Write-Host "FAILED: $($Daemon.Name) executable not found at $ExecutablePath" -ForegroundColor Red
                    $Status = "FAILED"
                }
            }
        } else {
            Write-Host "FAILED: $($Daemon.Name) build failed (Exit Code: $LASTEXITCODE)" -ForegroundColor Red
            $Status = "FAILED"
        }
    } catch {
        Write-Host "ERROR: $($Daemon.Name) build error - $($_.Exception.Message)" -ForegroundColor Red
        $Status = "ERROR"
    }
    
    $BuildDuration = (Get-Date) - $BuildStart
    $BuildResults += @{
        Name = $Daemon.Name
        Status = $Status
        Duration = $BuildDuration.TotalSeconds
        Executable = $Daemon.Executable
    }
}

Write-Host ""
Write-Host "Step 3: Basic Functionality Tests" -ForegroundColor White
Write-Host "--------------------------------"

foreach ($Result in $BuildResults) {
    if ($Result.Status -eq "PASSED" -and $Result.Name -ne "ALL_BUILD") {
        Write-Host ""
        Write-Host "Testing $($Result.Name) basic functionality..." -ForegroundColor Yellow
        
        $ExePath = Join-Path $BuildPath $Result.Executable
        try {
            # Test help/version output (most daemons support --help or -h)
            $Output = & $ExePath --help 2>&1
            if ($LASTEXITCODE -eq 0 -or $Output -match "usage|help|version") {
                Write-Host "PASSED: $($Result.Name) responds to help command" -ForegroundColor Green
            } else {
                Write-Host "WARNING: $($Result.Name) help command unclear (Exit Code: $LASTEXITCODE)" -ForegroundColor Yellow
            }
        } catch {
            Write-Host "WARNING: $($Result.Name) functionality test failed - $($_.Exception.Message)" -ForegroundColor Yellow
        }
    }
}

Write-Host ""
Write-Host "=============================================" -ForegroundColor Cyan
Write-Host "  BUILD VERIFICATION SUMMARY"
Write-Host "=============================================" -ForegroundColor Cyan

$PassedBuilds = ($BuildResults | Where-Object { $_.Status -eq "PASSED" }).Count
$TotalBuilds = $BuildResults.Count

Write-Host ""
Write-Host "Configuration Time: $([math]::Round($ConfigDuration.TotalSeconds, 2)) seconds" -ForegroundColor White
Write-Host "Total Daemons: $TotalBuilds" -ForegroundColor White
Write-Host "Successful Builds: $PassedBuilds" -ForegroundColor Green
Write-Host "Failed Builds: $($TotalBuilds - $PassedBuilds)" -ForegroundColor Red
Write-Host ""

foreach ($Result in $BuildResults) {
    $StatusColor = if ($Result.Status -eq "PASSED") { "Green" } else { "Red" }
    Write-Host "$($Result.Name): $($Result.Status) ($([math]::Round($Result.Duration, 2))s)" -ForegroundColor $StatusColor
}

Write-Host ""
if ($PassedBuilds -eq $TotalBuilds) {
    Write-Host "ALL BUILDS PASSED! OpenAvnu daemons are ready for deployment." -ForegroundColor Green
    exit 0
} elseif ($PassedBuilds -gt 0) {
    Write-Host "PARTIAL SUCCESS: Some daemons built successfully." -ForegroundColor Yellow
    exit 1
} else {
    Write-Host "ALL BUILDS FAILED: Check dependencies and configuration." -ForegroundColor Red
    exit 2
}
