@echo off
echo Building Hive-Compatible AVDECC Entity with Npcap Support...

REM Check if we're in the examples directory
if not exist "hive_compatible_avdecc_entity.c" (
    echo Error: hive_compatible_avdecc_entity.c not found
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
    echo Building Hive-Compatible AVDECC Entity with Npcap SDK support...
    
    REM Try MinGW GCC with Npcap
    echo.
    echo Trying MinGW GCC with Npcap...
    gcc -DNPCAP_AVAILABLE -I"%NPCAP_INC%" -L"%NPCAP_LIB%" -o hive_compatible_avdecc_entity.exe hive_compatible_avdecc_entity.c -lwpcap -lws2_32 2>nul
    if %errorlevel% == 0 (
        echo ✅ Successfully built Hive-Compatible AVDECC Entity with MinGW GCC + Npcap SDK
        goto :success_npcap
    )
    
    REM Try Visual Studio with Npcap
    echo MinGW GCC not found, trying Visual Studio with Npcap...
    cl /DNPCAP_AVAILABLE /I"%NPCAP_INC%" /Fe:hive_compatible_avdecc_entity.exe hive_compatible_avdecc_entity.c /link /LIBPATH:"%NPCAP_LIB%" wpcap.lib ws2_32.lib 2>nul
    if %errorlevel% == 0 (
        echo ✅ Successfully built Hive-Compatible AVDECC Entity with Visual Studio + Npcap SDK
        goto :success_npcap
    )
)

echo ❌ Npcap SDK not found or build failed
echo.
echo 📥 To install Npcap SDK:
echo    1. Download from: https://npcap.com/dist/npcap-sdk-1.13.zip
echo    2. Extract to C:\npcap-sdk or set NPCAP_SDK_DIR environment variable
echo    3. Run this script again
echo.
pause
exit /b 1

:success_npcap
echo.
echo 🎉 SUCCESS! Hive-Compatible AVDECC Entity built successfully!
echo.
echo 📋 Build Information:
echo    Executable: hive_compatible_avdecc_entity.exe
echo    PCAP Support: Npcap SDK
echo    Target Interface: Intel I219-LM
echo    Protocol: IEEE 1722.1 AVDECC with enhanced Hive compatibility
echo.
echo 🚀 To run the entity:
echo    1. Ensure Npcap is installed (not just SDK)
echo    2. Run as Administrator (required for raw packet access)
echo    3. Execute: hive_compatible_avdecc_entity.exe
echo.
echo 🔍 For Hive-AVDECC testing:
echo    1. Start the entity
echo    2. Open Hive-AVDECC
echo    3. Click 'Refresh' or 'Discover Entities'
echo    4. Look for 'OpenAvnu Audio Interface'
echo.
pause
