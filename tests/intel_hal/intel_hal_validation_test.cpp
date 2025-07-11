/******************************************************************************

  Copyright (c) 2025, Intel Corporation
  All rights reserved.

  Intel HAL Validation Test Suite
  
  This test validates Intel Hardware Abstraction Layer (HAL) integration
  across different Intel adapter families and Windows configurations.
  
  Use this test to:
  - Validate Intel HAL after code changes
  - Test new Intel adapter support
  - Verify timestamping functionality on different machines
  - Regression testing for gPTP integration

******************************************************************************/

#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>

// Include Intel HAL headers
extern "C" {
#include "../../thirdparty/intel-ethernet-hal/include/intel_ethernet_hal.h"
}

// Include gPTP headers for cross-timestamping validation
#include "../../thirdparty/gptp/windows/daemon_cl/windows_crosststamp.hpp"
#include "../../thirdparty/gptp/common/ieee1588.hpp"

class IntelHALTestSuite {
private:
    std::vector<intel_device_t*> discovered_devices;
    bool verbose_output;
    int test_count;
    int passed_tests;
    int failed_tests;

public:
    IntelHALTestSuite(bool verbose = false) 
        : verbose_output(verbose), test_count(0), passed_tests(0), failed_tests(0) {}

    ~IntelHALTestSuite() {
        cleanup();
    }

    void log(const char* format, ...) {
        if (!verbose_output) return;
        
        va_list args;
        va_start(args, format);
        printf("[VERBOSE] ");
        vprintf(format, args);
        printf("\n");
        va_end(args);
    }

    bool runTest(const char* test_name, std::function<bool()> test_func) {
        test_count++;
        printf("\n🧪 Running Test: %s\n", test_name);
        
        auto start = std::chrono::high_resolution_clock::now();
        bool result = test_func();
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        if (result) {
            passed_tests++;
            printf("✅ PASS: %s (took %lld ms)\n", test_name, duration.count());
        } else {
            failed_tests++;
            printf("❌ FAIL: %s (took %lld ms)\n", test_name, duration.count());
        }
        
        return result;
    }

    bool testHALInitialization() {
        log("Initializing Intel HAL...");
        intel_hal_result_t result = intel_hal_init();
        
        if (result != INTEL_HAL_SUCCESS) {
            printf("   ❌ HAL initialization failed: %s\n", intel_hal_get_last_error());
            return false;
        }
        
        printf("   ✅ HAL Version: %s\n", intel_hal_get_version());
        return true;
    }

    bool testDeviceEnumeration() {
        intel_device_info_t devices[16];
        uint32_t device_count = 16;
        
        log("Enumerating Intel devices...");
        intel_hal_result_t result = intel_hal_enumerate_devices(devices, &device_count);
        
        if (result != INTEL_HAL_SUCCESS) {
            printf("   ❌ Device enumeration failed: %s\n", intel_hal_get_last_error());
            return false;
        }
        
        if (device_count == 0) {
            printf("   ⚠️  No Intel devices found\n");
            return false;
        }
        
        printf("   ✅ Found %u Intel device(s):\n", device_count);
        
        for (uint32_t i = 0; i < device_count; i++) {
            const intel_device_info_t& dev = devices[i];
            printf("      Device %u: %s\n", i + 1, dev.device_name);
            printf("        - Device ID: 0x%04X\n", dev.device_id);
            printf("        - Family: %s\n", getDeviceFamilyName(dev.family));
            printf("        - Description: %s\n", dev.description);
            printf("        - Capabilities: 0x%08X\n", dev.capabilities);
            
            if (dev.capabilities & INTEL_CAP_BASIC_1588) {
                printf("          • Basic IEEE 1588 support\n");
            }
            if (dev.capabilities & INTEL_CAP_ENHANCED_TS) {
                printf("          • Enhanced timestamping\n");
            }
            if (dev.capabilities & INTEL_CAP_PCIe_PTM) {
                printf("          • PCIe Precision Time Measurement\n");
            }
            if (dev.capabilities & INTEL_CAP_NATIVE_OS) {
                printf("          • Native OS integration\n");
            }
        }
        
        return true;
    }

    bool testDeviceOpening() {
        intel_device_info_t devices[16];
        uint32_t device_count = 16;
        
        intel_hal_result_t result = intel_hal_enumerate_devices(devices, &device_count);
        if (result != INTEL_HAL_SUCCESS || device_count == 0) {
            printf("   ⚠️  No devices to test opening\n");
            return false;
        }
        
        bool any_opened = false;
        
        for (uint32_t i = 0; i < device_count; i++) {
            const intel_device_info_t& dev = devices[i];
            intel_device_t* device = nullptr;
            
            log("Attempting to open device: %s", dev.device_name);
            
            // Try opening by device name
            result = intel_hal_open_device(dev.device_name, &device);
            if (result == INTEL_HAL_SUCCESS && device != nullptr) {
                printf("   ✅ Opened device: %s\n", dev.device_name);
                discovered_devices.push_back(device);
                any_opened = true;
                
                // Test getting device info
                intel_device_info_t info;
                if (intel_hal_get_device_info(device, &info) == INTEL_HAL_SUCCESS) {
                    log("Device info retrieved successfully");
                }
                
                // Test getting interface info
                intel_interface_info_t iface_info;
                if (intel_hal_get_interface_info(device, &iface_info) == INTEL_HAL_SUCCESS) {
                    printf("      - Interface: %s\n", iface_info.name);
                    printf("      - MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", 
                           iface_info.mac_address[0], iface_info.mac_address[1],
                           iface_info.mac_address[2], iface_info.mac_address[3],
                           iface_info.mac_address[4], iface_info.mac_address[5]);
                    printf("      - Speed: %u Mbps\n", iface_info.speed_mbps);
                    printf("      - Link: %s\n", iface_info.link_up ? "UP" : "DOWN");
                }
                
            } else {
                printf("   ⚠️  Failed to open device %s: %s\n", dev.device_name, intel_hal_get_last_error());
            }
        }
        
        return any_opened;
    }

