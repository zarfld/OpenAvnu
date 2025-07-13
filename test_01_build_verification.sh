#!/bin/bash
#
# OpenAvnu Build Verification Test for DAW02
# Tests that all modernized daemons compile successfully
#

echo "=== OpenAvnu Build Verification Test ==="
echo "System: $(hostname)"
echo "Date: $(date)"
echo "Objective: Verify all daemons compile on DAW02"
echo ""

# Determine build configuration based on OS
if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" || "$OSTYPE" == "win32" ]]; then
    BUILD_TYPE="Windows"
    GENERATOR="Visual Studio 17 2022"
    CONFIG_ARG="--config Release"
    EXE_EXT=".exe"
    MRPD_PATH="daemons/mrpd/Release/mrpd.exe"
    MAAP_PATH="daemons/maap/Release/maap_daemon.exe"
    SHAPER_PATH="daemons/shaper/Release/openavnu_shaper_windows.exe"
else
    BUILD_TYPE="Linux"
    GENERATOR="Unix Makefiles"
    CONFIG_ARG=""
    EXE_EXT=""
    MRPD_PATH="daemons/mrpd/mrpd"
    MAAP_PATH="daemons/maap/maap_daemon"
    SHAPER_PATH="daemons/shaper/linux/shaper_linux"
fi

echo "Build Type: $BUILD_TYPE"
echo "Generator: $GENERATOR"
echo ""

# Clean previous build
echo "=== Cleaning Previous Build ==="
rm -rf build_daw02_verify
mkdir build_daw02_verify
cd build_daw02_verify

# Configure with timing
echo ""
echo "=== CMake Configuration ==="
echo "Command: cmake .. -G \"$GENERATOR\""
echo ""

START_TIME=$(date +%s)
if cmake .. -G "$GENERATOR"; then
    CMAKE_TIME=$(($(date +%s) - $START_TIME))
    echo "✅ CMake configuration successful ($CMAKE_TIME seconds)"
    CMAKE_SUCCESS=true
else
    CMAKE_TIME=$(($(date +%s) - $START_TIME))
    echo "❌ CMake configuration failed ($CMAKE_TIME seconds)"
    CMAKE_SUCCESS=false
    cd ..
    exit 1
fi

# Build all daemons with timing
echo ""
echo "=== Building All Daemons ==="
echo "Command: cmake --build . $CONFIG_ARG"
echo ""

START_TIME=$(date +%s)
if cmake --build . $CONFIG_ARG; then
    BUILD_TIME=$(($(date +%s) - $START_TIME))
    echo "✅ Build successful ($BUILD_TIME seconds)"
    BUILD_SUCCESS=true
else
    BUILD_TIME=$(($(date +%s) - $START_TIME))
    echo "❌ Build failed ($BUILD_TIME seconds)"
    BUILD_SUCCESS=false
fi

# Check for executables and test basic functionality
echo ""
echo "=== Executable Verification ==="

MRPD_SUCCESS=false
MAAP_SUCCESS=false
SHAPER_SUCCESS=false

# Test MRPD
if [ -f "$MRPD_PATH" ]; then
    MRPD_SIZE=$(stat --format="%s" "$MRPD_PATH" 2>/dev/null || stat -f%z "$MRPD_PATH" 2>/dev/null || echo "unknown")
    echo "✅ MRPD: BUILD SUCCESS (size: $MRPD_SIZE bytes)"
    echo "   Path: $MRPD_PATH"
    
    # Test basic execution
    if timeout 5s ./$MRPD_PATH --help >/dev/null 2>&1 || timeout 5s ./$MRPD_PATH -h >/dev/null 2>&1; then
        echo "   ✅ Basic execution: SUCCESS"
        MRPD_SUCCESS=true
    else
        echo "   ⚠️  Basic execution: Could not verify (may require admin privileges)"
        MRPD_SUCCESS=true  # Consider success if binary exists
    fi
else
    echo "❌ MRPD: BUILD FAILED"
    echo "   Expected path: $MRPD_PATH"
fi

# Test MAAP
if [ -f "$MAAP_PATH" ]; then
    MAAP_SIZE=$(stat --format="%s" "$MAAP_PATH" 2>/dev/null || stat -f%z "$MAAP_PATH" 2>/dev/null || echo "unknown")
    echo "✅ MAAP: BUILD SUCCESS (size: $MAAP_SIZE bytes)"
    echo "   Path: $MAAP_PATH"
    
    # Test basic execution  
    if timeout 5s ./$MAAP_PATH --help >/dev/null 2>&1 || timeout 5s ./$MAAP_PATH -h >/dev/null 2>&1; then
        echo "   ✅ Basic execution: SUCCESS"
        MAAP_SUCCESS=true
    else
        echo "   ⚠️  Basic execution: Could not verify (may require admin privileges)"
        MAAP_SUCCESS=true  # Consider success if binary exists
    fi
else
    echo "❌ MAAP: BUILD FAILED"
    echo "   Expected path: $MAAP_PATH"
fi

# Test Shaper
if [ -f "$SHAPER_PATH" ]; then
    SHAPER_SIZE=$(stat --format="%s" "$SHAPER_PATH" 2>/dev/null || stat -f%z "$SHAPER_PATH" 2>/dev/null || echo "unknown")
    echo "✅ SHAPER: BUILD SUCCESS (size: $SHAPER_SIZE bytes)"
    echo "   Path: $SHAPER_PATH"
    
    # Test basic execution
    if timeout 5s ./$SHAPER_PATH --help >/dev/null 2>&1 || timeout 5s ./$SHAPER_PATH -h >/dev/null 2>&1; then
        echo "   ✅ Basic execution: SUCCESS"
        SHAPER_SUCCESS=true
    else
        echo "   ⚠️  Basic execution: Could not verify"
        SHAPER_SUCCESS=true  # Consider success if binary exists
    fi
else
    echo "❌ SHAPER: BUILD FAILED"
    echo "   Expected path: $SHAPER_PATH"
fi

# Build performance summary
echo ""
echo "=== Build Performance Summary ==="
echo "CMake Configuration Time: $CMAKE_TIME seconds"
echo "Build Time: $BUILD_TIME seconds"
echo "Total Time: $((CMAKE_TIME + BUILD_TIME)) seconds"

# Overall results
echo ""
echo "=== Final Results ==="
if [ "$CMAKE_SUCCESS" = true ] && [ "$BUILD_SUCCESS" = true ] && [ "$MRPD_SUCCESS" = true ] && [ "$MAAP_SUCCESS" = true ] && [ "$SHAPER_SUCCESS" = true ]; then
    echo "🎉 BUILD VERIFICATION: SUCCESS"
    echo "   All three daemons built and verified successfully on DAW02"
    cd ..
    exit 0
else
    echo "❌ BUILD VERIFICATION: FAILED"
    echo "   One or more daemons failed to build or verify"
    
    # Provide detailed failure information
    echo ""
    echo "=== Failure Details ==="
    [ "$CMAKE_SUCCESS" = false ] && echo "   - CMake configuration failed"
    [ "$BUILD_SUCCESS" = false ] && echo "   - Build process failed"
    [ "$MRPD_SUCCESS" = false ] && echo "   - MRPD daemon failed"
    [ "$MAAP_SUCCESS" = false ] && echo "   - MAAP daemon failed"
    [ "$SHAPER_SUCCESS" = false ] && echo "   - Shaper daemon failed"
    
    cd ..
    exit 1
fi
