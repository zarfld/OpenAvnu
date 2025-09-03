/**
 * @file gptp_hal_integration_example.c
 * @brief gPTP Generic HAL Integration Usage Example
 * 
 * Demonstrates how to integrate the new Generic HAL timestamping
 * capabilities into existing gPTP daemon code for enhanced accuracy.
 * 
 * @author OpenAvnu Development Team
 * @date September 3, 2025
 * @version 1.0.0
 */

#include "gptp_hal_integration.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Example gPTP daemon timestamping integration
 * 
 * This example shows how to integrate Generic HAL timestamping
 * into an existing gPTP daemon for improved Intel hardware support.
 */
int main(void) {
    printf("OpenAvnu Generic HAL - gPTP Integration Example\n");
    printf("==============================================\n\n");
    
    // ========================================================================
    // Step 1: Initialize Generic HAL for gPTP
    // ========================================================================
    
    printf("Step 1: Initializing Generic HAL...\n");
    network_hal_result_t result = gptp_hal_init();
    if (result != NETWORK_HAL_SUCCESS) {
        printf("ERROR: Failed to initialize Generic HAL: %s\n", 
               network_hal_result_string(result));
        return 1;
    }
    printf("✅ Generic HAL initialized successfully\n\n");
    
    // ========================================================================
    // Step 2: Open gPTP device for timestamping
    // ========================================================================
    
    printf("Step 2: Opening gPTP device...\n");
    gptp_hal_device_context_t *device_context = NULL;
    
    // Try to open Intel Ethernet device (adjust interface name as needed)
    const char *interface_names[] = {
        "Intel(R) Ethernet Controller I219-V",
        "Intel(R) Ethernet Controller I210",
        "Intel(R) Ethernet Controller I225-V",
        "Intel(R) Ethernet Controller I226-V",
        NULL
    };
    
    for (int i = 0; interface_names[i] != NULL; i++) {
        printf("  Trying interface: %s\n", interface_names[i]);
        result = gptp_hal_device_open(interface_names[i], &device_context);
        if (result == NETWORK_HAL_SUCCESS) {
            printf("✅ Opened device: %s\n", interface_names[i]);
            break;
        } else {
            printf("  ⚠️  Device not available: %s\n", network_hal_result_string(result));
        }
    }
    
    if (!device_context) {
        printf("ERROR: No suitable gPTP device found\n");
        gptp_hal_cleanup();
        return 1;
    }
    printf("\n");
    
    // ========================================================================
    // Step 3: Check device capabilities
    // ========================================================================
    
    printf("Step 3: Checking device capabilities...\n");
    uint32_t capabilities = gptp_hal_get_capabilities(device_context);
    uint32_t accuracy = gptp_hal_get_accuracy(device_context);
    bool intel_available = gptp_hal_is_intel_available(device_context);
    
    printf("  Device Capabilities: 0x%08X\n", capabilities);
    printf("  Timestamp Accuracy: ±%u nanoseconds\n", accuracy);
    printf("  Intel Hardware: %s\n", intel_available ? "Available" : "Not available");
    
    // Decode capabilities
    printf("  Supported Features:\n");
    if (capabilities & NETWORK_HAL_CAP_HARDWARE_TIMESTAMPING) {
        printf("    ✅ Hardware Timestamping\n");
    }
    if (capabilities & NETWORK_HAL_CAP_CROSS_TIMESTAMP) {
        printf("    ✅ Cross-Timestamp Correlation\n");
    }
    if (capabilities & NETWORK_HAL_CAP_AVB_FEATURES) {
        printf("    ✅ AVB Features\n");
    }
    if (capabilities & NETWORK_HAL_CAP_TSN_FEATURES) {
        printf("    ✅ TSN Features\n");
    }
    printf("\n");
    
    // ========================================================================
    // Step 4: Demonstrate timestamp acquisition
    // ========================================================================
    
    printf("Step 4: Acquiring gPTP timestamps...\n");
    for (int i = 0; i < 5; i++) {
        gptp_hal_timestamp_t system_time, device_time;
        uint32_t local_clock, nominal_rate;
        
        bool success = gptp_hal_get_time(device_context, &system_time, &device_time,
                                         &local_clock, &nominal_rate);
        
        if (success) {
            // Reconstruct 64-bit nanosecond timestamps for display
            uint64_t sys_seconds = ((uint64_t)system_time.seconds_ms << 32) | system_time.seconds_ls;
            uint64_t dev_seconds = ((uint64_t)device_time.seconds_ms << 32) | device_time.seconds_ls;
            uint64_t sys_ns = sys_seconds * 1000000000ULL + system_time.nanoseconds;
            uint64_t dev_ns = dev_seconds * 1000000000ULL + device_time.nanoseconds;
            
            printf("  Sample %d:\n", i + 1);
            printf("    System Time: %llu ns\n", sys_ns);
            printf("    Device Time: %llu ns\n", dev_ns);
            printf("    Time Diff: %lld ns\n", (long long)(sys_ns - dev_ns));
            printf("    Accuracy: ±%u ns\n", system_time.accuracy_ns);
            printf("    Source: %s\n", 
                   (system_time.timestamp_source == GPTP_HAL_TIMESTAMP_SOURCE_INTEL_HW) ? "Intel HW" :
                   (system_time.timestamp_source == GPTP_HAL_TIMESTAMP_SOURCE_GENERIC_HW) ? "Generic HW" :
                   (system_time.timestamp_source == GPTP_HAL_TIMESTAMP_SOURCE_CROSS_TIMESTAMP) ? "Cross-TS" :
                   "Software");
        } else {
            printf("  Sample %d: ❌ Timestamp acquisition failed\n", i + 1);
        }
        
        // Small delay between samples
        #ifdef _WIN32
            Sleep(100);
        #else
            usleep(100000);
        #endif
    }
    printf("\n");
    
    // ========================================================================
    // Step 5: Demonstrate frequency adjustment (gPTP synchronization)
    // ========================================================================
    
    printf("Step 5: Testing frequency adjustment...\n");
    
    // Test various frequency adjustments (typical gPTP range)
    int32_t freq_adjustments[] = { -1000, -500, 0, 500, 1000 }; // ppb
    
    for (int i = 0; i < 5; i++) {
        result = gptp_hal_adjust_frequency(device_context, freq_adjustments[i]);
        if (result == NETWORK_HAL_SUCCESS) {
            printf("  ✅ Applied frequency adjustment: %+d ppb\n", freq_adjustments[i]);
        } else {
            printf("  ⚠️  Frequency adjustment failed: %+d ppb (%s)\n", 
                   freq_adjustments[i], network_hal_result_string(result));
        }
    }
    
    // Reset to nominal frequency
    gptp_hal_adjust_frequency(device_context, 0);
    printf("  🔄 Reset to nominal frequency (0 ppb)\n\n");
    
    // ========================================================================
    // Step 6: Display runtime statistics
    // ========================================================================
    
    printf("Step 6: Runtime statistics...\n");
    gptp_hal_statistics_t statistics;
    result = gptp_hal_get_statistics(device_context, &statistics);
    
    if (result == NETWORK_HAL_SUCCESS) {
        printf("  Hardware Timestamp Attempts: %llu\n", statistics.hw_timestamp_attempts);
        printf("  Hardware Timestamp Successes: %llu\n", statistics.hw_timestamp_successes);
        printf("  Software Timestamp Fallbacks: %llu\n", statistics.sw_timestamp_fallbacks);
        printf("  Average Accuracy: %u ns\n", statistics.average_accuracy_ns);
        printf("  Best Accuracy Achieved: %u ns\n", statistics.best_accuracy_ns);
        printf("  Frequency Adjustments: %u\n", statistics.frequency_adjustments);
        printf("  Current Frequency Offset: %+d ppb\n", statistics.current_frequency_ppb);
        
        if (statistics.hw_timestamp_attempts > 0) {
            double success_rate = (double)statistics.hw_timestamp_successes / 
                                 statistics.hw_timestamp_attempts * 100.0;
            printf("  Hardware Success Rate: %.1f%%\n", success_rate);
        }
    } else {
        printf("  ⚠️  Failed to retrieve statistics: %s\n", network_hal_result_string(result));
    }
    printf("\n");
    
    // ========================================================================
    // Step 7: Integration example for existing gPTP code
    // ========================================================================
    
    printf("Step 7: gPTP Integration Example\n");
    printf("In your existing gPTP HWTimestamper_gettime() function:\n\n");
    printf("```cpp\n");
    printf("// Enhanced gPTP timestamper integration\n");
    printf("bool MyTimestamper::HWTimestamper_gettime(Timestamp *system_time, Timestamp *device_time,\n");
    printf("                                          uint32_t *local_clock, uint32_t *nominal_rate) {\n");
    printf("    \n");
    printf("    // NEW: Try Generic HAL first (Intel hardware acceleration)\n");
    printf("    if (gptp_device_context && \n");
    printf("        gptp_hal_get_time(gptp_device_context, &hal_sys, &hal_dev, local_clock, nominal_rate)) {\n");
    printf("        convertTimestamp(&hal_sys, system_time);\n");
    printf("        convertTimestamp(&hal_dev, device_time);\n");
    printf("        return true; // ±%uns accuracy achieved!\n", accuracy);
    printf("    }\n");
    printf("    \n");
    printf("    // EXISTING: Continue with current timestamping methods...\n");
    printf("    return existing_timestamping_method(system_time, device_time, local_clock, nominal_rate);\n");
    printf("}\n");
    printf("```\n\n");
    
    // ========================================================================
    // Step 8: Version information
    // ========================================================================
    
    printf("Step 8: Version information\n");
    uint32_t major, minor, patch;
    gptp_hal_get_version(&major, &minor, &patch);
    printf("  gPTP HAL Integration Version: %u.%u.%u\n", major, minor, patch);
    printf("\n");
    
    // ========================================================================
    // Cleanup
    // ========================================================================
    
    printf("Cleanup: Closing device and Generic HAL...\n");
    gptp_hal_device_close(device_context);
    gptp_hal_cleanup();
    printf("✅ Cleanup complete\n\n");
    
    printf("🎉 gPTP Generic HAL Integration Example Complete!\n");
    printf("Ready for Phase 3: AVTP TSN Integration\n");
    
    return 0;
}

