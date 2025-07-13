# OpenAvnu Cross-Platform Testing Plan for DAW02

**Target System:** DAW02  
**Date:** July 13, 2025  
**Source:** Build verification complete on development machine  
**Objective:** Validate hardware-independent daemon modernization across platforms

## Pre-Testing System Information Collection

### System Environment Report
Create this script to capture DAW02 system information:

```bash
# daw02_system_report.sh (Linux) or daw02_system_report.ps1 (Windows)

echo "=== OpenAvnu DAW02 System Report ===" > daw02_system_report.txt
echo "Date: $(date)" >> daw02_system_report.txt
echo "Hostname: $(hostname)" >> daw02_system_report.txt
echo "OS: $(uname -a)" >> daw02_system_report.txt  # Linux
# Or: Get-ComputerInfo | Select-Object WindowsProductName, WindowsVersion >> daw02_system_report.txt  # Windows

echo "" >> daw02_system_report.txt
echo "=== Hardware Information ===" >> daw02_system_report.txt
lscpu >> daw02_system_report.txt  # Linux
# Or: Get-WmiObject -Class Win32_Processor >> daw02_system_report.txt  # Windows

echo "" >> daw02_system_report.txt
echo "=== Network Interfaces ===" >> daw02_system_report.txt
ip addr show >> daw02_system_report.txt  # Linux
# Or: Get-NetAdapter >> daw02_system_report.txt  # Windows

echo "" >> daw02_system_report.txt
echo "=== Intel Hardware Detection ===" >> daw02_system_report.txt
lspci | grep -i intel >> daw02_system_report.txt  # Linux
```

## Phase 1: Build Verification Tests

### 1.1 Clean Build Test
**Objective:** Verify all daemons compile on DAW02
**Duration:** 15-20 minutes

```bash
# Test script: test_01_build_verification.sh
#!/bin/bash

TEST_LOG="test_01_build_verification_$(date +%Y%m%d_%H%M%S).log"
echo "=== OpenAvnu Build Verification Test ===" | tee $TEST_LOG
echo "System: $(hostname)" | tee -a $TEST_LOG
echo "Date: $(date)" | tee -a $TEST_LOG

# Clean build directory
rm -rf build_daw02
mkdir build_daw02
cd build_daw02

# Configure with timing
echo "" | tee -a ../$TEST_LOG
echo "=== CMake Configuration ===" | tee -a ../$TEST_LOG
time cmake .. 2>&1 | tee -a ../$TEST_LOG
CMAKE_EXIT_CODE=$?

# Build all daemons with timing
echo "" | tee -a ../$TEST_LOG
echo "=== Building All Daemons ===" | tee -a ../$TEST_LOG
time cmake --build . --config Release 2>&1 | tee -a ../$TEST_LOG
BUILD_EXIT_CODE=$?

# Generate build report
echo "" | tee -a ../$TEST_LOG
echo "=== Build Results Summary ===" | tee -a ../$TEST_LOG
echo "CMake Exit Code: $CMAKE_EXIT_CODE" | tee -a ../$TEST_LOG
echo "Build Exit Code: $BUILD_EXIT_CODE" | tee -a ../$TEST_LOG

# Check for executables
if [ -f "daemons/mrpd/mrpd" ] || [ -f "daemons/mrpd/Release/mrpd.exe" ]; then
    echo "✅ MRPD: BUILD SUCCESS" | tee -a ../$TEST_LOG
else
    echo "❌ MRPD: BUILD FAILED" | tee -a ../$TEST_LOG
fi

if [ -f "daemons/maap/maap_daemon" ] || [ -f "daemons/maap/Release/maap_daemon.exe" ]; then
    echo "✅ MAAP: BUILD SUCCESS" | tee -a ../$TEST_LOG
else
    echo "❌ MAAP: BUILD FAILED" | tee -a ../$TEST_LOG
fi

# Linux shaper check
if [ -f "daemons/shaper/linux/shaper_linux" ]; then
    echo "✅ Linux Shaper: BUILD SUCCESS" | tee -a ../$TEST_LOG
elif [ -f "daemons/shaper/Release/openavnu_shaper_windows.exe" ]; then
    echo "✅ Windows Shaper: BUILD SUCCESS" | tee -a ../$TEST_LOG
else
    echo "❌ Shaper: BUILD FAILED" | tee -a ../$TEST_LOG
fi

cd ..
echo "Build verification complete. Report: $TEST_LOG"
```

