/*
 * Intel I219 VLAN/QoS Configuration Example
 * Demonstrates 802.1Q VLAN and 802.1p QoS features using Intel Ethernet HAL
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
#else
    #include <unistd.h>
#endif

#include "../thirdparty/intel-ethernet-hal/include/intel_ethernet_hal.h"

static int configure_i219_vlan_filtering(intel_device_t *device)
{
    printf("\n=== Configuring VLAN Filtering ===\n");
    
    // Configure VLAN tag
    intel_vlan_tag_t vlan_tag = {
        .vlan_id = 100,
        .priority = 6,      // High priority for AVB Class A
        .dei = 0
    };
    
    intel_hal_result_t result = intel_hal_set_vlan_tag(device, &vlan_tag);
    if (result != INTEL_HAL_SUCCESS) {
        printf("  ❌ Failed to set VLAN tag: %d\n", result);
        return result;
    }
    
    // Enable VLAN filtering
    result = intel_hal_configure_vlan_filter(device, 100, true);
    if (result != INTEL_HAL_SUCCESS) {
        printf("  ❌ Failed to configure VLAN filter: %d\n", result);
        return result;
    }
    
    printf("  ✓ VLAN 100 configured with priority 6\n");
    return INTEL_HAL_SUCCESS;
}

static int configure_i219_qos_mapping(intel_device_t *device)
{
    printf("\n=== Configuring QoS Priority Mapping ===\n");
    
    // Map 802.1p priorities to traffic classes
    struct {
        uint8_t user_priority;
        uint8_t traffic_class;
    } qos_mappings[] = {
        {6, 6},  // Voice (AVB Class A)
        {5, 5},  // Video (AVB Class B)
        {4, 4},  // Critical Applications
        {3, 3},  // Excellent Effort
        {0, 0}   // Best Effort
    };
    
    for (int i = 0; i < 5; i++) {
        intel_hal_result_t result = intel_hal_configure_priority_mapping(
            device, 
            qos_mappings[i].user_priority, 
            qos_mappings[i].traffic_class);
            
        if (result != INTEL_HAL_SUCCESS) {
            printf("  ❌ Failed to map priority %d to traffic class %d: %d\n", 
                   qos_mappings[i].user_priority, qos_mappings[i].traffic_class, result);
            return result;
        }
        
        printf("  ✓ Priority %d mapped to traffic class %d\n", 
               qos_mappings[i].user_priority, qos_mappings[i].traffic_class);
    }
    
    return INTEL_HAL_SUCCESS;
}

static int configure_i219_cbs(intel_device_t *device)
{
    printf("\n=== Configuring Credit-Based Shaper ===\n");
    
    // Configure Class A (high priority audio)
    intel_cbs_config_t class_a_cbs = {
        .traffic_class = INTEL_AVB_CLASS_A,
        .send_slope = 125000000,    // 125 Mbps
        .idle_slope = 25000000,     // 25 Mbps
        .hi_credit = 5000,
        .lo_credit = -5000,
        .enabled = true
    };
    
    intel_hal_result_t result = intel_hal_configure_cbs(device, 6, &class_a_cbs);
    if (result != INTEL_HAL_SUCCESS) {
        printf("  ❌ Failed to configure Class A CBS: %d\n", result);
        return result;
    }
    
    printf("  ✓ Class A CBS configured (Queue 6)\n");
    
    // Configure Class B (standard priority audio)
    intel_cbs_config_t class_b_cbs = {
        .traffic_class = INTEL_AVB_CLASS_B,
        .send_slope = 125000000,    // 125 Mbps  
        .idle_slope = 12500000,     // 12.5 Mbps
        .hi_credit = 2500,
        .lo_credit = -2500,
        .enabled = true
    };
    
    result = intel_hal_configure_cbs(device, 5, &class_b_cbs);
    if (result != INTEL_HAL_SUCCESS) {
        printf("  ❌ Failed to configure Class B CBS: %d\n", result);
        return result;
    }
    
    printf("  ✓ Class B CBS configured (Queue 5)\n");
    
    return INTEL_HAL_SUCCESS;
}

static int configure_i219_advanced_qos(intel_device_t *device)
{
    printf("\n=== Configuring Advanced QoS Features ===\n");
    
    // Configure bandwidth allocation
    intel_hal_result_t result = intel_hal_configure_bandwidth_allocation(device, 6, 25);  // 25% for Class A
    if (result != INTEL_HAL_SUCCESS) {
        printf("  ❌ Failed to configure bandwidth allocation: %d\n", result);
        return result;
    }
    
    // Configure rate limiting for management traffic
    result = intel_hal_set_rate_limit(device, 0, 10);  // 10 Mbps limit for best effort
    if (result != INTEL_HAL_SUCCESS) {
        printf("  ❌ Failed to configure rate limiting: %d\n", result);
        return result;
    }
    
    printf("  ✓ Bandwidth allocation and rate limiting configured\n");
    
    return INTEL_HAL_SUCCESS;
}

static int verify_i219_configuration(intel_device_t *device)
{
    printf("\n=== Verifying Configuration ===\n");
    
    // Verify VLAN configuration
    intel_vlan_tag_t current_vlan;
    intel_hal_result_t result = intel_hal_get_vlan_tag(device, &current_vlan);
    if (result == INTEL_HAL_SUCCESS) {
        printf("  ✓ Current VLAN: ID=%d, Priority=%d\n", 
               current_vlan.vlan_id, current_vlan.priority);
    } else {
        printf("  ❌ Failed to get VLAN configuration: %d\n", result);
    }
    
    // Verify CBS configuration for Class A
    intel_cbs_config_t current_cbs;
    result = intel_hal_get_cbs_config(device, 6, &current_cbs);
    if (result == INTEL_HAL_SUCCESS) {
        printf("  ✓ Class A CBS: Send Slope=%u, Enabled=%s\n", 
               current_cbs.send_slope, current_cbs.enabled ? "Yes" : "No");
    } else {
        printf("  ❌ Failed to get CBS configuration: %d\n", result);
    }
    
    return INTEL_HAL_SUCCESS;
}

int main(int argc, char *argv[])
{
    printf("Intel I219 VLAN/QoS Configuration Example\n");
    printf("==========================================\n");
    
    // Initialize HAL
    intel_hal_result_t result = intel_hal_init();
    if (result != INTEL_HAL_SUCCESS) {
        printf("❌ Failed to initialize Intel HAL: %d\n", result);
        return -1;
    }
    
    // Enumerate devices
    intel_device_info_t devices[10];
    uint32_t device_count = 10;
    result = intel_hal_enumerate_devices(devices, &device_count);
    if (result != INTEL_HAL_SUCCESS || device_count == 0) {
        printf("❌ No Intel devices found: %d\n", result);
        intel_hal_cleanup();
        return -1;
    }
    
    // Find I219 device (Familie 3 = INTEL_FAMILY_I219)
    intel_device_t *device = NULL;
    for (uint32_t i = 0; i < device_count; i++) {
        printf("Checking device %d: Family=%d, Device ID=0x%04X\n", 
               i, devices[i].family, devices[i].device_id);
        
        // I219 devices are family 3
        if (devices[i].family == 3) {
            result = intel_hal_open_device(devices[i].device_name, &device);
            if (result == INTEL_HAL_SUCCESS) {
                printf("✓ Found and opened I219 device: %s\n", devices[i].device_name);
                break;
            }
        }
    }
    
    if (device == NULL) {
        printf("❌ No I219 device found or failed to open\n");
        intel_hal_cleanup();
        return -1;
    }
    
    // Configure VLAN/QoS features
    if (configure_i219_vlan_filtering(device) != INTEL_HAL_SUCCESS ||
        configure_i219_qos_mapping(device) != INTEL_HAL_SUCCESS ||
        configure_i219_cbs(device) != INTEL_HAL_SUCCESS ||
        configure_i219_advanced_qos(device) != INTEL_HAL_SUCCESS ||
        verify_i219_configuration(device) != INTEL_HAL_SUCCESS) {
        
        printf("\n❌ Configuration failed\n");
        intel_hal_close_device(device);
        intel_hal_cleanup();
        return -1;
    }
    
    printf("\n✅ Intel I219 VLAN/QoS Configuration Complete!\n");
    printf("\nConfigured Features:\n");
    printf("  • 802.1Q VLAN tagging (VLAN 100)\n");
    printf("  • 802.1p QoS priority mapping\n");
    printf("  • AVB Credit-Based Shaping (Class A & B)\n");
    printf("  • Bandwidth allocation and rate limiting\n");
    printf("  • Professional audio streaming optimization\n");
    
    // Cleanup
    intel_hal_close_device(device);
    intel_hal_cleanup();
    
    return 0;
}
