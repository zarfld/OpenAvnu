#!/bin/bash
#
# OpenAvnu DAW02 Master Test Runner
# Comprehensive testing suite for cross-platform validation
#

set -e  # Exit on any error

# Configuration
TEST_SESSION="OpenAvnu_DAW02_$(date +%Y%m%d_%H%M%S)"
REPORT_DIR="test_results_$TEST_SESSION"
MASTER_REPORT="$REPORT_DIR/OpenAvnu_DAW02_Master_Report.md"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Create results directory
mkdir -p "$REPORT_DIR"

echo -e "${GREEN}🧪 OpenAvnu DAW02 Testing Suite Starting...${NC}"
echo "Test Session: $TEST_SESSION"
echo "Results Directory: $REPORT_DIR"
echo ""

# Initialize master report
cat > "$MASTER_REPORT" << EOF
# OpenAvnu DAW02 Comprehensive Testing Report

**Test Session:** $TEST_SESSION  
**System:** $(hostname)  
**Date:** $(date)  
**OS:** $(uname -a)  
**User:** $(whoami)  

## Executive Summary

This report contains comprehensive testing results for the OpenAvnu daemon modernization on DAW02.

EOF

# Function to run test and capture results
run_test() {
    local test_name="$1"
    local test_script="$2"
    local test_description="$3"
    
    echo -e "${BLUE}📋 Running: $test_name${NC}"
    echo "   $test_description"
    
    local log_file="$REPORT_DIR/${test_name}_$(date +%H%M%S).log"
    
    if [ -f "$test_script" ]; then
        echo "=== $test_name ===" > "$log_file"
        echo "Description: $test_description" >> "$log_file"
        echo "Started: $(date)" >> "$log_file"
        echo "" >> "$log_file"
        
        if bash "$test_script" >> "$log_file" 2>&1; then
            echo -e "   ${GREEN}✅ PASSED${NC}"
            echo "Completed: $(date)" >> "$log_file"
            
            # Add to master report
            echo "### ✅ $test_name - PASSED" >> "$MASTER_REPORT"
            echo "" >> "$MASTER_REPORT"
            echo "$test_description" >> "$MASTER_REPORT"
            echo "" >> "$MASTER_REPORT"
            echo '```' >> "$MASTER_REPORT"
            tail -20 "$log_file" >> "$MASTER_REPORT"
            echo '```' >> "$MASTER_REPORT"
            echo "" >> "$MASTER_REPORT"
        else
            echo -e "   ${RED}❌ FAILED${NC}"
            echo "Failed: $(date)" >> "$log_file"
            
            # Add to master report
            echo "### ❌ $test_name - FAILED" >> "$MASTER_REPORT"
            echo "" >> "$MASTER_REPORT"
            echo "$test_description" >> "$MASTER_REPORT"
            echo "" >> "$MASTER_REPORT"
            echo '```' >> "$MASTER_REPORT"
            tail -30 "$log_file" >> "$MASTER_REPORT"
            echo '```' >> "$MASTER_REPORT"
            echo "" >> "$MASTER_REPORT"
        fi
    else
        echo -e "   ${YELLOW}⚠️  SKIPPED (script not found)${NC}"
        echo "### ⚠️ $test_name - SKIPPED" >> "$MASTER_REPORT"
        echo "" >> "$MASTER_REPORT"
        echo "Test script '$test_script' not found." >> "$MASTER_REPORT"
        echo "" >> "$MASTER_REPORT"
    fi
    
    echo ""
}

# System Information Collection
echo -e "${YELLOW}📊 Collecting System Information...${NC}"
cat > "$REPORT_DIR/system_info.txt" << EOF
=== DAW02 System Information ===
Hostname: $(hostname)
Date: $(date)
Uptime: $(uptime)
OS: $(uname -a)
Kernel: $(uname -r)
Architecture: $(uname -m)

=== Hardware Information ===
EOF

if command -v lscpu &> /dev/null; then
    echo "CPU Information:" >> "$REPORT_DIR/system_info.txt"
    lscpu >> "$REPORT_DIR/system_info.txt"
    echo "" >> "$REPORT_DIR/system_info.txt"
fi

if command -v free &> /dev/null; then
    echo "Memory Information:" >> "$REPORT_DIR/system_info.txt"
    free -h >> "$REPORT_DIR/system_info.txt"
    echo "" >> "$REPORT_DIR/system_info.txt"
fi

if command -v lspci &> /dev/null; then
    echo "Intel Network Hardware:" >> "$REPORT_DIR/system_info.txt"
    lspci | grep -i intel | grep -i ethernet >> "$REPORT_DIR/system_info.txt" || echo "No Intel network hardware found" >> "$REPORT_DIR/system_info.txt"
    echo "" >> "$REPORT_DIR/system_info.txt"
fi