### 1.2 Intel HAL Detection Test
**Objective:** Verify Intel HAL graceful fallback behavior

```bash
# Test script: test_02_intel_hal_detection.sh
#!/bin/bash

TEST_LOG="test_02_intel_hal_detection_$(date +%Y%m%d_%H%M%S).log"
echo "=== Intel HAL Detection Test ===" | tee $TEST_LOG

# Test without Intel HAL
echo "" | tee -a $TEST_LOG
echo "=== Building WITHOUT Intel HAL ===" | tee -a $TEST_LOG
rm -rf build_no_hal
mkdir build_no_hal
cd build_no_hal
cmake .. -DOPENAVNU_BUILD_INTEL_HAL=OFF 2>&1 | tee -a ../$TEST_LOG
cmake --build . --config Release 2>&1 | tee -a ../$TEST_LOG
cd ..

# Test with Intel HAL (if available)
echo "" | tee -a $TEST_LOG
echo "=== Building WITH Intel HAL ===" | tee -a $TEST_LOG
rm -rf build_with_hal
mkdir build_with_hal
cd build_with_hal
cmake .. -DOPENAVNU_BUILD_INTEL_HAL=ON 2>&1 | tee -a ../$TEST_LOG
cmake --build . --config Release 2>&1 | tee -a ../$TEST_LOG
cd ..

echo "Intel HAL detection test complete. Report: $TEST_LOG"
```

## Phase 2: Runtime Functionality Tests

### 2.1 Basic Executable Tests
**Objective:** Verify daemons start and show proper help/version info

```bash
# Test script: test_03_runtime_basic.sh
#!/bin/bash

TEST_LOG="test_03_runtime_basic_$(date +%Y%m%d_%H%M%S).log"
echo "=== Basic Runtime Test ===" | tee $TEST_LOG

cd build_daw02

# Test MRPD
echo "" | tee -a ../$TEST_LOG
echo "=== MRPD Runtime Test ===" | tee -a ../$TEST_LOG
if [ -f "daemons/mrpd/mrpd" ]; then
    timeout 5s ./daemons/mrpd/mrpd --help 2>&1 | tee -a ../$TEST_LOG
elif [ -f "daemons/mrpd/Release/mrpd.exe" ]; then
    timeout 5s ./daemons/mrpd/Release/mrpd.exe --help 2>&1 | tee -a ../$TEST_LOG
fi

# Test MAAP
echo "" | tee -a ../$TEST_LOG
echo "=== MAAP Runtime Test ===" | tee -a ../$TEST_LOG
if [ -f "daemons/maap/maap_daemon" ]; then
    timeout 5s ./daemons/maap/maap_daemon --help 2>&1 | tee -a ../$TEST_LOG
elif [ -f "daemons/maap/Release/maap_daemon.exe" ]; then
    timeout 5s ./daemons/maap/Release/maap_daemon.exe --help 2>&1 | tee -a ../$TEST_LOG
fi

# Test Shaper
echo "" | tee -a ../$TEST_LOG
echo "=== Shaper Runtime Test ===" | tee -a ../$TEST_LOG
if [ -f "daemons/shaper/linux/shaper_linux" ]; then
    timeout 5s ./daemons/shaper/linux/shaper_linux --help 2>&1 | tee -a ../$TEST_LOG
elif [ -f "daemons/shaper/Release/openavnu_shaper_windows.exe" ]; then
    timeout 5s ./daemons/shaper/Release/openavnu_shaper_windows.exe --help 2>&1 | tee -a ../$TEST_LOG
fi

cd ..
echo "Basic runtime test complete. Report: $TEST_LOG"
```

### 2.2 Network Interface Detection Test
**Objective:** Verify daemons can detect network interfaces

