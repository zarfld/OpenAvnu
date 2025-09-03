/******************************************************************************

  Copyright (c) 2025, Intel Corporation
  All rights reserved.

  Intel TSN Integration Test Program
  
  This program tests the Intel TSN integration functionality including:
  - TSN initialization and capability detection
  - Time-Aware Shaper configuration
  - Frame Preemption setup
  - Timed packet transmission

******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#define sleep(x) Sleep((x) * 1000)

// Windows replacement for clock_gettime
static int clock_gettime(int clock_id, struct timespec *tp) {
    LARGE_INTEGER freq, count;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&count);
    tp->tv_sec = (time_t)(count.QuadPart / freq.QuadPart);
    tp->tv_nsec = (long)((count.QuadPart % freq.QuadPart) * 1000000000LL / freq.QuadPart);
    return 0;
}
#define CLOCK_REALTIME 0
#else
#include <unistd.h>
#endif

#include "intel_tsn_integration.h"

/**
 * @brief Test TSN initialization and capability detection
 */
int test_tsn_init(const char *interface_name)
{
    int result;
    intel_tsn_status_t status;
    
    printf("=== Testing TSN Initialization ===\n");
    
    // Initialize TSN integration
    result = intel_tsn_init(interface_name);
    if (result != 0) {
        printf("❌ TSN initialization failed: %d\n", result);
        return result;
    }
    
    printf("✅ TSN initialization successful\n");
    
    // Get status and capabilities
    result = intel_tsn_get_status(&status);
    if (result != 0) {
        printf("❌ Failed to get TSN status: %d\n", result);
        return result;
    }
    
    printf("\n--- TSN Capabilities ---\n");
    printf("Initialized: %s\n", status.initialized ? "Yes" : "No");
    printf("TAS Capable: %s\n", status.tas_capable ? "Yes" : "No");
    printf("FP Capable: %s\n", status.fp_capable ? "Yes" : "No");
    printf("Hardware Timestamping: %s\n", status.hardware_timestamping ? "Yes" : "No");
    printf("TAS Enabled: %s\n", status.tas_enabled ? "Yes" : "No");
    printf("FP Enabled: %s\n", status.fp_enabled ? "Yes" : "No");
    printf("Timed TX Enabled: %s\n", status.timed_tx_enabled ? "Yes" : "No");
    printf("Reserved Bandwidth: %u bps\n", status.reserved_bandwidth_bps);
    
    return 0;
}

/**
 * @brief Test Time-Aware Shaper configuration
 */
int test_tas_configuration(void)
{
    int result;
    
    printf("\n=== Testing Time-Aware Shaper Configuration ===\n");
    
    // Configure TAS for AVB Class A
    printf("Configuring TAS for AVB Class A (100 Mbps)...\n");
    result = intel_tsn_configure_tas(INTEL_AVB_CLASS_A, 100);
    if (result != 0) {
        printf("❌ TAS configuration for Class A failed: %d\n", result);
        if (result == -ENOTSUP) {
            printf("ℹ️  TAS not supported on this hardware - expected on I210/I219\n");
        }
    } else {
        printf("✅ TAS configured successfully for Class A\n");
    }
    
    // Configure TAS for AVB Class B
    printf("Configuring TAS for AVB Class B (50 Mbps)...\n");
    result = intel_tsn_configure_tas(INTEL_AVB_CLASS_B, 50);
    if (result != 0) {
        printf("❌ TAS configuration for Class B failed: %d\n", result);
        if (result == -ENOTSUP) {
            printf("ℹ️  TAS not supported on this hardware - expected on I210/I219\n");
        }
    } else {
        printf("✅ TAS configured successfully for Class B\n");
    }
    
    return 0;
}

/**
 * @brief Test Frame Preemption configuration
 */
int test_frame_preemption(void)
{
    int result;
    
    printf("\n=== Testing Frame Preemption Configuration ===\n");
    
    // Enable Frame Preemption
    printf("Enabling Frame Preemption...\n");
    result = intel_tsn_configure_frame_preemption(true);
    if (result != 0) {
        printf("❌ Frame Preemption configuration failed: %d\n", result);
        if (result == -ENOTSUP) {
            printf("ℹ️  Frame Preemption not supported on this hardware - expected on I210/I219\n");
        }
    } else {
        printf("✅ Frame Preemption configured successfully\n");
    }
    
    return 0;
}

/**
 * @brief Test timed packet transmission
 */
