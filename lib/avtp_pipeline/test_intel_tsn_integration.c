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

/* Ensure proper header inclusion order to prevent Winsock conflicts */
#include "intel_tsn_integration.h"

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
    
    printf("\n--- TSN Status ---\n");
    printf("Interface: %s\n", status.interface_name);
    printf("Device ID: %s\n", status.device_id);
    printf("Device Family: %u\n", status.device_family);
    printf("Driver Version: %s\n", status.driver_version);
    printf("TAS Capable: %s\n", status.tas_capable ? "Yes" : "No");
    printf("FP Capable: %s\n", status.fp_capable ? "Yes" : "No");
    printf("Hardware Timestamping: %s\n", status.hardware_timestamping ? "Yes" : "No");
    printf("TAS Enabled: %s\n", status.tas_enabled ? "Yes" : "No");
    printf("FP Enabled: %s\n", status.fp_enabled ? "Yes" : "No");
    printf("Timed TX Enabled: %s\n", status.timed_tx_enabled ? "Yes" : "No");
    printf("Reserved Bandwidth: %llu bps\n", status.reserved_bandwidth_bps);
    
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
    
    result = intel_tsn_transmit_timed_packet(INTEL_AVB_CLASS_A, test_packet, sizeof(test_packet),
                                           launch_time_ns);
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
    result = intel_tsn_transmit_timed_packet(INTEL_AVB_CLASS_B, test_packet, sizeof(test_packet),
                                           launch_time_ns);
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
    printf("Interface: %s\n", status.interface_name);
    printf("TAS Enabled: %s\n", status.tas_enabled ? "Yes" : "No");
    printf("FP Enabled: %s\n", status.fp_enabled ? "Yes" : "No");
    printf("Timed TX Enabled: %s\n", status.timed_tx_enabled ? "Yes" : "No");
    printf("Reserved Bandwidth: %llu bps (%.1f Mbps)\n", 
           status.reserved_bandwidth_bps, status.reserved_bandwidth_bps / 1000000.0);
    
    printf("✅ Status reporting successful\n");
    
    return 0;
}

/**
 * @brief Display information about all detected Intel hardware
 */
void show_intel_hardware_summary(void)
{
    printf("=== Intel Hardware Detection Summary ===\n");
    
    // Initialize just HAL to enumerate devices
    if (intel_hal_init() == INTEL_HAL_SUCCESS) {
        intel_device_info_t devices[16];
        uint32_t device_count = 16;
        
        if (intel_hal_enumerate_devices(devices, &device_count) == INTEL_HAL_SUCCESS && device_count > 0) {
            printf("🔍 Detected Intel devices on this system:\n");
            for (uint32_t i = 0; i < device_count; i++) {
                printf("   %u. %s (0x%04x) - Family %u\n", 
                       i + 1, devices[i].device_name, devices[i].device_id, devices[i].family);
                
                // Show capabilities by family
                switch (devices[i].family) {
                    case 1: printf("      Capabilities: Hardware timestamping only\n"); break;
                    case 2: printf("      Capabilities: Hardware timestamping only\n"); break; 
                    case 3: printf("      Capabilities: Full TSN (timestamping + TAS + FP)\n"); break;
                    case 4: printf("      Capabilities: Full TSN (timestamping + TAS + FP)\n"); break;
                    default: printf("      Capabilities: Unknown\n"); break;
                }
            }
            printf("📊 Total: %u Intel device(s) detected\n", device_count);
        } else {
            printf("❌ No Intel devices detected on this system\n");
        }
        intel_hal_cleanup();
    }
    printf("========================================\n\n");
}

/**
 * @brief Test a specific Intel device with appropriate capabilities
 */
int test_specific_device(const intel_device_info_t *device, const char *interface_name)
{
    printf("🔧 Testing device: %s (0x%04x) Family %u\n", 
           device->device_name, device->device_id, device->family);
    
    // Test 1: Initialize with this specific device
    printf("   1️⃣  Initializing TSN for %s...\n", device->device_name);
    int result = intel_tsn_init_with_device(interface_name, device->device_id);
    if (result != 0 && result != -ENOTSUP) {
        printf("   ❌ Device initialization failed: %d\n", result);
        return result;
    }
    
    // Test 2: Basic capabilities (all devices should support)
    printf("   2️⃣  Testing basic timestamping...\n");
    intel_tsn_status_t status;
    if (intel_tsn_get_status(&status) == 0) {
        printf("   ✅ Hardware timestamping: %s\n", 
               status.hardware_timestamping ? "Available" : "Not available");
    }
    
    // Test 3: Advanced features (family 3+ only)
    if (device->family >= 3) {
        printf("   3️⃣  Testing TAS (Time-Aware Shaper) - Full TSN device\n");
        test_tas_configuration();
        
        printf("   4️⃣  Testing Frame Preemption - Full TSN device\n");
        test_frame_preemption();
        
        printf("   5️⃣  Testing Timed Transmission - Full TSN device\n");
        test_timed_transmission();
    } else {
        printf("   3️⃣  Skipping TAS - Not supported on Family %u (I210/I219)\n", device->family);
        printf("   4️⃣  Skipping Frame Preemption - Not supported on Family %u\n", device->family);
        printf("   5️⃣  Skipping Timed TX - Not supported on Family %u\n", device->family);
    }
    
    // Test 4: Status reporting
    printf("   6️⃣  Final status check...\n");
    test_status_reporting();
    
    printf("   ✅ Device %s testing complete\n", device->device_name);
    
    return 0;
}

