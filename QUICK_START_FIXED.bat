@echo off
echo.
echo OpenAvnu DAW02 Quick Start - Fixed Version
echo =========================================
echo.
echo This will run the OpenAvnu testing suite on DAW02.
echo.
echo Prerequisites:
echo   - Visual Studio 2022 or Build Tools
echo   - CMake 3.16+
echo   - Npcap SDK or WinPcap
echo.
echo Starting tests...
echo.

REM Copy the fixed script to current directory
copy /Y "%~dp0..\..\run_daw02_tests_fixed.ps1" "%~dp0run_daw02_tests_fixed.ps1" >nul 2>&1

REM Run the fixed PowerShell script
powershell.exe -ExecutionPolicy Bypass -File "%~dp0run_daw02_tests_fixed.ps1"

echo.
echo Testing completed! Check the test_results_* directory for detailed reports.
echo.
pause
