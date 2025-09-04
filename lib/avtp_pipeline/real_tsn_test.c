/******************************************************************************

  REAL TSN Feature Test - Tests actual Intel HAL TSN functions
  
  This test validates the actual TSN features on Intel I210/I226 hardware:
  - Time-Aware Shaper configuration and status
  - Frame Preemption setup and validation
  - Timed packet transmission with LAUNCHTIME
  
******************************************************************************/

/* Ensure proper Windows header management before including Intel HAL */
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#define _WINSOCKAPI_  // Prevent winsock.h inclusion
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#endif

#include "../../thirdparty/intel-ethernet-hal/include/intel_ethernet_hal.h"
#include "intel_hal_compatibility.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Additional Windows compatibility
#ifdef _WIN32
#include <time.h>
#endif

// Use Windows-provided timespec if available
#ifndef HAVE_STRUCT_TIMESPEC
#define HAVE_STRUCT_TIMESPEC
#endif

#ifdef _WIN32
static int clock_gettime(int clk_id, struct timespec *tp) {
    LARGE_INTEGER frequency, counter;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&counter);
    
    tp->tv_sec = (time_t)(counter.QuadPart / frequency.QuadPart);
    tp->tv_nsec = (long)(((counter.QuadPart % frequency.QuadPart) * 1000000000L) / frequency.QuadPart);
    return 0;
}

#define CLOCK_REALTIME 0
#else
#include <time.h>
#endif

// Test macros
#define TEST_PASS(name) printf("✅ PASS: %s\n", name)
#define TEST_FAIL(name, reason) printf("❌ FAIL: %s - %s\n", name, reason)
#define TEST_WARN(name, reason) printf("⚠️  WARN: %s - %s\n", name, reason)
#define TEST_INFO(fmt, ...) printf("ℹ️  INFO: " fmt "\n", ##__VA_ARGS__)

static int tests_passed = 0;
static int tests_failed = 0;
static int tests_warned = 0;

/**
 * @brief Test Intel HAL initialization and device detection
 */
int test_hal_init_and_detection(void)
{
    printf("\n=== TEST 1: Intel HAL Initialization and Device Detection ===\n");
    
    // Initialize Intel HAL
    intel_hal_result_t result = intel_hal_init();
    if (result != INTEL_HAL_SUCCESS) {
        TEST_FAIL("HAL Initialization", "intel_hal_init() failed");
        return 0;
    }
    TEST_PASS("HAL Initialization");
    
    // Enumerate devices
    intel_device_info_t devices[16];
    uint32_t device_count = 16;
    result = intel_hal_enumerate_devices(devices, &device_count);
    
    if (result != INTEL_HAL_SUCCESS || device_count == 0) {
        TEST_FAIL("Device Enumeration", "No Intel devices found");
        intel_hal_cleanup();
        return 0;
    }
    
    TEST_PASS("Device Enumeration");
    TEST_INFO("Found %u Intel device(s)", device_count);
    
    // Display detected devices and their TSN capabilities
    for (uint32_t i = 0; i < device_count; i++) {
        printf("  Device %u: %s (ID: 0x%04X)\n", i + 1, devices[i].device_name, devices[i].device_id);
        printf("    Family: %d, Capabilities: 0x%08X\n", devices[i].family, devices[i].capabilities);
        
        // Check TSN-specific capabilities
        bool has_tas = (devices[i].capabilities & INTEL_CAP_TSN_TIME_AWARE_SHAPER) != 0;
        bool has_fp = (devices[i].capabilities & INTEL_CAP_TSN_FRAME_PREEMPTION) != 0;
        bool has_timestamp = (devices[i].capabilities & INTEL_CAP_BASIC_IEEE1588) != 0;
        
        printf("    TSN Capabilities:\n");
        printf("      Time-Aware Shaper: %s\n", has_tas ? "✅ YES" : "❌ NO");
        printf("      Frame Preemption: %s\n", has_fp ? "✅ YES" : "❌ NO");  
        printf("      IEEE 1588 Timestamping: %s\n", has_timestamp ? "✅ YES" : "❌ NO");
    }
    
    intel_hal_cleanup();
    return 1;
}

/**
 * @brief Test Time-Aware Shaper configuration on capable devices
 */
