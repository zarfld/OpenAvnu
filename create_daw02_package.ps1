# Quick DAW02 Testing Package Creator for Windows
# PowerShell script to bundle testing files

$PackageName = "OpenAvnu_DAW02_Testing_Package_$(Get-Date -Format 'yyyyMMdd_HHmmss')"
$PackageDir = $PackageName

Write-Host "📦 Creating OpenAvnu DAW02 Testing Package for Windows..." -ForegroundColor Green
Write-Host "Package: $PackageName"
Write-Host ""

# Create package directory
New-Item -ItemType Directory -Path $PackageDir -Force | Out-Null

# Copy core testing files
Write-Host "Copying testing scripts..." -ForegroundColor Yellow
Copy-Item "run_daw02_tests.ps1" "$PackageDir\" -ErrorAction SilentlyContinue
Copy-Item "test_01_build_verification.ps1" "$PackageDir\" -ErrorAction SilentlyContinue
Copy-Item "test_02_intel_hal_detection.ps1" "$PackageDir\" -ErrorAction SilentlyContinue

# Copy documentation
Write-Host "Copying documentation..." -ForegroundColor Yellow
Copy-Item "DAW02_TESTING_PLAN.md" "$PackageDir\" -ErrorAction SilentlyContinue
Copy-Item "DAW02_TESTING_README.md" "$PackageDir\" -ErrorAction SilentlyContinue
Copy-Item "DAEMON_MODERNIZATION_BUILD_VERIFICATION_REPORT.md" "$PackageDir\" -ErrorAction SilentlyContinue

# Create quick start batch file
@"
@echo off
echo.
echo OpenAvnu DAW02 Quick Start for Windows
echo =====================================
echo.
echo This package contains comprehensive testing for OpenAvnu daemon modernization.
echo.
echo Prerequisites:
echo   - Visual Studio 2022 or Build Tools
echo   - CMake 3.16+
echo   - Npcap SDK or WinPcap
echo.
echo Ready to start testing!
echo.
pause
powershell.exe -ExecutionPolicy Bypass -File "run_daw02_tests.ps1"
"@ | Out-File -FilePath "$PackageDir\QUICK_START.bat" -Encoding ASCII

# Create simple README
@"
# OpenAvnu DAW02 Testing Package for Windows

Created: $(Get-Date)
Package: $PackageName

## Quick Start

Double-click QUICK_START.bat to begin testing

OR

Open PowerShell and run: .\run_daw02_tests.ps1

## Prerequisites

- Windows 10/11
- Visual Studio 2022 or Visual Studio Build Tools
- CMake 3.16+
- Npcap SDK or WinPcap Developer Pack

## Expected Output

Results will be saved in test_results_* directory
Transfer the results back for analysis after testing

---
OpenAvnu DAW02 Testing Package
"@ | Out-File -FilePath "$PackageDir\README.md" -Encoding UTF8

# Create archive
Write-Host "Creating archive..." -ForegroundColor Yellow
try {
    Compress-Archive -Path $PackageDir -DestinationPath "$PackageName.zip" -Force
    Write-Host "✅ ZIP archive created: $PackageName.zip" -ForegroundColor Green
} catch {
    Write-Host "⚠️  Could not create ZIP archive: $($_.Exception.Message)" -ForegroundColor Yellow
}

# Summary
Write-Host ""
Write-Host "✅ Windows testing package created successfully!" -ForegroundColor Green
Write-Host ""
Write-Host "📁 Package Directory: $PackageDir\" -ForegroundColor Cyan
Write-Host "📦 Windows Archive: $PackageName.zip" -ForegroundColor Cyan
Write-Host ""
Write-Host "🚀 Ready for deployment to Windows DAW02!" -ForegroundColor Green
Write-Host ""
Write-Host "Transfer to DAW02:" -ForegroundColor Yellow
Write-Host "1. Copy $PackageName.zip to DAW02"
Write-Host "2. Extract and double-click QUICK_START.bat"
Write-Host "3. Review generated test reports"
Write-Host "4. Transfer results back for analysis"
