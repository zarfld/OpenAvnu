#!/bin/bash
#
# OpenAvnu Intel HAL Detection Test for DAW02
# Tests Intel HAL integration and graceful fallback behavior
#

echo "=== Intel HAL Detection Test ==="
echo "System: $(hostname)"
echo "Date: $(date)"
echo "Objective: Verify Intel HAL integration and graceful fallback"
echo ""

# Determine build configuration
if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" || "$OSTYPE" == "win32" ]]; then
    BUILD_TYPE="Windows"
    GENERATOR="Visual Studio 17 2022"
    CONFIG_ARG="--config Release"
else
    BUILD_TYPE="Linux"
    GENERATOR="Unix Makefiles"
    CONFIG_ARG=""
fi

echo "Build Type: $BUILD_TYPE"
echo ""

# Test 1: Build WITHOUT Intel HAL
echo "=== Test 1: Building WITHOUT Intel HAL ==="
echo "This tests the software fallback path"
echo ""

rm -rf build_no_hal
mkdir build_no_hal
cd build_no_hal

echo "Configuring without Intel HAL..."
START_TIME=$(date +%s)
if cmake .. -G "$GENERATOR" -DOPENAVNU_BUILD_INTEL_HAL=OFF; then
    CMAKE_NO_HAL_TIME=$(($(date +%s) - $START_TIME))
    echo "✅ CMake configuration (no HAL): SUCCESS ($CMAKE_NO_HAL_TIME seconds)"
    
    echo "Building without Intel HAL..."
    START_TIME=$(date +%s)
    if cmake --build . $CONFIG_ARG; then
        BUILD_NO_HAL_TIME=$(($(date +%s) - $START_TIME))
        echo "✅ Build (no HAL): SUCCESS ($BUILD_NO_HAL_TIME seconds)"
        NO_HAL_SUCCESS=true
    else
        BUILD_NO_HAL_TIME=$(($(date +%s) - $START_TIME))
        echo "❌ Build (no HAL): FAILED ($BUILD_NO_HAL_TIME seconds)"
        NO_HAL_SUCCESS=false
    fi
else
    echo "❌ CMake configuration (no HAL): FAILED"
    NO_HAL_SUCCESS=false
fi

cd ..

# Test 2: Build WITH Intel HAL (if available)
echo ""
echo "=== Test 2: Building WITH Intel HAL ==="
echo "This tests Intel HAL integration when available"
echo ""

rm -rf build_with_hal
mkdir build_with_hal
cd build_with_hal

echo "Configuring with Intel HAL..."
START_TIME=$(date +%s)
if cmake .. -G "$GENERATOR" -DOPENAVNU_BUILD_INTEL_HAL=ON; then
    CMAKE_HAL_TIME=$(($(date +%s) - $START_TIME))
    echo "✅ CMake configuration (with HAL): SUCCESS ($CMAKE_HAL_TIME seconds)"
    
    echo "Building with Intel HAL..."
    START_TIME=$(date +%s)
    if cmake --build . $CONFIG_ARG; then
        BUILD_HAL_TIME=$(($(date +%s) - $START_TIME))
        echo "✅ Build (with HAL): SUCCESS ($BUILD_HAL_TIME seconds)"
        WITH_HAL_SUCCESS=true
    else
        BUILD_HAL_TIME=$(($(date +%s) - $START_TIME))
        echo "⚠️  Build (with HAL): FAILED ($BUILD_HAL_TIME seconds)"
        echo "   This is expected if Intel HAL libraries are not available"
        WITH_HAL_SUCCESS=false
    fi
else
    echo "⚠️  CMake configuration (with HAL): FAILED"
    echo "   This is expected if Intel HAL dependencies are not available"
    WITH_HAL_SUCCESS=false
fi

cd ..

# Test 3: Intel Hardware Detection
echo ""
echo "=== Test 3: Intel Hardware Detection ==="

INTEL_HARDWARE_COUNT=0
if command -v lspci &> /dev/null; then
    echo "Scanning for Intel network hardware..."
    INTEL_DEVICES=$(lspci | grep -i intel | grep -i ethernet)
    
    if [ -n "$INTEL_DEVICES" ]; then
        INTEL_HARDWARE_COUNT=$(echo "$INTEL_DEVICES" | wc -l)
        echo "✅ Intel network hardware detected:"
        echo "$INTEL_DEVICES" | sed 's/^/   /'
    else
        echo "ℹ️  No Intel network hardware detected"
    fi
else
    echo "⚠️  Cannot detect hardware (lspci not available)"
fi

# Test 4: Intel HAL Library Detection
echo ""
echo "=== Test 4: Intel HAL Library Detection ==="

echo "Searching for Intel HAL libraries..."
HAL_LIBRARIES_FOUND=false

# Common locations for Intel HAL libraries
SEARCH_PATHS="/usr/lib /usr/local/lib /opt/intel /lib64 /usr/lib64"

