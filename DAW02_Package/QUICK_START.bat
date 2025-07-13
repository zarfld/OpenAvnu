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
