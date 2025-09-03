/**
 * Simple TSN Feature Test - Windows-only version
 * Tests actual Intel HAL TSN functions without network include conflicts
 */

#include "../../thirdparty/intel-ethernet-hal/include/intel_ethernet_hal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <windows.h>

// Test configuration constants
#define CLOCK_REALTIME 0
#define MAX_DEVICES 16
#define GATE_SCHEDULE_ENTRIES 8
#define FRAME_PREEMPTION_QUEUES 8
#define TEST_PACKET_SIZE 1500

// Windows-compatible timespec
struct tsn_timespec {
    long tv_sec;
    long tv_nsec;
};

static int get_current_time(struct tsn_timespec *tp) {
    LARGE_INTEGER frequency, counter;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&counter);
    
    tp->tv_sec = (long)(counter.QuadPart / frequency.QuadPart);
    tp->tv_nsec = (long)(((counter.QuadPart % frequency.QuadPart) * 1000000000L) / frequency.QuadPart);
    return 0;
}

// Test structures for TSN features
struct tsn_gate_control_entry {
    uint32_t gate_states;  // Bitmask for gate states
    uint32_t time_interval_ns;
};

struct tsn_admin_config {
    struct tsn_timespec base_time;
    uint32_t cycle_time_ns;
    uint8_t config_pending;
    struct tsn_gate_control_entry gate_list[GATE_SCHEDULE_ENTRIES];
    uint32_t list_length;
};

struct frame_preemption_config {
    uint8_t preemptable_queues[FRAME_PREEMPTION_QUEUES];
    uint32_t hold_advance_ns;
    uint32_t release_advance_ns;
};

// Global test results
static int test_results[4] = {0, 0, 0, 0};
static const char* test_names[] = {
    "HAL Initialization & Device Detection",
    "Time-Aware Shaper Configuration", 
    "Frame Preemption Setup",
    "Timed Packet Transmission"
};

// Forward declarations
int test_hal_init_and_detection(void);
int test_time_aware_shaper(intel_hal_device_t *device);
int test_frame_preemption(intel_hal_device_t *device);
int test_timed_transmission(intel_hal_device_t *device);

int test_hal_init_and_detection(void) {
    printf("🔍 Testing Intel HAL initialization and TSN device detection...\n");
    
    // Initialize Intel HAL
    intel_hal_ret_t ret = intel_hal_init();
    if (ret != INTEL_HAL_SUCCESS) {
        printf("❌ Intel HAL initialization failed: %d\n", ret);
        return 0;
    }
    
    printf("✅ Intel HAL initialized successfully\n");
    
    // Get device count
    uint32_t device_count = intel_hal_get_device_count();
    if (device_count == 0) {
        printf("❌ No Intel Ethernet devices found\n");
        intel_hal_cleanup();
        return 0;
    }
    
    printf("✅ Found %u Intel Ethernet device(s)\n", device_count);
    
    // Check each device for TSN capabilities
    int tsn_devices_found = 0;
    for (uint32_t i = 0; i < device_count; i++) {
        intel_hal_device_t *device = intel_hal_get_device(i);
        if (!device) continue;
        
        printf("📋 Device %u:\n", i);
        printf("   Name: %s\n", device->name);
        printf("   Family: %s\n", 
               device->info.family == INTEL_DEVICE_FAMILY_I210 ? "I210" :
               device->info.family == INTEL_DEVICE_FAMILY_I226 ? "I226" : "Unknown");
        
        // Check TSN capabilities
        if (device->capabilities & INTEL_CAP_TSN_TIME_AWARE_SHAPER) {
            printf("   ✅ Supports Time-Aware Shaper (IEEE 802.1Qbv)\n");
            tsn_devices_found++;
        } else {
            printf("   ❌ No Time-Aware Shaper support\n");
        }
        
        if (device->capabilities & INTEL_CAP_TSN_FRAME_PREEMPTION) {
            printf("   ✅ Supports Frame Preemption (IEEE 802.1Qbu)\n");
        } else {
            printf("   ❌ No Frame Preemption support\n");
        }
        
        if (device->capabilities & INTEL_CAP_ENHANCED_TIMESTAMPING) {
            printf("   ✅ Enhanced timestamping available\n");
        } else {
            printf("   ❌ Basic timestamping only\n");
        }
    }
    
    if (tsn_devices_found > 0) {
        printf("✅ Found %d device(s) with TSN capabilities\n", tsn_devices_found);
        return 1;
    } else {
        printf("❌ No TSN-capable devices found\n");
        return 0;
    }
}