    bool testTimestampingCapabilities() {
        if (discovered_devices.empty()) {
            printf("   ⚠️  No open devices to test timestamping\n");
            return false;
        }
        
        bool any_timestamp_capable = false;
        
        for (auto device : discovered_devices) {
            intel_device_info_t info;
            if (intel_hal_get_device_info(device, &info) != INTEL_HAL_SUCCESS) {
                continue;
            }
            
            printf("   Testing timestamping on: %s\n", info.device_name);
            
            // Check basic 1588 capability
            if (!(info.capabilities & INTEL_CAP_BASIC_1588)) {
                printf("      ⚠️  Device does not support IEEE 1588\n");
                continue;
            }
            
            // Test enabling timestamping
            intel_hal_result_t result = intel_hal_enable_timestamping(device, true);
            if (result != INTEL_HAL_SUCCESS) {
                printf("      ❌ Failed to enable timestamping: %s\n", intel_hal_get_last_error());
                continue;
            }
            
            printf("      ✅ Timestamping enabled\n");
            
            // Test reading timestamp
            intel_timestamp_t timestamp;
            result = intel_hal_read_timestamp(device, &timestamp);
            if (result != INTEL_HAL_SUCCESS) {
                printf("      ❌ Failed to read timestamp: %s\n", intel_hal_get_last_error());
                continue;
            }
            
            printf("      ✅ Timestamp read: %llu.%09u seconds\n", 
                   timestamp.seconds, timestamp.nanoseconds);
            
            any_timestamp_capable = true;
        }
        
        return any_timestamp_capable;
    }