```bash
# Test script: test_04_network_detection.sh
#!/bin/bash

TEST_LOG="test_04_network_detection_$(date +%Y%m%d_%H%M%S).log"
echo "=== Network Interface Detection Test ===" | tee $TEST_LOG

echo "" | tee -a $TEST_LOG
echo "=== Available Network Interfaces ===" | tee -a $TEST_LOG
ip addr show | grep -E "^[0-9]+:" | tee -a $TEST_LOG

echo "" | tee -a $TEST_LOG
echo "=== Testing MRPD Interface Detection ===" | tee -a $TEST_LOG
cd build_daw02
# Test with first available interface
FIRST_INTERFACE=$(ip addr show | grep -E "^[0-9]+:" | head -1 | awk '{print $2}' | tr -d ':')
timeout 10s ./daemons/mrpd/mrpd -i $FIRST_INTERFACE -v 2>&1 | tee -a ../$TEST_LOG

cd ..
echo "Network detection test complete. Report: $TEST_LOG"
```

## Phase 3: Hardware Compatibility Tests

### 3.1 Intel Hardware Detection
**Objective:** Test Intel HAL integration on real Intel hardware

```bash
# Test script: test_05_intel_hardware.sh
#!/bin/bash

TEST_LOG="test_05_intel_hardware_$(date +%Y%m%d_%H%M%S).log"
echo "=== Intel Hardware Compatibility Test ===" | tee $TEST_LOG

echo "" | tee -a $TEST_LOG
echo "=== Intel Network Hardware Detection ===" | tee -a $TEST_LOG
lspci | grep -i intel | grep -i ethernet | tee -a $TEST_LOG

echo "" | tee -a $TEST_LOG
echo "=== Intel HAL Library Detection ===" | tee -a $TEST_LOG
find /usr/lib* /opt -name "*intel*hal*" -o -name "*igb*" 2>/dev/null | tee -a $TEST_LOG

# Test Intel HAL enabled build if hardware is present
INTEL_COUNT=$(lspci | grep -i intel | grep -i ethernet | wc -l)
if [ $INTEL_COUNT -gt 0 ]; then
    echo "" | tee -a $TEST_LOG
    echo "=== Intel Hardware Found - Testing HAL Integration ===" | tee -a $TEST_LOG
    
    cd build_with_hal
    
    # Test MRPD with Intel HAL
    echo "Testing MRPD with Intel HAL..." | tee -a ../$TEST_LOG
    timeout 15s ./daemons/mrpd/mrpd -i eth0 -v 2>&1 | tee -a ../$TEST_LOG
    
    cd ..
else
    echo "No Intel network hardware detected" | tee -a $TEST_LOG
fi

echo "Intel hardware test complete. Report: $TEST_LOG"
```

## Phase 4: Performance and Stress Tests

### 4.1 Memory Leak Detection
**Objective:** Verify no memory leaks in daemon operations

```bash
# Test script: test_06_memory_leak.sh
#!/bin/bash

TEST_LOG="test_06_memory_leak_$(date +%Y%m%d_%H%M%S).log"
echo "=== Memory Leak Detection Test ===" | tee $TEST_LOG

if command -v valgrind &> /dev/null; then
    echo "" | tee -a $TEST_LOG
    echo "=== Valgrind Memory Check - MRPD ===" | tee -a $TEST_LOG
    cd build_daw02
    valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes \
             --log-file=../mrpd_valgrind.log \
             timeout 30s ./daemons/mrpd/mrpd --help
    
    echo "Valgrind results:" | tee -a ../$TEST_LOG
    cat ../mrpd_valgrind.log | tail -10 | tee -a ../$TEST_LOG
    cd ..
else
    echo "Valgrind not available - skipping memory leak test" | tee -a $TEST_LOG
fi

echo "Memory leak test complete. Report: $TEST_LOG"
```

## Phase 5: Integration Tests

### 5.1 Multi-Daemon Coordination Test
**Objective:** Test daemons working together