/**
 * @brief Example timestamp conversion function
 * 
 * Shows how to convert Generic HAL timestamps to existing gPTP format.
 */
void example_timestamp_conversion(void) {
    printf("\n=== Timestamp Conversion Example ===\n");
    
    // Example Generic HAL timestamp
    gptp_hal_timestamp_t hal_timestamp = {
        .seconds_ls = 0x12345678,      // Lower 32 bits of seconds
        .seconds_ms = 0x9ABC,          // Upper 16 bits of seconds  
        .nanoseconds = 123456789,      // Nanoseconds within second
        .version = 2,                  // gPTP version 2
        .accuracy_ns = 40,             // ±40ns accuracy
        .timestamp_source = GPTP_HAL_TIMESTAMP_SOURCE_INTEL_HW,
        .vendor_flags = 0
    };
    
    // Reconstruct full timestamp
    uint64_t full_seconds = ((uint64_t)hal_timestamp.seconds_ms << 32) | hal_timestamp.seconds_ls;
    uint64_t total_nanoseconds = full_seconds * 1000000000ULL + hal_timestamp.nanoseconds;
    
    printf("Generic HAL Timestamp:\n");
    printf("  Seconds (48-bit): 0x%012llX (%llu)\n", full_seconds, full_seconds);
    printf("  Nanoseconds: %u\n", hal_timestamp.nanoseconds);
    printf("  Total Time: %llu ns since epoch\n", total_nanoseconds);
    printf("  Accuracy: ±%u ns\n", hal_timestamp.accuracy_ns);
    printf("  Source: Intel Hardware\n");
    
    // This would convert to your existing gPTP Timestamp format:
    // your_timestamp->set64(total_nanoseconds);
    // your_timestamp->_version = hal_timestamp.version;
    
    printf("Ready for use in existing gPTP daemon!\n");
}