int test_time_aware_shaper(intel_hal_device_t *device) {
    printf("⏰ Testing Time-Aware Shaper (IEEE 802.1Qbv) configuration...\n");
    
    if (!device || !(device->capabilities & INTEL_CAP_TSN_TIME_AWARE_SHAPER)) {
        printf("❌ Device doesn't support Time-Aware Shaper\n");
        return 0;
    }
    
    // Prepare TAS configuration
    struct tsn_admin_config tas_config = {0};
    
    // Set base time (current time + 1 second)
    get_current_time(&tas_config.base_time);
    tas_config.base_time.tv_sec += 1;
    
    // Configure 1ms cycle time
    tas_config.cycle_time_ns = 1000000; // 1ms
    
    // Create a simple gate schedule (4 entries)
    tas_config.list_length = 4;
    
    // Entry 0: High priority queue open (250μs)
    tas_config.gate_list[0].gate_states = 0x80; // Queue 7 open
    tas_config.gate_list[0].time_interval_ns = 250000;
    
    // Entry 1: Mid priority queues open (250μs)
    tas_config.gate_list[1].gate_states = 0x78; // Queues 6,5,4,3 open
    tas_config.gate_list[1].time_interval_ns = 250000;
    
    // Entry 2: All queues open (250μs)
    tas_config.gate_list[2].gate_states = 0xFF; // All queues open
    tas_config.gate_list[2].time_interval_ns = 250000;
    
    // Entry 3: Low priority queues only (250μs)
    tas_config.gate_list[3].gate_states = 0x07; // Queues 2,1,0 open
    tas_config.gate_list[3].time_interval_ns = 250000;
    
    tas_config.config_pending = 1;
    
    printf("📊 TAS Configuration:\n");
    printf("   Base Time: %ld.%09ld\n", tas_config.base_time.tv_sec, tas_config.base_time.tv_nsec);
    printf("   Cycle Time: %u ns (1ms)\n", tas_config.cycle_time_ns);
    printf("   Gate List Entries: %u\n", tas_config.list_length);
    
    for (uint32_t i = 0; i < tas_config.list_length; i++) {
        printf("   Entry %u: Gates=0x%02X, Duration=%u ns\n", 
               i, 
               tas_config.gate_list[i].gate_states,
               tas_config.gate_list[i].time_interval_ns);
    }
    
    // Apply TAS configuration using Intel HAL
    intel_hal_ret_t ret = intel_hal_setup_time_aware_shaper(
        device, 
        (void*)&tas_config
    );
    
    if (ret == INTEL_HAL_SUCCESS) {
        printf("✅ Time-Aware Shaper configured successfully\n");
        
        // Check TAS status
        uint32_t tas_status = 0;
        ret = intel_hal_get_tas_status(device, &tas_status);
        if (ret == INTEL_HAL_SUCCESS) {
            printf("✅ TAS Status: %s\n", 
                   tas_status ? "Active" : "Inactive");
        }
        
        return 1;
    } else {
        printf("❌ Time-Aware Shaper configuration failed: %d\n", ret);
        return 0;
    }
}

