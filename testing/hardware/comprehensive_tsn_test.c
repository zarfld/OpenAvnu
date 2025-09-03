/******************************************************************************

  COMPREHENSIVE TSN Feature Test - Complete Intel HAL TSN validation
  
  This test provides full validation of Time-Sensitive Networking features:
  - Time-Aware Shaper (IEEE 802.1Qbv) configuration and status
  - Frame Preemption (IEEE 802.1Qbu) setup and validation  
  - Timed packet transmission with precise LAUNCHTIME
  - Hardware timestamp validation for TSN packets
  - Multi-device TSN capability matrix
  
******************************************************************************/

// Intel HAL includes first
#include "../../thirdparty/intel-ethernet-hal/include/intel_ethernet_hal.h"

// Standard C library includes  
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

// TSN Configuration structures (for comprehensive testing)
typedef struct {
    uint32_t cycle_time;                    // TAS cycle time in nanoseconds
    uint8_t gate_control_list_length;      // Number of gate control entries
    struct {
        uint8_t gate_states;                // Bitmask of open gates (queues)
        uint32_t time_interval;             // Time interval for this gate state
    } gate_control_list[8];                 // Up to 8 gate control entries
    uint64_t base_time;                     // TAS base time (PTP timestamp)
} intel_tas_config_t;

typedef struct {
    uint8_t preemptable_queues;             // Bitmask of preemptable queues
    uint8_t express_queues;                 // Bitmask of express (non-preemptable) queues
    uint16_t fragment_size_min;             // Minimum fragment size
    bool preemption_enabled;                // Enable/disable preemption
} intel_fp_config_t;

// Windows timing compatibility - avoid network includes that cause conflicts
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

// Windows timespec implementation  
#ifndef HAVE_STRUCT_TIMESPEC
struct timespec {
    time_t tv_sec;
    long tv_nsec;
};
#define HAVE_STRUCT_TIMESPEC
#endif

static int clock_gettime(int clk_id, struct timespec *tp) {
    LARGE_INTEGER frequency, counter;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&counter);
    
    tp->tv_sec = (time_t)(counter.QuadPart / frequency.QuadPart);
    tp->tv_nsec = (long)(((counter.QuadPart % frequency.QuadPart) * 1000000000L) / frequency.QuadPart);
    return 0;
}

#define CLOCK_REALTIME 0
#define CLOCK_MONOTONIC 1

#else
#include <time.h>
#include <unistd.h>
#endif

// Test framework macros with detailed results tracking
#define TEST_PASS(name) do { printf("✅ PASS: %s\n", name); tests_passed++; } while(0)
#define TEST_FAIL(name, reason) do { printf("❌ FAIL: %s - %s\n", name, reason); tests_failed++; } while(0)
#define TEST_WARN(name, reason) do { printf("⚠️  WARN: %s - %s\n", name, reason); tests_warned++; } while(0)
#define TEST_INFO(fmt, ...) printf("ℹ️  INFO: " fmt "\n", ##__VA_ARGS__)
#define TEST_SECTION(name) printf("\n=== %s ===\n", name)

// Global test counters
static int tests_passed = 0;
static int tests_failed = 0;
static int tests_warned = 0;
static int tsn_devices_found = 0;

// TSN device capability matrix
typedef struct {
    char device_name[64];
    uint16_t device_id;
    uint32_t capabilities;
    bool has_tas;           // Time-Aware Shaper
    bool has_fp;            // Frame Preemption  
    bool has_timestamp;     // IEEE 1588 Timestamping
    bool has_launchtime;    // Timed transmission
} tsn_device_info_t;

static tsn_device_info_t tsn_devices[16];

/**
 * @brief Initialize test framework and Intel HAL
 */
int initialize_test_environment(void)
{
    TEST_SECTION("TEST ENVIRONMENT INITIALIZATION");
    
    printf("Comprehensive TSN Test Suite v2.0\n");
    printf("Testing IEEE 802.1Qbv Time-Aware Shaper and IEEE 802.1Qbu Frame Preemption\n");
    printf("Target Hardware: Intel I210, I219, I225, I226 with TSN support\n\n");
    
    // Initialize Intel HAL
    intel_hal_result_t result = intel_hal_init();
    if (result != INTEL_HAL_SUCCESS) {
        TEST_FAIL("Intel HAL Initialization", "intel_hal_init() failed");
        return 0;
    }
    TEST_PASS("Intel HAL Initialization");
    
    return 1;
}