```bash
# Test script: test_07_integration.sh
#!/bin/bash

TEST_LOG="test_07_integration_$(date +%Y%m%d_%H%M%S).log"
echo "=== Multi-Daemon Integration Test ===" | tee $TEST_LOG

cd build_daw02

# Start MRPD in background
echo "" | tee -a ../$TEST_LOG
echo "=== Starting MRPD ===" | tee -a ../$TEST_LOG
./daemons/mrpd/mrpd -i eth0 &
MRPD_PID=$!
sleep 2

# Start MAAP in background
echo "" | tee -a ../$TEST_LOG
echo "=== Starting MAAP ===" | tee -a ../$TEST_LOG
./daemons/maap/maap_daemon &
MAAP_PID=$!
sleep 2

# Check if both are running
echo "" | tee -a ../$TEST_LOG
echo "=== Process Status ===" | tee -a ../$TEST_LOG
ps aux | grep -E "(mrpd|maap)" | grep -v grep | tee -a ../$TEST_LOG

# Let them run for a few seconds
sleep 5

# Clean shutdown
echo "" | tee -a ../$TEST_LOG
echo "=== Shutting Down Daemons ===" | tee -a ../$TEST_LOG
kill $MRPD_PID $MAAP_PID 2>/dev/null
sleep 2

cd ..
echo "Integration test complete. Report: $TEST_LOG"
```

## Report Generation and Analysis

### Automated Report Compilation
Create a master script that runs all tests and generates a comprehensive report:

```bash
# master_test_runner.sh
#!/bin/bash

MASTER_REPORT="OpenAvnu_DAW02_Test_Report_$(date +%Y%m%d_%H%M%S).md"

echo "# OpenAvnu DAW02 Testing Report" > $MASTER_REPORT
echo "" >> $MASTER_REPORT
echo "**System:** $(hostname)" >> $MASTER_REPORT
echo "**Date:** $(date)" >> $MASTER_REPORT
echo "**Tester:** [Your Name]" >> $MASTER_REPORT
echo "" >> $MASTER_REPORT

# Run all tests
echo "Running system information collection..."
./daw02_system_report.sh

echo "Running build verification..."
./test_01_build_verification.sh

echo "Running Intel HAL detection..."
./test_02_intel_hal_detection.sh

echo "Running basic runtime tests..."
./test_03_runtime_basic.sh

echo "Running network detection tests..."
./test_04_network_detection.sh

echo "Running Intel hardware tests..."
./test_05_intel_hardware.sh

echo "Running memory leak tests..."
./test_06_memory_leak.sh

echo "Running integration tests..."
./test_07_integration.sh

# Compile results into master report
echo "## Test Results Summary" >> $MASTER_REPORT
echo "" >> $MASTER_REPORT

for log in test_*.log; do
    echo "### $(basename $log .log)" >> $MASTER_REPORT
    echo '```' >> $MASTER_REPORT
    tail -20 $log >> $MASTER_REPORT
    echo '```' >> $MASTER_REPORT
    echo "" >> $MASTER_REPORT
done

echo "Master test report generated: $MASTER_REPORT"
```

## Expected Deliverables from DAW02

### 1. System Configuration Report
- Hardware specifications
- OS version and configuration
- Network interface details
- Intel hardware presence/absence

### 2. Build Verification Report
- Compilation success/failure for each daemon
- Build time metrics
- Compiler warnings/errors
- Binary size comparisons

### 3. Runtime Functionality Report
- Daemon startup/shutdown behavior
- Help/version output verification
- Network interface detection results
- Error handling verification

### 4. Intel HAL Integration Report
- Hardware detection results
- HAL availability status
- Graceful fallback behavior
- Performance differences (if applicable)

### 5. Performance Metrics
- Memory usage patterns
- CPU utilization
- Network interface handling performance
- Memory leak detection results

## Analysis Framework

After collecting all reports from DAW02, we can analyze:

1. **Cross-Platform Compatibility:** Compare build success across Windows/Linux
2. **Hardware Independence:** Verify consistent behavior with/without Intel hardware
3. **Performance Baseline:** Establish performance metrics for future optimization
4. **Deployment Readiness:** Identify any platform-specific issues for production deployment

The reports will help us identify any platform-specific issues and validate that our hardware-independent modernization approach works correctly across different systems.
