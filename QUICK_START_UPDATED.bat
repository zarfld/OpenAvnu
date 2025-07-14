@echo off
echo.
echo OpenAvnu DAW02 Quick Start - Updated Fixed Version
echo =================================================
echo.
echo This package contains the corrected test scripts without Unicode issues.
echo.
echo Prerequisites:
echo   - Visual Studio 2022 or Build Tools
echo   - CMake 3.16+
echo   - Npcap SDK or WinPcap
echo.
echo Starting updated tests...
echo.

REM Run the corrected PowerShell script
powershell.exe -ExecutionPolicy Bypass -File "%~dp0run_daw02_tests_fixed.ps1"

echo.
echo Testing completed! Check the test_results_* directory for detailed reports.
echo.
pause