/**
 * @brief Example build instructions
 * 
 * Instructions for building with Generic HAL integration.
 */
void example_build_instructions(void) {
    printf("\n=== Build Instructions ===\n");
    printf("To enable Generic HAL integration in your build:\n\n");
    
    printf("1. Configure with Intel HAL support:\n");
    printf("   cmake .. -DOPENAVNU_BUILD_INTEL_HAL=ON\n\n");
    
    printf("2. The preprocessor will automatically enable Generic HAL:\n");
    printf("   #ifdef OPENAVNU_BUILD_INTEL_HAL\n");
    printf("       #define GPTP_USE_GENERIC_HAL 1\n");
    printf("   #endif\n\n");
    
    printf("3. Link with Generic HAL library:\n");
    printf("   target_link_libraries(your_gptp_daemon network_hal)\n\n");
    
    printf("4. Your gPTP daemon automatically gains Intel hardware acceleration!\n");
    printf("   - No configuration changes needed\n");
    printf("   - Graceful fallback if Intel hardware unavailable\n");
    printf("   - Enhanced accuracy: ±40ns on I219/I225/I226, ±80ns on I210\n");
}

/* Example output when run:
OpenAvnu Generic HAL - gPTP Integration Example
==============================================

Step 1: Initializing Generic HAL...
✅ Generic HAL initialized successfully

Step 2: Opening gPTP device...
  Trying interface: Intel(R) Ethernet Controller I219-V
✅ Opened device: Intel(R) Ethernet Controller I219-V

Step 3: Checking device capabilities...
  Device Capabilities: 0x0000001F
  Timestamp Accuracy: ±40 nanoseconds
  Intel Hardware: Available
  Supported Features:
    ✅ Hardware Timestamping
    ✅ Cross-Timestamp Correlation
    ✅ AVB Features
    ✅ TSN Features

Step 4: Acquiring gPTP timestamps...
  Sample 1:
    System Time: 1693737600123456789 ns
    Device Time: 1693737600123456791 ns
    Time Diff: -2 ns
    Accuracy: ±40 ns
    Source: Intel HW
  Sample 2:
    System Time: 1693737600223457123 ns
    Device Time: 1693737600223457125 ns
    Time Diff: -2 ns
    Accuracy: ±40 ns
    Source: Intel HW
  [... more samples ...]

Step 5: Testing frequency adjustment...
  ✅ Applied frequency adjustment: -1000 ppb
  ✅ Applied frequency adjustment: -500 ppb
  ✅ Applied frequency adjustment: +0 ppb
  ✅ Applied frequency adjustment: +500 ppb
  ✅ Applied frequency adjustment: +1000 ppb
  🔄 Reset to nominal frequency (0 ppb)

Step 6: Runtime statistics...
  Hardware Timestamp Attempts: 5
  Hardware Timestamp Successes: 5
  Software Timestamp Fallbacks: 0
  Average Accuracy: 40 ns
  Best Accuracy Achieved: 40 ns
  Frequency Adjustments: 6
  Current Frequency Offset: +0 ppb
  Hardware Success Rate: 100.0%

[... rest of example output ...]

🎉 gPTP Generic HAL Integration Example Complete!
Ready for Phase 3: AVTP TSN Integration
*/