int test_frame_preemption(intel_hal_device_t *device) {
    printf("🔧 Testing Frame Preemption (IEEE 802.1Qbu) setup...\n");
    
    if (!device || !(device->capabilities & INTEL_CAP_TSN_FRAME_PREEMPTION)) {
        printf("❌ Device doesn't support Frame Preemption\n");
        return 0;
    }
    
    // Configure Frame Preemption
    struct frame_preemption_config fp_config = {0};
    
    // Mark queues 0-3 as preemptable, 4-7 as express
    fp_config.preemptable_queues[0] = 1; // Queue 0: preemptable
    fp_config.preemptable_queues[1] = 1; // Queue 1: preemptable  
    fp_config.preemptable_queues[2] = 1; // Queue 2: preemptable
    fp_config.preemptable_queues[3] = 1; // Queue 3: preemptable
    fp_config.preemptable_queues[4] = 0; // Queue 4: express
    fp_config.preemptable_queues[5] = 0; // Queue 5: express
    fp_config.preemptable_queues[6] = 0; // Queue 6: express
    fp_config.preemptable_queues[7] = 0; // Queue 7: express
    
    // Set timing parameters (typical values)
    fp_config.hold_advance_ns = 123000;    // Hold advance: 123μs
    fp_config.release_advance_ns = 67000;  // Release advance: 67μs
    
    printf("📊 Frame Preemption Configuration:\n");
    printf("   Preemptable Queues: ");
    for (int i = 0; i < FRAME_PREEMPTION_QUEUES; i++) {
        if (fp_config.preemptable_queues[i]) {
            printf("Q%d ", i);
        }
    }
    printf("\n   Express Queues: ");
    for (int i = 0; i < FRAME_PREEMPTION_QUEUES; i++) {
        if (!fp_config.preemptable_queues[i]) {
            printf("Q%d ", i);
        }
    }
    printf("\n   Hold Advance: %u ns\n", fp_config.hold_advance_ns);
    printf("   Release Advance: %u ns\n", fp_config.release_advance_ns);
    
    // Apply Frame Preemption configuration
    intel_hal_ret_t ret = intel_hal_setup_frame_preemption(
        device, 
        (void*)&fp_config
    );
    
    if (ret == INTEL_HAL_SUCCESS) {
        printf("✅ Frame Preemption configured successfully\n");
        
        // Check Frame Preemption status
        uint32_t fp_status = 0;
        ret = intel_hal_get_frame_preemption_status(device, &fp_status);
        if (ret == INTEL_HAL_SUCCESS) {
            printf("✅ Frame Preemption Status: %s\n", 
                   fp_status ? "Enabled" : "Disabled");
        }
        
        return 1;
    } else {
        printf("❌ Frame Preemption setup failed: %d\n", ret);
        return 0;
    }
}

int test_timed_transmission(intel_hal_device_t *device) {
    printf("📡 Testing Timed Packet Transmission (LAUNCHTIME)...\n");
    
    if (!device) {
        printf("❌ No device provided for timed transmission test\n");
        return 0;
    }
    
    // Create test packet buffer
    uint8_t test_packet[TEST_PACKET_SIZE];
    memset(test_packet, 0xAB, TEST_PACKET_SIZE); // Fill with test pattern
    
    // Set Ethernet header (simplified)
    uint8_t *eth_header = test_packet;
    
    // Destination MAC (test pattern)
    eth_header[0] = 0x01; eth_header[1] = 0x02; eth_header[2] = 0x03;
    eth_header[3] = 0x04; eth_header[4] = 0x05; eth_header[5] = 0x06;
    
    // Source MAC (test pattern) 
    eth_header[6] = 0x10; eth_header[7] = 0x20; eth_header[8] = 0x30;
    eth_header[9] = 0x40; eth_header[10] = 0x50; eth_header[11] = 0x60;
    
    // EtherType (custom test type)
    eth_header[12] = 0x88; eth_header[13] = 0xAB;
    
    // Calculate launch time (current time + 100ms)
    struct tsn_timespec launch_time;
    get_current_time(&launch_time);
    launch_time.tv_nsec += 100000000; // Add 100ms
    if (launch_time.tv_nsec >= 1000000000) {
        launch_time.tv_sec += 1;
        launch_time.tv_nsec -= 1000000000;
    }
    
    printf("📊 Timed Transmission Configuration:\n");
    printf("   Packet Size: %d bytes\n", TEST_PACKET_SIZE);
    printf("   Launch Time: %ld.%09ld\n", launch_time.tv_sec, launch_time.tv_nsec);
    printf("   Queue Priority: 7 (Highest)\n");
    
    // Prepare packet transmission parameters
    struct {
        void *packet_data;
        uint32_t packet_size;
        struct tsn_timespec launch_time;
        uint8_t queue_priority;
        uint32_t flags;
    } tx_params = {
        .packet_data = test_packet,
        .packet_size = TEST_PACKET_SIZE,
        .launch_time = launch_time,
        .queue_priority = 7, // Highest priority queue
        .flags = 0x01 // Enable LAUNCHTIME
    };
    
    // Perform timed transmission
    intel_hal_ret_t ret = intel_hal_xmit_timed_packet(device, &tx_params);
    
    if (ret == INTEL_HAL_SUCCESS) {
        printf("✅ Timed packet transmission initiated successfully\n");
        printf("📈 Packet scheduled for transmission at specified launch time\n");
        return 1;
    } else {
        printf("❌ Timed packet transmission failed: %d\n", ret);
        return 0;
    }
}