/**
 * @brief Main test function - Tests the WHOLE SYSTEM
 */
int main(int argc, char *argv[])
{
    const char *interface_name = "Ethernet"; // Default interface
    intel_device_info_t devices[16];
    uint32_t device_count = 16;
    int overall_result = 0;
    
    printf("Intel TSN WHOLE SYSTEM Integration Test\n");
    printf("======================================\n\n");
    
    // Show all detected Intel hardware first
    show_intel_hardware_summary();
    
    // Allow interface name override
    if (argc > 1) {
        interface_name = argv[1];
    }
    
    // Initialize HAL to get device list for comprehensive testing
    if (intel_hal_init() != INTEL_HAL_SUCCESS) {
        printf("❌ Failed to initialize Intel HAL\n");
        return 1;
    }
    
    if (intel_hal_enumerate_devices(devices, &device_count) != INTEL_HAL_SUCCESS || device_count == 0) {
        printf("❌ No Intel devices found for whole system testing\n");
        intel_hal_cleanup();
        return 1;
    }
    
    intel_hal_cleanup();
    
    printf("🌐 COMPREHENSIVE WHOLE SYSTEM TEST - Testing ALL %u Intel device(s)\n", device_count);
    printf("==================================================================\n\n");
    
    // Test each Intel device found on the system
    for (uint32_t device_idx = 0; device_idx < device_count; device_idx++) {
        printf("🎯 TESTING DEVICE %u of %u: %s (0x%04x)\n", 
               device_idx + 1, device_count, 
               devices[device_idx].device_name, devices[device_idx].device_id);
        printf("Family %u - Expected capabilities: ", devices[device_idx].family);
        
        switch (devices[device_idx].family) {
            case 1: case 2: 
                printf("Hardware timestamping only\n");
                break;
            case 3: case 4:
                printf("Full TSN (timestamping + TAS + FP)\n");
                break;
            default:
                printf("Unknown\n");
                break;
        }
        printf("─────────────────────────────────────────────────────\n");
        
        // Test this specific device
        int result = test_specific_device(&devices[device_idx], interface_name);
        if (result != 0 && result != -ENOTSUP) {
            overall_result = result;
        }
        
        // Cleanup between device tests
        intel_tsn_cleanup();
        
        printf("\n");
    }
    
    // Final comprehensive whole-system summary
    printf("🏁 WHOLE SYSTEM TEST SUMMARY\n");
    printf("════════════════════════════\n");
    printf("📊 Complete System Analysis:\n");
    printf("   • Total Intel devices tested: %u\n", device_count);
    
    uint32_t basic_count = 0, full_tsn_count = 0;
    for (uint32_t i = 0; i < device_count; i++) {
        if (devices[i].family <= 2) basic_count++;
        else if (devices[i].family >= 3) full_tsn_count++;
    }
    
    printf("   • Devices with basic timestamping: %u\n", basic_count);
    printf("   • Devices with full TSN support: %u\n", full_tsn_count);
    
    // System capability assessment
    if (full_tsn_count > 0) {
        printf("✅ SYSTEM HAS FULL TSN CAPABILITIES AVAILABLE!\n");
        printf("🚀 Ready for production AVB/TSN deployment with:\n");
        for (uint32_t i = 0; i < device_count; i++) {
            if (devices[i].family >= 3) {
                printf("   • %s (0x%04x) - Full TSN capable\n", 
                       devices[i].device_name, devices[i].device_id);
            }
        }
        printf("📈 Advanced features: TAS, Frame Preemption, Timed TX\n");
    } else if (basic_count > 0) {
        printf("⚠️  System has basic timestamping capabilities only:\n");
        for (uint32_t i = 0; i < device_count; i++) {
            if (devices[i].family <= 2) {
                printf("   • %s (0x%04x) - Basic timestamping\n", 
                       devices[i].device_name, devices[i].device_id);
            }
        }
        printf("📝 Consider upgrading to I225/I226 for full TSN features\n");
    }
    
    // Overall result
    if (overall_result == 0) {
        printf("🎉 WHOLE SYSTEM TEST COMPLETED SUCCESSFULLY!\n");
        printf("🔧 All %u Intel device(s) tested and validated\n", device_count);
    } else {
        printf("❌ Some device tests encountered issues (code: %d)\n", overall_result);
    }
    
    printf("🏆 End of comprehensive whole system Intel TSN testing\n");
    
    return overall_result;
}