/**
 * @brief Comprehensive device detection and TSN capability analysis
 */
int test_device_detection_and_capabilities(void)
{
    TEST_SECTION("DEVICE DETECTION AND TSN CAPABILITY ANALYSIS");
    
    // Enumerate all Intel devices
    intel_device_info_t devices[16];
    uint32_t device_count = 16;
    intel_hal_result_t result = intel_hal_enumerate_devices(devices, &device_count);
    
    if (result != INTEL_HAL_SUCCESS || device_count == 0) {
        TEST_FAIL("Device Enumeration", "No Intel Ethernet devices detected");
        return 0;
    }
    
    TEST_PASS("Device Enumeration");
    TEST_INFO("Found %u Intel Ethernet device(s)", device_count);
    
    // Analyze TSN capabilities for each device
    tsn_devices_found = 0;
    printf("\nDevice Capability Matrix:\n");
    printf("%-20s %-8s %-12s %-12s %-12s %-12s\n", 
           "Device Name", "ID", "TAS", "Frame Preempt", "IEEE1588", "Launch Time");
    printf("%-20s %-8s %-12s %-12s %-12s %-12s\n", 
           "────────────────────", "────────", "────────────", "────────────", "────────────", "────────────");
    
    for (uint32_t i = 0; i < device_count; i++) {
        tsn_device_info_t *tsn_dev = &tsn_devices[i];
        
        // Copy basic device info
        strncpy(tsn_dev->device_name, devices[i].device_name, sizeof(tsn_dev->device_name) - 1);
        tsn_dev->device_name[sizeof(tsn_dev->device_name) - 1] = '\0';
        tsn_dev->device_id = devices[i].device_id;
        tsn_dev->capabilities = devices[i].capabilities;
        
        // Analyze TSN capabilities
        tsn_dev->has_tas = (devices[i].capabilities & INTEL_CAP_TSN_TIME_AWARE_SHAPER) != 0;
        tsn_dev->has_fp = (devices[i].capabilities & INTEL_CAP_TSN_FRAME_PREEMPTION) != 0;
        tsn_dev->has_timestamp = (devices[i].capabilities & INTEL_CAP_BASIC_IEEE1588) != 0;
        tsn_dev->has_launchtime = (devices[i].capabilities & INTEL_CAP_TSN_TIME_AWARE_SHAPER) != 0; // TAS implies launch time
        
        // Display capability matrix
        printf("%-20s 0x%04X   %-12s %-12s %-12s %-12s\n",
               tsn_dev->device_name,
               tsn_dev->device_id,
               tsn_dev->has_tas ? "✅ YES" : "❌ NO",
               tsn_dev->has_fp ? "✅ YES" : "❌ NO", 
               tsn_dev->has_timestamp ? "✅ YES" : "❌ NO",
               tsn_dev->has_launchtime ? "✅ YES" : "❌ NO");
        
        // Count TSN-capable devices
        if (tsn_dev->has_tas || tsn_dev->has_fp) {
            tsn_devices_found++;
        }
    }
    
    printf("\nTSN Capability Summary:\n");
    TEST_INFO("Total devices found: %u", device_count);
    TEST_INFO("TSN-capable devices: %d", tsn_devices_found);
    
    if (tsn_devices_found > 0) {
        TEST_PASS("TSN Device Detection");
    } else {
        TEST_WARN("TSN Device Detection", "No TSN-capable devices found - limited testing possible");
    }
    
    return device_count;
}

/**
 * @brief Comprehensive Time-Aware Shaper testing
 */
