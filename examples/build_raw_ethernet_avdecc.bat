@echo off
echo Building Raw Ethernet AVDECC Entity with Npcap Support...

REM Check if we're in the examples directory
if not exist "raw_ethernet_avdecc_entity.c" (
    echo Error: raw_ethernet_avdecc_entity.c not found
    echo Please run this script from the examples directory
    pause
    exit /b 1
)

REM Check for Npcap SDK
set NPCAP_FOUND=0
if exist "%NPCAP_DIR%\Include\pcap.h" (
    set NPCAP_FOUND=1
    set NPCAP_INC=%NPCAP_DIR%\Include
    set NPCAP_LIB=%NPCAP_DIR%\Lib\x64
    echo ✅ Found Npcap SDK at %NPCAP_DIR%
) else if exist "%NPCAP_SDK_DIR%\Include\pcap.h" (
    set NPCAP_FOUND=1
    set NPCAP_INC=%NPCAP_SDK_DIR%\Include
    set NPCAP_LIB=%NPCAP_SDK_DIR%\Lib\x64
    echo ✅ Found Npcap SDK at %NPCAP_SDK_DIR%
) else if exist "C:\npcap-sdk\Include\pcap.h" (
    set NPCAP_FOUND=1
    set NPCAP_INC=C:\npcap-sdk\Include
    set NPCAP_LIB=C:\npcap-sdk\Lib\x64
    echo ✅ Found Npcap SDK at C:\npcap-sdk
)

if %NPCAP_FOUND%==1 (
    echo Building with Npcap SDK support...
    
    REM Try MinGW GCC with Npcap
    echo.
    echo Trying MinGW GCC with Npcap...
    gcc -DNPCAP_AVAILABLE -I"%NPCAP_INC%" -L"%NPCAP_LIB%" -o raw_ethernet_avdecc_entity.exe raw_ethernet_avdecc_entity.c -lwpcap -lws2_32 2>nul
    if %errorlevel% == 0 (
        echo ✅ Successfully built with MinGW GCC + Npcap SDK
        goto :success_npcap
    )
    
    REM Try Visual Studio with Npcap
    echo MinGW GCC not found, trying Visual Studio with Npcap...
    cl /DNPCAP_AVAILABLE /I"%NPCAP_INC%" /Fe:raw_ethernet_avdecc_entity.exe raw_ethernet_avdecc_entity.c /link /LIBPATH:"%NPCAP_LIB%" wpcap.lib ws2_32.lib 2>nul
    if %errorlevel% == 0 (
        echo ✅ Successfully built with Visual Studio + Npcap SDK
        goto :success_npcap
    )
)

REM Fallback to simulation build
echo.
echo Npcap SDK not found, building simulation version...

REM Try different compilers
echo.
echo Trying MinGW GCC...
gcc -o raw_ethernet_avdecc_entity.exe raw_ethernet_avdecc_entity.c -lws2_32 2>nul
if %errorlevel% == 0 (
    echo ✅ Successfully built with MinGW GCC (simulation mode)
    goto :success_sim
)

echo MinGW GCC not found, trying Visual Studio...
cl /Fe:raw_ethernet_avdecc_entity.exe raw_ethernet_avdecc_entity.c ws2_32.lib 2>nul
if %errorlevel% == 0 (
    echo ✅ Successfully built with Visual Studio (simulation mode)
    goto :success_sim
)

echo.
echo ❌ No suitable compiler found!
echo.
echo To build this Raw Ethernet AVDECC Entity, you need:
echo   1. MinGW-w64 with GCC, OR
echo   2. Visual Studio with C++ Build Tools
echo.
echo For PRODUCTION use with Hive-AVDECC compatibility:
echo   3. Install Npcap SDK from: https://npcap.com/dist/npcap-sdk-1.13.zip
echo   4. Set NPCAP_DIR or NPCAP_SDK_DIR environment variable
echo   5. Run this script again for production build
echo.
pause
exit /b 1

:success_npcap
echo.
echo 🎯 SUCCESS: Production build with Npcap SDK
echo    Professional AVDECC tools compatibility: ENABLED
echo.
echo 📋 To test with Hive-AVDECC:
echo    1. Install Npcap runtime if not already installed
echo    2. Run as Administrator: raw_ethernet_avdecc_entity.exe
echo    3. Ensure Hive uses same network interface
echo.
echo 🚀 Run: raw_ethernet_avdecc_entity.exe --duration 60
echo.
pause
exit /b 0

:success_sim
echo.
echo 🎯 SIMULATION build completed
echo    Professional AVDECC tools require Npcap SDK for Raw Ethernet
echo.
echo 📋 To enable Hive-AVDECC compatibility:
echo    1. Download Npcap SDK: https://npcap.com/dist/npcap-sdk-1.13.zip
echo    2. Extract to C:\npcap-sdk or set NPCAP_DIR environment variable
echo    3. Run this script again for production build
echo.
echo 🚀 Run simulation: raw_ethernet_avdecc_entity.exe
echo.
pause