void print_test_summary(void) {
    printf("\n" "=" * 60 "\n");
    printf("🎯 TSN FEATURE TEST SUMMARY\n");
    printf("=" * 60 "\n");
    
    int passed_tests = 0;
    for (int i = 0; i < 4; i++) {
        printf("%s %s\n", 
               test_results[i] ? "✅" : "❌",
               test_names[i]);
        if (test_results[i]) passed_tests++;
    }
    
    printf("\n📊 Results: %d/4 tests passed (%.1f%%)\n", 
           passed_tests, (passed_tests * 100.0) / 4);
    
    if (passed_tests == 4) {
        printf("🏆 ALL TSN FEATURES VALIDATED SUCCESSFULLY!\n");
    } else if (passed_tests >= 2) {
        printf("⚠️  PARTIAL TSN FUNCTIONALITY AVAILABLE\n");
    } else {
        printf("❌ TSN FEATURES NOT AVAILABLE ON THIS SYSTEM\n");
    }
}

int main(void) {
    printf("\n" "=" * 60 "\n");
    printf("🚀 REAL TSN FEATURE TEST\n");
    printf("Testing actual Time-Sensitive Networking with Intel hardware\n");
    printf("=" * 60 "\n\n");
    
    intel_hal_device_t *tsn_device = NULL;
    
    // Test 1: HAL Initialization and Device Detection
    printf("TEST 1: ");
    test_results[0] = test_hal_init_and_detection();
    
    if (test_results[0]) {
        // Find the first TSN-capable device
        uint32_t device_count = intel_hal_get_device_count();
        for (uint32_t i = 0; i < device_count; i++) {
            intel_hal_device_t *device = intel_hal_get_device(i);
            if (device && (device->capabilities & INTEL_CAP_TSN_TIME_AWARE_SHAPER)) {
                tsn_device = device;
                printf("🎯 Using device '%s' for TSN testing\n\n", device->name);
                break;
            }
        }
    }
    
    // Test 2: Time-Aware Shaper Configuration
    printf("TEST 2: ");
    test_results[1] = test_time_aware_shaper(tsn_device);
    printf("\n");
    
    // Test 3: Frame Preemption Setup  
    printf("TEST 3: ");
    test_results[2] = test_frame_preemption(tsn_device);
    printf("\n");
    
    // Test 4: Timed Packet Transmission
    printf("TEST 4: ");
    test_results[3] = test_timed_transmission(tsn_device);
    printf("\n");
    
    // Cleanup
    intel_hal_cleanup();
    
    // Print summary
    print_test_summary();
    
    return (test_results[0] && test_results[1]) ? 0 : 1;
}