if command -v ip &> /dev/null; then
    echo "Network Interfaces:" >> "$REPORT_DIR/system_info.txt"
    ip addr show >> "$REPORT_DIR/system_info.txt"
    echo "" >> "$REPORT_DIR/system_info.txt"
fi

echo "Development Tools:" >> "$REPORT_DIR/system_info.txt"
echo "CMake: $(cmake --version 2>/dev/null | head -1 || echo 'Not found')" >> "$REPORT_DIR/system_info.txt"
echo "GCC: $(gcc --version 2>/dev/null | head -1 || echo 'Not found')" >> "$REPORT_DIR/system_info.txt"
echo "Clang: $(clang --version 2>/dev/null | head -1 || echo 'Not found')" >> "$REPORT_DIR/system_info.txt"
echo "Make: $(make --version 2>/dev/null | head -1 || echo 'Not found')" >> "$REPORT_DIR/system_info.txt"

# Add system info to master report
echo "## System Information" >> "$MASTER_REPORT"
echo "" >> "$MASTER_REPORT"
echo '```' >> "$MASTER_REPORT"
cat "$REPORT_DIR/system_info.txt" >> "$MASTER_REPORT"
echo '```' >> "$MASTER_REPORT"
echo "" >> "$MASTER_REPORT"

echo "## Test Results" >> "$MASTER_REPORT"
echo "" >> "$MASTER_REPORT"

# Run test suite
run_test "01_BuildVerification" "test_01_build_verification.sh" "Verify all daemons compile successfully on DAW02"

run_test "02_IntelHALDetection" "test_02_intel_hal_detection.sh" "Test Intel HAL integration and graceful fallback"

run_test "03_RuntimeBasic" "test_03_runtime_basic.sh" "Verify basic daemon startup and help functionality"

run_test "04_NetworkDetection" "test_04_network_detection.sh" "Test network interface detection capabilities"

run_test "05_IntelHardware" "test_05_intel_hardware.sh" "Validate Intel hardware compatibility and HAL usage"

run_test "06_MemoryLeak" "test_06_memory_leak.sh" "Memory leak detection and resource cleanup verification"

run_test "07_Integration" "test_07_integration.sh" "Multi-daemon coordination and integration testing"

# Generate final summary
echo "## Test Summary" >> "$MASTER_REPORT"
echo "" >> "$MASTER_REPORT"

TOTAL_TESTS=$(ls "$REPORT_DIR"/*_*.log 2>/dev/null | wc -l)
PASSED_TESTS=$(grep -l "✅ PASSED" "$MASTER_REPORT" | wc -l || echo 0)
FAILED_TESTS=$(grep -l "❌ FAILED" "$MASTER_REPORT" | wc -l || echo 0)
SKIPPED_TESTS=$(grep -l "⚠️ SKIPPED" "$MASTER_REPORT" | wc -l || echo 0)

cat >> "$MASTER_REPORT" << EOF
- **Total Tests:** $TOTAL_TESTS
- **Passed:** $PASSED_TESTS ✅
- **Failed:** $FAILED_TESTS ❌  
- **Skipped:** $SKIPPED_TESTS ⚠️

### Recommendations

EOF

if [ "$FAILED_TESTS" -eq 0 ]; then
    echo "🎉 **All tests passed!** The OpenAvnu daemon modernization is fully compatible with DAW02." >> "$MASTER_REPORT"
    echo "" >> "$MASTER_REPORT"
    echo "✅ **Ready for production deployment**" >> "$MASTER_REPORT"
else
    echo "⚠️ **Some tests failed.** Review failed test details above for resolution steps." >> "$MASTER_REPORT"
    echo "" >> "$MASTER_REPORT"
    echo "🔧 **Requires investigation before production deployment**" >> "$MASTER_REPORT"
fi

echo "" >> "$MASTER_REPORT"
echo "---" >> "$MASTER_REPORT"
echo "*Report generated by OpenAvnu automated testing suite*" >> "$MASTER_REPORT"
echo "*Test session: $TEST_SESSION*" >> "$MASTER_REPORT"

# Final output
echo -e "${GREEN}🎯 Testing Complete!${NC}"
echo ""
echo "📋 Master Report: $MASTER_REPORT"
echo "📁 All Results: $REPORT_DIR/"
echo ""

if [ "$FAILED_TESTS" -eq 0 ]; then
    echo -e "${GREEN}🎉 All tests passed! OpenAvnu is ready for production on DAW02.${NC}"
else
    echo -e "${YELLOW}⚠️  $FAILED_TESTS test(s) failed. Review the master report for details.${NC}"
fi

echo ""
echo "Next steps:"
echo "1. Review the master report: $MASTER_REPORT"
echo "2. Archive results: tar -czf ${TEST_SESSION}_results.tar.gz $REPORT_DIR"
echo "3. Transfer results back to development system for analysis"