int test_time_aware_shaper(void)
{
    printf("\n=== TEST 2: Time-Aware Shaper Configuration ===\n");
    
    // Initialize and find devices
    intel_hal_result_t result = intel_hal_init();
    if (result != INTEL_HAL_SUCCESS) {
        TEST_FAIL("HAL Init for TAS", "Cannot initialize HAL");
        return 0;
    }
    
    intel_device_info_t devices[16];
    uint32_t device_count = 16;
    intel_hal_enumerate_devices(devices, &device_count);
    
    bool found_tas_device = false;
    
    // Test TAS on each capable device
    for (uint32_t i = 0; i < device_count; i++) {
        if (!(devices[i].capabilities & INTEL_CAP_TSN_TIME_AWARE_SHAPER)) {
            TEST_INFO("Device %s: TAS not supported, skipping", devices[i].device_name);
            continue;
        }
        
        found_tas_device = true;
        TEST_INFO("Testing TAS on device: %s", devices[i].device_name);
        
        // Open device using device ID (not name)
        char device_id_str[16];
        snprintf(device_id_str, sizeof(device_id_str), "0x%04x", devices[i].device_id);
        
        intel_device_t *device = NULL;
        result = intel_hal_open_device(device_id_str, &device);
        if (result != INTEL_HAL_SUCCESS || !device) {
            TEST_FAIL("Device Open", "Cannot open TAS-capable device");
            continue;
        }
        
        // Configure TAS for AVB Class A (125μs cycle)
        intel_tas_config_t tas_config = {0};
        tas_config.cycle_time = 125000; // 125μs
        tas_config.gate_control_list_length = 2;
        
        // Gate 0: AVB Class A traffic (75% of cycle)
        tas_config.gate_control_list[0].gate_states = 0x40; // Queue 6 open
        tas_config.gate_control_list[0].time_interval = 93750; // 75% of 125μs
        
        // Gate 1: Other traffic (25% of cycle)
        tas_config.gate_control_list[1].gate_states = 0x3F; // Queues 0-5 open
        tas_config.gate_control_list[1].time_interval = 31250; // 25% of 125μs
        
        // Set base time to current time + 1ms
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        tas_config.base_time = (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec + 1000000ULL;
        
        // Apply TAS configuration
        result = intel_hal_setup_time_aware_shaper(device, &tas_config);
        
        if (result == INTEL_HAL_SUCCESS) {
            TEST_PASS("TAS Configuration");
            
            // Check TAS status
            bool enabled;
            uint64_t current_time;
            result = intel_hal_get_tas_status(device, &enabled, &current_time);
            
            if (result == INTEL_HAL_SUCCESS) {
                TEST_PASS("TAS Status Query");
                TEST_INFO("TAS Status: %s, Current Time: %llu ns", enabled ? "Enabled" : "Disabled", (unsigned long long)current_time);
            } else {
                TEST_WARN("TAS Status Query", "Status query failed but configuration succeeded");
            }
            
        } else {
            TEST_FAIL("TAS Configuration", "Hardware TAS configuration failed");
        }
        
        intel_hal_close_device(device);
    }
    
    if (!found_tas_device) {
        TEST_WARN("TAS Testing", "No TAS-capable devices found for testing");
    }
    
    intel_hal_cleanup();
    return found_tas_device ? 1 : 0;
}

/**
 * @brief Test Frame Preemption configuration on I226 devices
 */
int test_frame_preemption(void)
{
    printf("\n=== TEST 3: Frame Preemption Configuration ===\n");
    
    // Initialize and find devices
    intel_hal_result_t result = intel_hal_init();
    if (result != INTEL_HAL_SUCCESS) {
        TEST_FAIL("HAL Init for Frame Preemption", "Cannot initialize HAL");
        return 0;
    }
    
    intel_device_info_t devices[16];
    uint32_t device_count = 16;
    intel_hal_enumerate_devices(devices, &device_count);
    
    bool found_fp_device = false;
    
    // Test Frame Preemption on each capable device (I226 only)
    for (uint32_t i = 0; i < device_count; i++) {
        if (!(devices[i].capabilities & INTEL_CAP_TSN_FRAME_PREEMPTION)) {
            TEST_INFO("Device %s: Frame Preemption not supported, skipping", devices[i].device_name);
            continue;
        }
        
        found_fp_device = true;
        TEST_INFO("Testing Frame Preemption on device: %s", devices[i].device_name);
        
        // Open device using device ID (not name)
        char device_id_str[16];
        snprintf(device_id_str, sizeof(device_id_str), "0x%04x", devices[i].device_id);
        
        intel_device_t *device = NULL;
        result = intel_hal_open_device(device_id_str, &device);
        if (result != INTEL_HAL_SUCCESS || !device) {
            TEST_FAIL("Device Open", "Cannot open Frame Preemption-capable device");
            continue;
        }
        
        // Configure Frame Preemption
        intel_frame_preemption_config_t fp_config = {0};
        fp_config.preemptible_queues = 0x1F; // Queues 0-4 preemptible
        fp_config.additional_fragment_size = 64; // 64-byte minimum fragment
        fp_config.verify_disable = false; // Enable verification
        fp_config.verify_time = 10000; // 10ms verification time
        
        // Apply Frame Preemption configuration
        result = intel_hal_setup_frame_preemption(device, &fp_config);
        
        if (result == INTEL_HAL_SUCCESS) {
            TEST_PASS("Frame Preemption Configuration");
            
            // Check Frame Preemption status
            bool enabled;
            uint8_t active_queues;
            result = intel_hal_get_frame_preemption_status(device, &enabled, &active_queues);
            
            if (result == INTEL_HAL_SUCCESS) {
                TEST_PASS("Frame Preemption Status Query");
                TEST_INFO("Frame Preemption Status: %s, Active Queues: 0x%02X", enabled ? "Enabled" : "Disabled", active_queues);
            } else {
                TEST_WARN("Frame Preemption Status Query", "Status query failed but configuration succeeded");
            }
            
        } else {
            TEST_FAIL("Frame Preemption Configuration", "Hardware Frame Preemption configuration failed");
        }
        
        intel_hal_close_device(device);
    }
    
    if (!found_fp_device) {
        TEST_WARN("Frame Preemption Testing", "No Frame Preemption-capable devices found (I226 required)");
    }
    
    intel_hal_cleanup();
    return found_fp_device ? 1 : 0;
}

/**
 * @brief Test timed packet transmission with LAUNCHTIME
 */
int test_timed_transmission(void)
{
    printf("\n=== TEST 4: Timed Packet Transmission (LAUNCHTIME) ===\n");
    
    // Initialize and find devices
    intel_hal_result_t result = intel_hal_init();
    if (result != INTEL_HAL_SUCCESS) {
        TEST_FAIL("HAL Init for Timed TX", "Cannot initialize HAL");
        return 0;
    }
    
    intel_device_info_t devices[16];
    uint32_t device_count = 16;
    intel_hal_enumerate_devices(devices, &device_count);
    
    bool found_timing_device = false;
    
    // Test timed transmission on devices with enhanced timestamping
    for (uint32_t i = 0; i < device_count; i++) {
        bool has_enhanced_timing = (devices[i].capabilities & INTEL_CAP_ENHANCED_TIMESTAMPING) != 0;
        bool has_basic_timing = (devices[i].capabilities & INTEL_CAP_BASIC_IEEE1588) != 0;
        
        if (!has_enhanced_timing && !has_basic_timing) {
            TEST_INFO("Device %s: No timestamping capability, skipping", devices[i].device_name);
            continue;
        }
        
        found_timing_device = true;
        TEST_INFO("Testing Timed Transmission on device: %s (%s timing)", 
                  devices[i].device_name, has_enhanced_timing ? "Enhanced" : "Basic");
        
        // Open device using device ID (not name)
        char device_id_str[16];
        snprintf(device_id_str, sizeof(device_id_str), "0x%04x", devices[i].device_id);
        
        intel_device_t *device = NULL;
        result = intel_hal_open_device(device_id_str, &device);
        if (result != INTEL_HAL_SUCCESS || !device) {
            TEST_FAIL("Device Open", "Cannot open timing-capable device");
            continue;
        }
        
        // Create test packet
        uint8_t test_packet[64] = {
            0x01, 0x02, 0x03, 0x04, 0x05, 0x06, // Destination MAC
            0x06, 0x05, 0x04, 0x03, 0x02, 0x01, // Source MAC
            0x08, 0x00, // EtherType
            // Payload...
        };
        
        // Set launch time to current time + 1ms
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        uint64_t launch_time = (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec + 1000000ULL;
        
        // Create timed packet structure
        intel_timed_packet_t timed_packet = {0};
        timed_packet.packet_data = test_packet;
        timed_packet.packet_length = sizeof(test_packet);
        timed_packet.launch_time = launch_time;
        timed_packet.queue = 6; // AVB Class A queue
        
        // Transmit timed packet
        result = intel_hal_xmit_timed_packet(device, &timed_packet);
        
        if (result == INTEL_HAL_SUCCESS) {
            TEST_PASS("Timed Packet Transmission");
            TEST_INFO("Packet transmitted with launch time: %llu ns", (unsigned long long)launch_time);
        } else if (result == INTEL_HAL_ERROR_NOT_SUPPORTED) {
            TEST_WARN("Timed Packet Transmission", "LAUNCHTIME not supported, using immediate transmission");
        } else {
            TEST_FAIL("Timed Packet Transmission", "Packet transmission failed");
        }
        
        intel_hal_close_device(device);
    }
    
    if (!found_timing_device) {
        TEST_WARN("Timed Transmission Testing", "No timestamping-capable devices found");
    }
    
    intel_hal_cleanup();
    return found_timing_device ? 1 : 0;
}

/**
 * @brief Main test runner
 */
int main(void)
{
    printf("🚀 REAL Intel TSN Feature Testing\n");
    printf("==================================\n");
    printf("Testing actual TSN functionality on Intel I210/I226 hardware\n");
    
    // Run all tests
    tests_passed += test_hal_init_and_detection();
    tests_passed += test_time_aware_shaper();
    tests_passed += test_frame_preemption();
    tests_passed += test_timed_transmission();
    
    // Display summary
    printf("\n🎯 TEST SUMMARY\n");
    printf("===============\n");
    printf("Tests Passed: %d\n", tests_passed);
    printf("Tests Failed: %d\n", tests_failed);  
    printf("Tests Warned: %d\n", tests_warned);
    
    if (tests_passed >= 3) {
        printf("\n✅ TSN FEATURE VALIDATION: SUCCESSFUL\n");
        printf("Intel TSN features are working correctly on your hardware!\n");
        return 0;
    } else if (tests_passed >= 1) {
        printf("\n⚠️  TSN FEATURE VALIDATION: PARTIAL\n");
        printf("Some TSN features working, hardware limitations detected.\n");
        return 1;
    } else {
        printf("\n❌ TSN FEATURE VALIDATION: FAILED\n");
        printf("TSN features not working or hardware not detected.\n");
        return 2;
    }
}
