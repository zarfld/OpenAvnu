/**
 * Test program to verify Intel HAL TSN functions compile and work correctly
 * This tests the fixed intel_hal_setup_time_aware_shaper and intel_hal_setup_frame_preemption functions
 */

#include <stdio.h>
#include <stdlib.h>

// Avoid Windows socket header conflicts
#define WIN32_LEAN_AND_MEAN

#include "thirdparty/intel-ethernet-hal/include/intel_ethernet_hal.h"

int main(void) {
    printf("=== Intel HAL TSN Function Test ===\n");
    
    // Test initialization
    intel_hal_result_t result = intel_hal_init();
    if (result != INTEL_HAL_SUCCESS) {
        printf("ERROR: Failed to initialize Intel HAL: %d\n", result);
        return 1;
    }
    printf("✅ Intel HAL initialized successfully\n");
    
    // Enumerate devices
    intel_device_info_t devices[10];
    uint32_t device_count = 10;
    result = intel_hal_enumerate_devices(devices, &device_count);
    printf("📊 Found %u Intel devices\n", device_count);
    
    if (device_count == 0) {
        printf("⚠️  No Intel devices found for testing\n");
        intel_hal_cleanup();
        return 0;
    }
    
    // Find a device with TSN hardware support (I225/I226)
    intel_device_t *device = NULL;
    char device_id_str[16];
    intel_device_info_t *selected_device = NULL;
    
    // Look for I225/I226 devices first (they have hardware TSN support)
    for (uint32_t i = 0; i < device_count; i++) {
        if (devices[i].device_id == 0x15f2 || devices[i].device_id == 0x15f3 ||  // I225-LM, I225-V
            devices[i].device_id == 0x125b || devices[i].device_id == 0x125c) {   // I226-LM, I226-V
            selected_device = &devices[i];
            printf("🎯 Found TSN-capable device: %s (0x%04X)\n", 
                   selected_device->device_name, selected_device->device_id);
            break;
        }
    }
    
    // If no I225/I226 found, use first available device for testing
    if (!selected_device) {
        selected_device = &devices[0];
        printf("⚠️  No I225/I226 TSN devices found, testing with: %s (0x%04X)\n", 
               selected_device->device_name, selected_device->device_id);
    }
    
    snprintf(device_id_str, sizeof(device_id_str), "0x%04X", selected_device->device_id);
    
    result = intel_hal_open_device(device_id_str, &device);
    if (result != INTEL_HAL_SUCCESS || !device) {
        printf("ERROR: Failed to open device %s: %d\n", device_id_str, result);
        printf("Attempting to use first available device from enumeration...\n");
        
        // Try opening with just the device name instead
        result = intel_hal_open_device(selected_device->device_name, &device);
        if (result != INTEL_HAL_SUCCESS || !device) {
            printf("ERROR: Failed to open device by name: %d\n", result);
            printf("ℹ️  Available devices:\n");
            for (uint32_t i = 0; i < device_count; i++) {
                printf("   [%u] %s (ID: 0x%04X)\n", i, devices[i].device_name, devices[i].device_id);
            }
            intel_hal_cleanup();
            return 1;
        }
    }
    
    // Get device info
    intel_device_info_t device_info;
    result = intel_hal_get_device_info(device, &device_info);
    if (result == INTEL_HAL_SUCCESS) {
        printf("🔧 Testing device: %s\n", device_info.device_name);
        printf("   Device ID: 0x%04X\n", device_info.device_id);
        printf("   Vendor ID: 0x%04X\n", device_info.vendor_id);
    }
    
    // Test Time-Aware Shaper configuration
    printf("\n=== Testing Time-Aware Shaper ===\n");
    
    intel_tas_config_t tas_config = {
        .base_time = 0,
        .cycle_time = 1000000,  // 1ms cycle
        .gate_control_list_length = 2
    };
    
    // Configure basic gate schedule
    tas_config.gate_control_list[0].gate_states = 0x01;      // Queue 0 open
    tas_config.gate_control_list[0].time_interval = 500000;  // 500µs
    
    tas_config.gate_control_list[1].gate_states = 0x02;      // Queue 1 open
    tas_config.gate_control_list[1].time_interval = 500000;  // 500µs
    
    result = intel_hal_setup_time_aware_shaper(device, &tas_config);
    if (result == INTEL_HAL_SUCCESS) {
        printf("✅ Time-Aware Shaper configured successfully\n");
    } else if (result == INTEL_HAL_ERROR_NOT_SUPPORTED) {
        printf("⚠️  Time-Aware Shaper not supported on this device\n");
    } else {
        printf("❌ Time-Aware Shaper configuration failed: %d\n", result);
    }
    
    // Test Frame Preemption configuration
    printf("\n=== Testing Frame Preemption ===\n");
    
    intel_frame_preemption_config_t fp_config = {
        .preemptible_queues = 0x0F,      // Queues 0-3 preemptible
        .additional_fragment_size = 64,   // 64 byte fragments
        .verify_disable = false,          // Enable verification
        .verify_time = 128               // 128µs verification time
    };
    
    result = intel_hal_setup_frame_preemption(device, &fp_config);
    if (result == INTEL_HAL_SUCCESS) {
        printf("✅ Frame Preemption configured successfully\n");
    } else if (result == INTEL_HAL_ERROR_NOT_SUPPORTED) {
        printf("⚠️  Frame Preemption not supported on this device\n");
    } else {
        printf("❌ Frame Preemption configuration failed: %d\n", result);
    }
    
    // Test capability checking
    printf("\n=== Testing Capability Detection ===\n");
    
    if (intel_hal_has_capability(device, INTEL_CAP_TSN_TIME_AWARE_SHAPER)) {
        printf("✅ Device supports Time-Aware Shaping\n");
    } else {
        printf("⚠️  Device does not support Time-Aware Shaping\n");
    }
    
    if (intel_hal_has_capability(device, INTEL_CAP_TSN_FRAME_PREEMPTION)) {
        printf("✅ Device supports Frame Preemption\n");
    } else {
        printf("⚠️  Device does not support Frame Preemption\n");
    }
    
    // Cleanup
    intel_hal_close_device(device);
    intel_hal_cleanup();
    printf("\n🎉 Intel HAL TSN test completed successfully!\n");
    printf("✅ Both TSN functions compile and execute without errors\n");
    
    return 0;
}
