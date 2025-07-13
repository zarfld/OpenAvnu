@echo off
REM Generic Intel Adapter Testing - Quick Launcher
REM Windows batch script for easy test execution

echo ===============================================================================
echo  Generic Intel Adapter Testing Framework - Quick Launcher
echo ===============================================================================
echo.

set "SCRIPT_PATH=%~dp0generic_adapter_testing_framework.ps1"

if not exist "%SCRIPT_PATH%" (
    echo ERROR: Testing framework not found at %SCRIPT_PATH%
    echo Please ensure you're running this from the testing/unified directory.
    pause
    exit /b 1
)

echo Available Test Options:
echo.
echo  1. List Available Adapters
echo  2. Run All Tests (Recommended)
echo  3. Test I219 Adapters Only
echo  4. Test I225/I226 Adapters Only
echo  5. Detection Phase Only
echo  6. Register Access Tests Only
echo  7. Timestamp Tests Only
echo  8. gPTP Integration Tests Only
echo  9. Performance Tests Only
echo  A. Advanced: Continue on Failure
echo  B. Advanced: Dry Run (No actual testing)
echo  Q. Quit
echo.

set /p choice="Enter your choice (1-9, A, B, or Q): "

if /i "%choice%"=="1" goto list_adapters
if /i "%choice%"=="2" goto run_all_tests
if /i "%choice%"=="3" goto test_i219
if /i "%choice%"=="4" goto test_i225
if /i "%choice%"=="5" goto detection_only
if /i "%choice%"=="6" goto register_tests
if /i "%choice%"=="7" goto timestamp_tests
if /i "%choice%"=="8" goto gptp_tests
if /i "%choice%"=="9" goto performance_tests
if /i "%choice%"=="A" goto continue_on_failure
if /i "%choice%"=="B" goto dry_run
if /i "%choice%"=="Q" goto quit

echo Invalid choice. Please try again.
pause
goto :eof

:list_adapters
echo.
echo Running: List Available Adapters
echo.
powershell -ExecutionPolicy Bypass -File "%SCRIPT_PATH%" -ListAdapters
goto end

:run_all_tests
echo.
echo Running: All Test Phases (Recommended)
echo This will run comprehensive testing on all detected Intel adapters.
echo.
powershell -ExecutionPolicy Bypass -File "%SCRIPT_PATH%" -SaveResults -Verbose
goto end

:test_i219
echo.
echo Running: I219 Adapter Tests Only
echo.
powershell -ExecutionPolicy Bypass -File "%SCRIPT_PATH%" -AdapterFilter "I219" -SaveResults -Verbose
goto end

:test_i225
echo.
echo Running: I225/I226 Adapter Tests Only
echo.
powershell -ExecutionPolicy Bypass -File "%SCRIPT_PATH%" -AdapterFilter "I225" -SaveResults -Verbose
goto end

:detection_only
echo.
echo Running: Detection Phase Only
echo.
powershell -ExecutionPolicy Bypass -File "%SCRIPT_PATH%" -TestPhase "detection" -Verbose
goto end

:register_tests
echo.
echo Running: Register Access Tests Only
echo.
powershell -ExecutionPolicy Bypass -File "%SCRIPT_PATH%" -TestPhase "register" -SaveResults -Verbose
goto end

:timestamp_tests
echo.
echo Running: Timestamp Capability Tests Only
echo.
powershell -ExecutionPolicy Bypass -File "%SCRIPT_PATH%" -TestPhase "timestamp" -SaveResults -Verbose
goto end

:gptp_tests
echo.
echo Running: gPTP Integration Tests Only
echo.
powershell -ExecutionPolicy Bypass -File "%SCRIPT_PATH%" -TestPhase "gptp" -SaveResults -Verbose
goto end

:performance_tests
echo.
echo Running: Performance Tests Only
echo.
powershell -ExecutionPolicy Bypass -File "%SCRIPT_PATH%" -TestPhase "performance" -SaveResults -Verbose
goto end

:continue_on_failure
echo.
echo Running: All Tests with Continue on Failure
echo This will run all test phases and continue even if some phases fail.
echo.
powershell -ExecutionPolicy Bypass -File "%SCRIPT_PATH%" -ContinueOnFailure -SaveResults -Verbose
goto end

:dry_run
echo.
echo Running: Dry Run (Detection Only, No Testing)
echo.
powershell -ExecutionPolicy Bypass -File "%SCRIPT_PATH%" -DryRun -Verbose
goto end

:quit
echo.
echo Exiting...
exit /b 0

:end
echo.
echo ===============================================================================
echo  Test execution completed.
echo.
echo Results have been saved to docs/tests/results/ (if -SaveResults was used)
echo Check the output above for detailed results and recommendations.
echo ===============================================================================
echo.
pause