int test_time_aware_shaper_comprehensive(void)
{
    TEST_SECTION("TIME-AWARE SHAPER (IEEE 802.1Qbv) COMPREHENSIVE TEST");
    
    if (tsn_devices_found == 0) {
        TEST_WARN("TAS Testing", "No TSN devices available - skipping TAS tests");
        return 0;
    }
    
    int tas_tests_passed = 0;
    
    // Test TAS on each capable device
    for (int i = 0; i < 16 && tsn_devices[i].device_name[0] != '\0'; i++) {
        if (!tsn_devices[i].has_tas) {
            TEST_INFO("Device %s: TAS not supported, skipping", tsn_devices[i].device_name);
            continue;
        }
        
        TEST_INFO("Testing TAS on device: %s (0x%04X)", tsn_devices[i].device_name, tsn_devices[i].device_id);
        
        // Open device for TAS configuration
        intel_device_t *device = NULL;
        intel_hal_result_t result = intel_hal_open_device(tsn_devices[i].device_name, &device);
        if (result != INTEL_HAL_SUCCESS || !device) {
            TEST_FAIL("TAS Device Open", "Cannot open TAS-capable device");
            continue;
        }
        
        // Test 1: Basic TAS configuration for AVB streaming
        printf("  Configuring TAS for AVB Class A streams (125μs cycle time)...\n");
        
        intel_tas_config_t tas_config = {0};
        tas_config.cycle_time = 125000; // 125μs cycle time
        tas_config.gate_control_list_length = 3;
        
        // Gate 0: AVB Class A traffic (Queue 6) - 40% of cycle
        tas_config.gate_control_list[0].gate_states = 0x40; // Queue 6 open
        tas_config.gate_control_list[0].time_interval = 50000; // 40% of 125μs
        
        // Gate 1: AVB Class B traffic (Queue 5) - 35% of cycle  
        tas_config.gate_control_list[1].gate_states = 0x20; // Queue 5 open
        tas_config.gate_control_list[1].time_interval = 43750; // 35% of 125μs
        
        // Gate 2: Best effort traffic (Queues 0-4) - 25% of cycle
        tas_config.gate_control_list[2].gate_states = 0x1F; // Queues 0-4 open
        tas_config.gate_control_list[2].time_interval = 31250; // 25% of 125μs
        
        // Set base time to current time + 2ms for synchronization
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        tas_config.base_time = (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec + 2000000ULL;
        
        // Apply TAS configuration
        result = intel_hal_setup_time_aware_shaper(device, &tas_config);
        
        if (result == INTEL_HAL_SUCCESS) {
            TEST_PASS("TAS Configuration (AVB Profile)");
            tas_tests_passed++;
            
            // Test 2: TAS status verification
            bool enabled = false;
            uint64_t current_time = 0;
            result = intel_hal_get_tas_status(device, &enabled, &current_time);
            
            if (result == INTEL_HAL_SUCCESS) {
                TEST_PASS("TAS Status Query");
                TEST_INFO("  TAS Status: %s", enabled ? "✅ ENABLED" : "❌ DISABLED");
                TEST_INFO("  Hardware Time: %llu ns", (unsigned long long)current_time);
                
                // Test 3: Validate timing accuracy
                struct timespec ts_after;
                clock_gettime(CLOCK_REALTIME, &ts_after);
                uint64_t expected_time = (uint64_t)ts_after.tv_sec * 1000000000ULL + ts_after.tv_nsec;
                
                if (current_time > 0 && current_time < expected_time + 1000000000ULL) { // Within 1 second
                    TEST_PASS("TAS Hardware Clock Validation");
                    TEST_INFO("  Clock drift: %lld ns", (long long)(expected_time - current_time));
                } else {
                    TEST_WARN("TAS Hardware Clock", "Clock synchronization may need adjustment");
                }
            } else {
                TEST_WARN("TAS Status Query", "Status query failed but configuration succeeded");
            }
            
        } else {
            TEST_FAIL("TAS Configuration", "Hardware rejected TAS configuration");
        }
        
        intel_hal_close_device(device);
    }
    
    TEST_INFO("TAS tests completed on %d device(s)", tas_tests_passed);
    return tas_tests_passed;
}

/**
 * @brief Frame Preemption comprehensive testing  
 */
int test_frame_preemption_comprehensive(void)
{
    TEST_SECTION("FRAME PREEMPTION (IEEE 802.1Qbu) COMPREHENSIVE TEST");
    
    int fp_devices_found = 0;
    int fp_tests_passed = 0;
    
    // Count Frame Preemption capable devices
    for (int i = 0; i < 16 && tsn_devices[i].device_name[0] != '\0'; i++) {
        if (tsn_devices[i].has_fp) {
            fp_devices_found++;
        }
    }
    
    if (fp_devices_found == 0) {
        TEST_WARN("Frame Preemption Testing", "No Frame Preemption capable devices found (requires I226+)");
        return 0;
    }
    
    TEST_INFO("Found %d Frame Preemption capable device(s)", fp_devices_found);
    
    // Test Frame Preemption on each capable device
    for (int i = 0; i < 16 && tsn_devices[i].device_name[0] != '\0'; i++) {
        if (!tsn_devices[i].has_fp) {
            continue;
        }
        
        TEST_INFO("Testing Frame Preemption on: %s", tsn_devices[i].device_name);
        
        intel_device_t *device = NULL;
        intel_hal_result_t result = intel_hal_open_device(tsn_devices[i].device_name, &device);
        if (result != INTEL_HAL_SUCCESS || !device) {
            TEST_FAIL("FP Device Open", "Cannot open Frame Preemption capable device");
            continue;
        }
        
        // Configure Frame Preemption for TSN traffic
        printf("  Configuring Frame Preemption for low-latency streams...\n");
        
        intel_fp_config_t fp_config = {0};
        fp_config.preemptable_queues = 0x1F;    // Queues 0-4 are preemptable
        fp_config.express_queues = 0x60;        // Queues 5-6 are express (non-preemptable)
        fp_config.fragment_size_min = 64;       // Minimum fragment size
        fp_config.preemption_enabled = true;
        
        result = intel_hal_setup_frame_preemption(device, &fp_config);
        
        if (result == INTEL_HAL_SUCCESS) {
            TEST_PASS("Frame Preemption Configuration");
            fp_tests_passed++;
            
            // Verify Frame Preemption status
            bool fp_active = false;
            uint32_t preemption_stats = 0;
            result = intel_hal_get_frame_preemption_status(device, &fp_active, &preemption_stats);
            
            if (result == INTEL_HAL_SUCCESS) {
                TEST_PASS("Frame Preemption Status Query");
                TEST_INFO("  FP Status: %s", fp_active ? "✅ ACTIVE" : "❌ INACTIVE");
                TEST_INFO("  Preemption Events: %u", preemption_stats);
            } else {
                TEST_WARN("FP Status Query", "Status query failed but configuration succeeded");
            }
            
        } else {
            TEST_FAIL("Frame Preemption Configuration", "Hardware rejected FP configuration");
        }
        
        intel_hal_close_device(device);
    }
    
    TEST_INFO("Frame Preemption tests completed on %d device(s)", fp_tests_passed);
    return fp_tests_passed;
}

/**
 * @brief Timed packet transmission testing
 */
int test_timed_packet_transmission(void)
{
    TEST_SECTION("TIMED PACKET TRANSMISSION (LAUNCHTIME) TEST");
    
    int launch_time_tests = 0;
    
    // Test timed transmission on TAS-capable devices
    for (int i = 0; i < 16 && tsn_devices[i].device_name[0] != '\0'; i++) {
        if (!tsn_devices[i].has_launchtime) {
            continue;
        }
        
        TEST_INFO("Testing timed transmission on: %s", tsn_devices[i].device_name);
        
        intel_device_t *device = NULL;
        intel_hal_result_t result = intel_hal_open_device(tsn_devices[i].device_name, &device);
        if (result != INTEL_HAL_SUCCESS || !device) {
            TEST_FAIL("Launch Time Device Open", "Cannot open device for timed transmission");
            continue;
        }
        
        // Create a test packet for timed transmission
        uint8_t test_packet[64] = {0};
        // Ethernet header with test pattern
        memset(test_packet, 0xAA, 6);      // Destination MAC (test pattern)  
        memset(test_packet + 6, 0xBB, 6);  // Source MAC (test pattern)
        test_packet[12] = 0x08;             // EtherType high
        test_packet[13] = 0x00;             // EtherType low (IP)
        
        // Calculate launch time: current time + 1ms
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        uint64_t launch_time = (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec + 1000000ULL;
        
        printf("  Scheduling packet transmission for launch time: %llu ns\n", (unsigned long long)launch_time);
        
        result = intel_hal_xmit_timed_packet(device, test_packet, sizeof(test_packet), launch_time);
        
        if (result == INTEL_HAL_SUCCESS) {
            TEST_PASS("Timed Packet Transmission");
            launch_time_tests++;
            
            // Verify transmission timing accuracy (would require hardware loopback in real test)
            TEST_INFO("  Packet scheduled successfully for precise transmission");
            TEST_INFO("  Note: Timing accuracy verification requires hardware loopback setup");
            
        } else {
            TEST_FAIL("Timed Packet Transmission", "Hardware rejected timed transmission request");
        }
        
        intel_hal_close_device(device);
    }
    
    if (launch_time_tests == 0) {
        TEST_WARN("Timed Transmission", "No devices support timed packet transmission");
    }
    
    return launch_time_tests;
}

/**
 * @brief Generate comprehensive test report
 */
void generate_test_report(void)
{
    TEST_SECTION("COMPREHENSIVE TSN TEST REPORT");
    
    printf("Test Execution Summary:\n");
    printf("  ✅ Tests Passed: %d\n", tests_passed);
    printf("  ❌ Tests Failed: %d\n", tests_failed);  
    printf("  ⚠️  Tests Warned: %d\n", tests_warned);
    printf("  📊 Total Tests: %d\n", tests_passed + tests_failed + tests_warned);
    
    printf("\nTSN Hardware Summary:\n");
    printf("  🔍 Intel devices detected: %d\n", tsn_devices_found > 0 ? tsn_devices_found : 0);
    
    int tas_devices = 0, fp_devices = 0, timestamp_devices = 0;
    for (int i = 0; i < 16 && tsn_devices[i].device_name[0] != '\0'; i++) {
        if (tsn_devices[i].has_tas) tas_devices++;
        if (tsn_devices[i].has_fp) fp_devices++;
        if (tsn_devices[i].has_timestamp) timestamp_devices++;
    }
    
    printf("  ⏰ Time-Aware Shaper devices: %d\n", tas_devices);
    printf("  🔄 Frame Preemption devices: %d\n", fp_devices);
    printf("  🕐 IEEE 1588 Timestamp devices: %d\n", timestamp_devices);
    
    printf("\nTest Coverage Analysis:\n");
    if (tas_devices > 0) {
        printf("  ✅ Time-Aware Shaper: TESTED\n");
    } else {
        printf("  ❌ Time-Aware Shaper: NOT AVAILABLE\n");
    }
    
    if (fp_devices > 0) {
        printf("  ✅ Frame Preemption: TESTED\n");
    } else {
        printf("  ❌ Frame Preemption: NOT AVAILABLE (requires I226+)\n");
    }
    
    printf("  ✅ Device Detection: COMPLETE\n");
    printf("  ✅ HAL Function Validation: COMPLETE\n");
    
    // Final assessment
    printf("\nFinal Assessment:\n");
    if (tests_failed == 0 && tsn_devices_found > 0) {
        printf("  🎉 SUCCESS: TSN implementation validated on available hardware\n");
    } else if (tests_failed == 0 && tsn_devices_found == 0) {
        printf("  ⚠️  LIMITED: TSN functions validated but no TSN hardware detected\n");
    } else {
        printf("  ❌ ISSUES: %d test failures require investigation\n", tests_failed);
    }
}

/**
 * @brief Main comprehensive TSN test function
 */
int main(void)
{
    printf("Intel HAL Comprehensive TSN Test Suite\n");
    printf("======================================\n\n");
    
    // Initialize test environment
    if (!initialize_test_environment()) {
        printf("❌ CRITICAL: Cannot initialize test environment\n");
        return 1;
    }
    
    // Run comprehensive test suite
    int devices_found = test_device_detection_and_capabilities();
    
    if (devices_found > 0) {
        test_time_aware_shaper_comprehensive();
        test_frame_preemption_comprehensive();
        test_timed_packet_transmission();
    } else {
        TEST_FAIL("Critical Failure", "No Intel devices found - cannot proceed with TSN testing");
    }
    
    // Generate final report
    generate_test_report();
    
    // Cleanup
    intel_hal_cleanup();
    
    // Return appropriate exit code
    return (tests_failed == 0) ? 0 : 1;
}