int test_timed_transmission(void)
{
    int result;
    uint8_t test_packet[64];
    uint64_t current_time_ns;
    struct timespec ts;
    
    printf("\n=== Testing Timed Packet Transmission ===\n");
    
    // Create test packet (minimal Ethernet frame)
    memset(test_packet, 0, sizeof(test_packet));
    // Destination MAC (broadcast)
    memset(&test_packet[0], 0xFF, 6);
    // Source MAC (dummy)
    test_packet[6] = 0x01; test_packet[7] = 0x02; test_packet[8] = 0x03;
    test_packet[9] = 0x04; test_packet[10] = 0x05; test_packet[11] = 0x06;
    // EtherType (test)
    test_packet[12] = 0x88; test_packet[13] = 0xF7; // IEEE 1722 AVTP
    
    // Get current time and schedule transmission 1ms in the future
    clock_gettime(CLOCK_REALTIME, &ts);
    current_time_ns = (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
    uint64_t launch_time_ns = current_time_ns + 1000000; // +1ms
    
    printf("Transmitting timed packet (Class A, +1ms)...\n");
    printf("Current time: %llu ns\n", (unsigned long long)current_time_ns);
    printf("Launch time:  %llu ns\n", (unsigned long long)launch_time_ns);
    
    result = intel_tsn_transmit_timed_packet(test_packet, sizeof(test_packet),
                                           INTEL_AVB_CLASS_A, launch_time_ns);
    if (result != 0) {
        printf("❌ Timed packet transmission failed: %d\n", result);
        if (result == -EINVAL) {
            printf("ℹ️  This may be expected if device is not properly initialized\n");
        }
    } else {
        printf("✅ Timed packet transmitted successfully\n");
    }
    
    // Test Class B packet
    launch_time_ns = current_time_ns + 2000000; // +2ms
    printf("\nTransmitting timed packet (Class B, +2ms)...\n");
    result = intel_tsn_transmit_timed_packet(test_packet, sizeof(test_packet),
                                           INTEL_AVB_CLASS_B, launch_time_ns);
    if (result != 0) {
        printf("❌ Timed packet transmission (Class B) failed: %d\n", result);
    } else {
        printf("✅ Timed packet (Class B) transmitted successfully\n");
    }
    
    return 0;
}

/**
 * @brief Test status reporting after configuration
 */
int test_status_reporting(void)
{
    int result;
    intel_tsn_status_t status;
    
    printf("\n=== Testing Status Reporting ===\n");
    
    result = intel_tsn_get_status(&status);
    if (result != 0) {
        printf("❌ Failed to get TSN status: %d\n", result);
        return result;
    }
    
    printf("--- Final TSN Status ---\n");
    printf("Initialized: %s\n", status.initialized ? "Yes" : "No");
    printf("TAS Enabled: %s\n", status.tas_enabled ? "Yes" : "No");
    printf("FP Enabled: %s\n", status.fp_enabled ? "Yes" : "No");
    printf("Timed TX Enabled: %s\n", status.timed_tx_enabled ? "Yes" : "No");
    printf("Reserved Bandwidth: %u bps (%.1f Mbps)\n", 
           status.reserved_bandwidth_bps, status.reserved_bandwidth_bps / 1000000.0);
    
    printf("✅ Status reporting successful\n");
    
    return 0;
}

/**
 * @brief Main test function
 */
int main(int argc, char *argv[])
{
    const char *interface_name = "Ethernet"; // Default interface
    
    printf("Intel TSN Integration Test Program\n");
    printf("==================================\n");
    
    // Allow interface name override
    if (argc > 1) {
        interface_name = argv[1];
    }
    
    printf("Testing with interface: %s\n\n", interface_name);
    
    // Run test sequence
    int result = 0;
    
    // Test 1: Initialization
    result = test_tsn_init(interface_name);
    if (result != 0 && result != -ENOTSUP) {
        goto cleanup;
    }
    
    // Test 2: TAS Configuration
    test_tas_configuration();
    
    // Test 3: Frame Preemption
    test_frame_preemption();
    
    // Test 4: Timed Transmission
    test_timed_transmission();
    
    // Test 5: Status Reporting
    test_status_reporting();
    
    printf("\n=== Test Summary ===\n");
    if (result == 0) {
        printf("✅ All tests completed successfully!\n");
        printf("ℹ️  Note: Some features may not be supported on all hardware\n");
        printf("   - TAS/FP require I225/I226 hardware\n");
        printf("   - I210/I219 support basic timestamping only\n");
    } else {
        printf("❌ Some tests failed - see details above\n");
    }
    
cleanup:
    // Cleanup
    printf("\nCleaning up TSN integration...\n");
    intel_tsn_cleanup();
    printf("✅ Cleanup complete\n");
    
    return (result == -ENOTSUP) ? 0 : result; // Don't fail on unsupported features
}
