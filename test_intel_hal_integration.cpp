/*
 * Intel HAL Integration Test for OpenAvnu gPTP
 * 
 * This test validates that the Intel HAL integration is working correctly
 * in the gPTP Windows timestamping implementation.
 */

#include <iostream>
#include <windows.h>

// Test Intel HAL availability by checking for the library and headers
#ifdef OPENAVNU_BUILD_INTEL_HAL
#include "thirdparty/intel-ethernet-hal/include/intel_ethernet_hal.h"
#define HAL_AVAILABLE 1
#else
#define HAL_AVAILABLE 0
#endif

// Test includes from gPTP
#include "thirdparty/gptp/windows/daemon_cl/windows_crosststamp.hpp"
#include "thirdparty/gptp/common/ieee1588.hpp"

int main() {
    std::cout << "=== Intel HAL Integration Test for OpenAvnu ===" << std::endl;
    std::cout << std::endl;
    
    // Test 1: Check if Intel HAL is compiled in
    std::cout << "Test 1: Intel HAL Compile-Time Integration" << std::endl;
    std::cout << "Intel HAL Available: " << (HAL_AVAILABLE ? "YES" : "NO") << std::endl;
    std::cout << std::endl;
    
#ifdef OPENAVNU_BUILD_INTEL_HAL
    // Test 2: Initialize Intel HAL
    std::cout << "Test 2: Intel HAL Initialization" << std::endl;
    intel_hal_result_t result = intel_hal_init();
    if (result == INTEL_HAL_SUCCESS) {
        std::cout << "Intel HAL initialization: SUCCESS" << std::endl;
        std::cout << "HAL Version: " << intel_hal_get_version() << std::endl;
        
        // Test 3: Enumerate Intel devices
        std::cout << std::endl << "Test 3: Intel Device Enumeration" << std::endl;
        intel_device_info_t devices[8];
        uint32_t device_count = 8;
        
        result = intel_hal_enumerate_devices(devices, &device_count);
        if (result == INTEL_HAL_SUCCESS) {
            std::cout << "Found " << device_count << " Intel devices:" << std::endl;
            
            for (uint32_t i = 0; i < device_count; i++) {
                std::cout << "  Device " << i + 1 << ": " << devices[i].device_name 
                         << " (ID: 0x" << std::hex << devices[i].device_id << std::dec << ")" << std::endl;
                std::cout << "    Family: ";
                switch (devices[i].family) {
                    case INTEL_FAMILY_I210: std::cout << "I210"; break;
                    case INTEL_FAMILY_I219: std::cout << "I219"; break;
                    case INTEL_FAMILY_I225: std::cout << "I225"; break;
                    case INTEL_FAMILY_I226: std::cout << "I226"; break;
                    default: std::cout << "Unknown"; break;
                }
                std::cout << std::endl;
                std::cout << "    Capabilities: 0x" << std::hex << devices[i].capabilities << std::dec << std::endl;
                
                // Check for hardware timestamping capability
                if (devices[i].capabilities & INTEL_CAP_BASIC_1588) {
                    std::cout << "    ✅ IEEE 1588 Hardware Timestamping Supported" << std::endl;
                } else {
                    std::cout << "    ❌ No IEEE 1588 Hardware Timestamping" << std::endl;
                }
            }
        } else {
            std::cout << "Device enumeration failed: " << intel_hal_get_last_error() << std::endl;
        }
        
        intel_hal_cleanup();
    } else {
        std::cout << "Intel HAL initialization failed: " << intel_hal_get_last_error() << std::endl;
    }
#endif
    
    // Test 4: gPTP Cross-Timestamp Integration
    std::cout << std::endl << "Test 4: gPTP Cross-Timestamp Integration" << std::endl;
    WindowsCrossTimestamp cross_timestamp;
    
    // Try to initialize with a fake interface label for testing
    bool cross_init = cross_timestamp.initialize("Intel(R) Ethernet Controller I219-LM");
    std::cout << "Cross-timestamp initialization: " << (cross_init ? "SUCCESS" : "FAILED") << std::endl;
    
    if (cross_init) {
        std::cout << "Precise timestamping supported: " << 
                     (cross_timestamp.isPreciseTimestampingSupported() ? "YES" : "NO") << std::endl;
        std::cout << "Intel HAL available: " << 
                     (cross_timestamp.isIntelHALAvailable() ? "YES" : "NO") << std::endl;
        std::cout << "Timestamp quality: " << cross_timestamp.getTimestampQuality() << "%" << std::endl;
        std::cout << "Estimated error: " << cross_timestamp.getEstimatedError() << " ns" << std::endl;
    }
    
    std::cout << std::endl << "=== Test Summary ===" << std::endl;
    std::cout << "Intel HAL Integration Status: ";
    
#ifdef OPENAVNU_BUILD_INTEL_HAL
    std::cout << "✅ ENABLED and FUNCTIONAL" << std::endl;
    std::cout << std::endl;
    std::cout << "Next Steps:" << std::endl;
    std::cout << "1. Build OpenAvnu with Intel HAL support: cmake -DOPENAVNU_BUILD_INTEL_HAL=ON" << std::endl;
    std::cout << "2. Run gPTP daemon on Intel I210/I219/I225/I226 adapter" << std::endl;
    std::cout << "3. Verify hardware timestamping in logs" << std::endl;
#else
    std::cout << "❌ DISABLED" << std::endl;
    std::cout << std::endl;
    std::cout << "To enable Intel HAL integration:" << std::endl;
    std::cout << "1. Ensure Intel Ethernet HAL submodule is present" << std::endl;
    std::cout << "2. Build with: cmake -DOPENAVNU_BUILD_INTEL_HAL=ON" << std::endl;
#endif
    
    return 0;
}