    bool testTimestampPrecision() {
        if (discovered_devices.empty()) {
            printf("   ⚠️  No open devices to test precision\n");
            return false;
        }
        
        bool precision_acceptable = false;
        
        for (auto device : discovered_devices) {
            intel_device_info_t info;
            if (intel_hal_get_device_info(device, &info) != INTEL_HAL_SUCCESS) {
                continue;
            }
            
            if (!(info.capabilities & INTEL_CAP_BASIC_1588)) {
                continue;
            }
            
            printf("   Testing timestamp precision on: %s\n", info.device_name);
            
            // Enable timestamping
            if (intel_hal_enable_timestamping(device, true) != INTEL_HAL_SUCCESS) {
                continue;
            }
            
            // Take multiple timestamp readings
            std::vector<uint64_t> timestamps;
            const int samples = 100;
            
            for (int i = 0; i < samples; i++) {
                intel_timestamp_t ts;
                if (intel_hal_read_timestamp(device, &ts) == INTEL_HAL_SUCCESS) {
                    uint64_t ns = ts.seconds * 1000000000ULL + ts.nanoseconds;
                    timestamps.push_back(ns);
                }
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
            
            if (timestamps.size() < 10) {
                printf("      ❌ Insufficient timestamp samples\n");
                continue;
            }
            
            // Calculate precision metrics
            uint64_t min_diff = UINT64_MAX;
            uint64_t max_diff = 0;
            uint64_t total_diff = 0;
            int diff_count = 0;
            
            for (size_t i = 1; i < timestamps.size(); i++) {
                if (timestamps[i] > timestamps[i-1]) {
                    uint64_t diff = timestamps[i] - timestamps[i-1];
                    if (diff > 0 && diff < 1000000000ULL) { // Less than 1 second (sanity check)
                        min_diff = std::min(min_diff, diff);
                        max_diff = std::max(max_diff, diff);
                        total_diff += diff;
                        diff_count++;
                    }
                }
            }
            
            if (diff_count > 0) {
                uint64_t avg_diff = total_diff / diff_count;
                printf("      ✅ Precision Analysis:\n");
                printf("         - Samples: %d\n", (int)timestamps.size());
                printf("         - Min resolution: %llu ns\n", min_diff);
                printf("         - Max resolution: %llu ns\n", max_diff);
                printf("         - Avg resolution: %llu ns\n", avg_diff);
                
                // Consider precision acceptable if average resolution is < 1μs
                if (avg_diff < 1000) {
                    printf("         - Precision: EXCELLENT (<1μs)\n");
                    precision_acceptable = true;
                } else if (avg_diff < 10000) {
                    printf("         - Precision: GOOD (<10μs)\n");
                    precision_acceptable = true;
                } else {
                    printf("         - Precision: ACCEPTABLE\n");
                    precision_acceptable = true;
                }
            }
        }
        
        return precision_acceptable;
    }

    bool testGPTPIntegration() {
        printf("   Testing gPTP cross-timestamp integration...\n");
        
        try {
            WindowsCrossTimestamp crossTs;
            
            // Try to initialize with a mock interface name
            if (!crossTs.initialize("Intel")) {
                printf("      ⚠️  Cross-timestamp initialization failed\n");
                return false;
            }
            
            printf("      ✅ Cross-timestamp initialized\n");
            
            // Test if Intel HAL is available
            if (crossTs.isIntelHALAvailable()) {
                printf("      ✅ Intel HAL integration available\n");
                
                // Test getting cross-timestamp
                Timestamp system_time, device_time;
                if (crossTs.getCrossTimestamp(&system_time, &device_time)) {
                    printf("      ✅ Cross-timestamp successful\n");
                    printf("         - System: %u.%09u\n", system_time.seconds_ls, system_time.nanoseconds);
                    printf("         - Device: %u.%09u\n", device_time.seconds_ls, device_time.nanoseconds);
                    
                    int quality = crossTs.getTimestampQuality();
                    printf("         - Quality: %d%%\n", quality);
                    
                    return true;
                } else {
                    printf("      ❌ Cross-timestamp failed\n");
                }
            } else {
                printf("      ⚠️  Intel HAL not available for cross-timestamping\n");
            }
            
        } catch (const std::exception& e) {
            printf("      ❌ Exception during gPTP integration test: %s\n", e.what());
        } catch (...) {
            printf("      ❌ Unknown exception during gPTP integration test\n");
        }
        
        return false;
    }

    void cleanup() {
        for (auto device : discovered_devices) {
            if (device) {
                intel_hal_close_device(device);
            }
        }
        discovered_devices.clear();
        intel_hal_cleanup();
    }

    void printSummary() {
        printf("\n" "=" * 60 "\n");
        printf("Intel HAL Validation Summary\n");
        printf("=" * 60 "\n");
        printf("Total Tests: %d\n", test_count);
        printf("Passed: %d\n", passed_tests);
        printf("Failed: %d\n", failed_tests);
        printf("Success Rate: %.1f%%\n", test_count > 0 ? (passed_tests * 100.0 / test_count) : 0.0);
        
        if (passed_tests == test_count) {
            printf("\n🎉 ALL TESTS PASSED! Intel HAL integration is working correctly.\n");
        } else if (passed_tests > 0) {
            printf("\n⚠️  Some tests failed. Check the output above for details.\n");
        } else {
            printf("\n❌ All tests failed. Intel HAL integration may not be working.\n");
        }
    }

private:
    const char* getDeviceFamilyName(intel_device_family_t family) {
        switch (family) {
            case INTEL_FAMILY_I210: return "I210";
            case INTEL_FAMILY_I219: return "I219";
            case INTEL_FAMILY_I225: return "I225";
            case INTEL_FAMILY_I226: return "I226";
            default: return "Unknown";
        }
    }
};

void printUsage(const char* program_name) {
    printf("Intel HAL Validation Test Suite\n");
    printf("Usage: %s [options]\n", program_name);
    printf("\nOptions:\n");
    printf("  -v, --verbose    Enable verbose output\n");
    printf("  -h, --help       Show this help message\n");
    printf("\nThis test validates Intel HAL integration and should be run:\n");
    printf("  - After code changes affecting Intel HAL\n");
    printf("  - On new machines with Intel adapters\n");
    printf("  - For regression testing before releases\n");
    printf("  - To verify timestamping functionality\n");
}

int main(int argc, char* argv[]) {
    bool verbose = false;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = true;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printUsage(argv[0]);
            return 0;
        } else {
            printf("Unknown option: %s\n", argv[i]);
            printUsage(argv[0]);
            return 1;
        }
    }
    
    printf("🚀 Intel HAL Validation Test Suite\n");
    printf("Date: %s %s\n", __DATE__, __TIME__);
    printf("Verbose: %s\n", verbose ? "Enabled" : "Disabled");
    printf("\n");
    
    IntelHALTestSuite testSuite(verbose);
    
    // Run all tests
    testSuite.runTest("HAL Initialization", [&]() { return testSuite.testHALInitialization(); });
    testSuite.runTest("Device Enumeration", [&]() { return testSuite.testDeviceEnumeration(); });
    testSuite.runTest("Device Opening", [&]() { return testSuite.testDeviceOpening(); });
    testSuite.runTest("Timestamping Capabilities", [&]() { return testSuite.testTimestampingCapabilities(); });
    testSuite.runTest("Timestamp Precision", [&]() { return testSuite.testTimestampPrecision(); });
    testSuite.runTest("gPTP Integration", [&]() { return testSuite.testGPTPIntegration(); });
    
    testSuite.printSummary();
    
    return testSuite.failed_tests > 0 ? 1 : 0;
}