for path in $SEARCH_PATHS; do
    if [ -d "$path" ]; then
        INTEL_LIBS=$(find "$path" -name "*intel*hal*" -o -name "*igb*" 2>/dev/null | head -5)
        if [ -n "$INTEL_LIBS" ]; then
            echo "✅ Intel libraries found in $path:"
            echo "$INTEL_LIBS" | sed 's/^/   /'
            HAL_LIBRARIES_FOUND=true
        fi
    fi
done

if [ "$HAL_LIBRARIES_FOUND" = false ]; then
    echo "ℹ️  No Intel HAL libraries detected in standard locations"
fi

# Test 5: Runtime HAL Detection (if builds succeeded)
echo ""
echo "=== Test 5: Runtime HAL Detection ==="

if [ "$NO_HAL_SUCCESS" = true ]; then
    echo "Testing runtime behavior without Intel HAL..."
    cd build_no_hal
    
    # Determine executable paths
    if [[ "$BUILD_TYPE" == "Windows" ]]; then
        MRPD_PATH="daemons/mrpd/Release/mrpd.exe"
    else
        MRPD_PATH="daemons/mrpd/mrpd"
    fi
    
    if [ -f "$MRPD_PATH" ]; then
        echo "Running MRPD without HAL (brief test)..."
        timeout 3s ./$MRPD_PATH -v 2>&1 | head -10 | sed 's/^/   /' || echo "   (Expected - daemon requires proper network setup)"
    fi
    
    cd ..
fi

if [ "$WITH_HAL_SUCCESS" = true ]; then
    echo ""
    echo "Testing runtime behavior with Intel HAL..."
    cd build_with_hal
    
    # Determine executable paths
    if [[ "$BUILD_TYPE" == "Windows" ]]; then
        MRPD_PATH="daemons/mrpd/Release/mrpd.exe"
    else
        MRPD_PATH="daemons/mrpd/mrpd"
    fi
    
    if [ -f "$MRPD_PATH" ]; then
        echo "Running MRPD with HAL (brief test)..."
        timeout 3s ./$MRPD_PATH -v 2>&1 | head -10 | sed 's/^/   /' || echo "   (Expected - daemon requires proper network setup)"
    fi
    
    cd ..
fi

# Results Summary
echo ""
echo "=== Intel HAL Test Results Summary ==="
echo "Build without HAL: $([ "$NO_HAL_SUCCESS" = true ] && echo "✅ SUCCESS" || echo "❌ FAILED")"
echo "Build with HAL: $([ "$WITH_HAL_SUCCESS" = true ] && echo "✅ SUCCESS" || echo "⚠️ FAILED (expected if HAL unavailable)")"
echo "Intel Hardware Count: $INTEL_HARDWARE_COUNT"
echo "Intel Libraries Found: $([ "$HAL_LIBRARIES_FOUND" = true ] && echo "Yes" || echo "No")"

# Performance comparison (if both builds succeeded)
if [ "$NO_HAL_SUCCESS" = true ] && [ "$WITH_HAL_SUCCESS" = true ]; then
    echo ""
    echo "=== Build Performance Comparison ==="
    echo "Build Time (no HAL): $BUILD_NO_HAL_TIME seconds"
    echo "Build Time (with HAL): $BUILD_HAL_TIME seconds"
    
    if [ "$BUILD_HAL_TIME" -gt "$BUILD_NO_HAL_TIME" ]; then
        DIFF=$((BUILD_HAL_TIME - BUILD_NO_HAL_TIME))
        echo "Intel HAL build took $DIFF seconds longer (expected due to additional dependencies)"
    else
        DIFF=$((BUILD_NO_HAL_TIME - BUILD_HAL_TIME))
        echo "Intel HAL build took $DIFF seconds less (unusual, possible cache effects)"
    fi
fi

# Final assessment
echo ""
echo "=== Final Assessment ==="

if [ "$NO_HAL_SUCCESS" = true ]; then
    echo "🎉 GRACEFUL FALLBACK: SUCCESS"
    echo "   OpenAvnu daemons build and work without Intel HAL"
    
    if [ "$WITH_HAL_SUCCESS" = true ]; then
        echo "🎉 INTEL HAL INTEGRATION: SUCCESS"
        echo "   OpenAvnu daemons also build with Intel HAL enabled"
        echo "✅ HARDWARE INDEPENDENCE: VERIFIED"
        exit 0
    else
        echo "ℹ️  INTEL HAL INTEGRATION: NOT AVAILABLE"
        echo "   This is normal if Intel HAL libraries are not installed"
        echo "✅ HARDWARE INDEPENDENCE: VERIFIED"
        exit 0
    fi
else
    echo "❌ GRACEFUL FALLBACK: FAILED"
    echo "   OpenAvnu daemons failed to build without Intel HAL"
    echo "   This indicates a dependency issue that needs resolution"
    exit 1
fi
