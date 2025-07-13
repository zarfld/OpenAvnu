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
Copy-Item "run_daw02_tests.ps1" "$PackageDir\"
Copy-Item "test_01_build_verification.ps1" "$PackageDir\"
Copy-Item "test_02_intel_hal_detection.ps1" "$PackageDir\"

# Copy documentation
Write-Host "Copying documentation..." -ForegroundColor Yellow
Copy-Item "DAW02_TESTING_PLAN.md" "$PackageDir\" -ErrorAction SilentlyContinue
Copy-Item "DAW02_TESTING_README.md" "$PackageDir\" -ErrorAction SilentlyContinue
Copy-Item "DAEMON_MODERNIZATION_BUILD_VERIFICATION_REPORT.md" "$PackageDir\" -ErrorAction SilentlyContinue

# Create quick start batch file for Windows
$BatchContent = @'
@echo off
echo.
echo 🧪 OpenAvnu DAW02 Quick Start for Windows
echo ========================================
echo.
echo This package contains comprehensive testing for OpenAvnu daemon modernization.
echo.
echo Quick Commands:
echo   run_daw02_tests.ps1           - Run complete test suite
echo   test_01_build_verification.ps1 - Test build only  
echo   test_02_intel_hal_detection.ps1 - Test Intel HAL only
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
'@

$BatchContent | Out-File -FilePath "$PackageDir\QUICK_START.bat" -Encoding ASCII

# Create PowerShell quick start
$QuickStartContent = @'
# OpenAvnu DAW02 Quick Start for Windows PowerShell

Write-Host "🧪 OpenAvnu DAW02 Quick Start for Windows" -ForegroundColor Green
Write-Host "=========================================="
Write-Host ""
Write-Host "This package contains comprehensive testing for OpenAvnu daemon modernization."
Write-Host ""
Write-Host "Quick Commands:" -ForegroundColor Yellow
Write-Host "  .\run_daw02_tests.ps1           # Run complete test suite"
Write-Host "  .\test_01_build_verification.ps1 # Test build only"
Write-Host "  .\test_02_intel_hal_detection.ps1 # Test Intel HAL only"
Write-Host ""
Write-Host "Prerequisites:" -ForegroundColor Yellow
Write-Host "  - Visual Studio 2022 or Build Tools"
Write-Host "  - CMake 3.16+"
Write-Host "  - Npcap SDK or WinPcap"
Write-Host ""
Write-Host "Ready to start testing! Run: .\run_daw02_tests.ps1" -ForegroundColor Green
'@

$QuickStartContent | Out-File -FilePath "$PackageDir\QUICK_START.ps1" -Encoding UTF8

# Create README for the package
$ReadmeContent = @"
# OpenAvnu DAW02 Testing Package for Windows

Created: $(Get-Date)
Package: $PackageName

## Quick Start

### Option 1: Double-click
1. Double-click QUICK_START.bat
2. Follow the prompts

### Option 2: PowerShell
1. Right-click folder and "Open PowerShell window here"
2. Run: .\run_daw02_tests.ps1

### Option 3: Individual Tests
- Build test only: .\test_01_build_verification.ps1
- Intel HAL test only: .\test_02_intel_hal_detection.ps1

## Prerequisites

- Windows 10/11
- Visual Studio 2022 or Visual Studio Build Tools
- CMake 3.16+ (available in VS or standalone)
- Npcap SDK or WinPcap Developer Pack

## Expected Results

Success: All tests pass, ready for production
Partial: Some tests fail, review detailed logs
Failure: Major issues, requires investigation

## Output

Results will be saved in test_results_* directory:
- Master report with summary
- Detailed logs for each test
- System information

## Transfer Results

After testing, compress the results:
Compress-Archive -Path test_results_* -DestinationPath daw02_results.zip

Then send daw02_results.zip back for analysis.

---
OpenAvnu DAW02 Testing Package
"@

$ReadmeContent | Out-File -FilePath "$PackageDir\README.md" -Encoding UTF8

# Create archive
Write-Host "Creating archive..." -ForegroundColor Yellow
try {
    Compress-Archive -Path $PackageDir -DestinationPath "$PackageName.zip" -Force
    Write-Host "✅ ZIP archive created: $PackageName.zip" -ForegroundColor Green
} catch {
    Write-Host "⚠️  Could not create ZIP archive: $($_.Exception.Message)" -ForegroundColor Yellow
}

# Generate transfer instructions
$TransferInstructions = @"
OpenAvnu DAW02 Testing Package Transfer Instructions (Windows)
=============================================================

Package Created: $(Get-Date)
Package Name: $PackageName

Files Generated:
- $PackageName.zip      (Windows ZIP archive)
- $PackageDir\          (Directory with all files)

Transfer to DAW02:
==================

Option 1: Network Share
Copy $PackageName.zip to shared network drive
Extract on DAW02
Double-click QUICK_START.bat or run run_daw02_tests.ps1

Option 2: USB/Physical Media  
Copy $PackageName.zip to USB drive
Transfer to DAW02
Extract and run QUICK_START.bat

Option 3: Email/Cloud
Send $PackageName.zip via email or cloud storage
Download and extract on DAW02
Run QUICK_START.bat

On DAW02:
=========
1. Extract $PackageName.zip
2. Double-click QUICK_START.bat OR
3. Open PowerShell and run: .\run_daw02_tests.ps1
4. Review results in test_results_* directory
5. Compress results: Compress-Archive -Path test_results_* -DestinationPath daw02_results.zip
6. Transfer daw02_results.zip back for analysis

Expected Runtime: 15-30 minutes
Expected Output: Comprehensive test report with pass/fail status

Troubleshooting:
===============
- If execution policy blocks scripts: Run PowerShell as Administrator, then:
  Set-ExecutionPolicy -ExecutionPolicy Bypass -Scope Process
- If Visual Studio not found: Install Visual Studio 2022 Community (free)
- If CMake not found: Install from https://cmake.org or enable in VS installer
- If PCAP not found: Install Npcap from https://npcap.com

Support:
========
Generated test reports will contain detailed error information
for debugging and resolution.
"@

$TransferInstructions | Out-File -FilePath "${PackageName}_TRANSFER_INSTRUCTIONS.txt" -Encoding UTF8

# Summary
Write-Host ""
Write-Host "✅ Windows testing package created successfully!" -ForegroundColor Green
Write-Host ""
Write-Host "📁 Package Directory: $PackageDir\" -ForegroundColor Cyan
Write-Host "📦 Windows Archive: $PackageName.zip" -ForegroundColor Cyan
Write-Host "📋 Transfer Instructions: ${PackageName}_TRANSFER_INSTRUCTIONS.txt" -ForegroundColor Cyan
Write-Host ""
Write-Host "🚀 Ready for deployment to Windows DAW02!" -ForegroundColor Green
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Yellow
Write-Host "1. Transfer $PackageName.zip to DAW02"
Write-Host "2. Extract and run QUICK_START.bat"
Write-Host "3. Review generated test reports"
Write-Host "4. Transfer results back for analysis"
