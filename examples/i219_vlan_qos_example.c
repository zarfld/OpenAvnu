/*
 * Intel I219 VLAN/QoS Configuration Example
 * Demonstrates 802.1Q VLAN and 802.1p QoS features using Intel Ethernet HAL
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
#endif

#include "intel_ethernet_hal.h"

// Simple configuration structure
typedef struct {
    uint16_t vlan_id;
    uint8_t priority;
} simple_vlan_config_t;

// Professional AVB network configuration
static avb_network_config_t default_avb_config = {
    .management_vlan = 1,          // Default management VLAN
    .audio_class_a_vlan = 100,     // Class A audio VLAN
    .audio_class_b_vlan = 200,     // Class B audio VLAN  
    .control_vlan = 300,           // Control protocol VLAN
    
    .class_a_priority = 6,         // Highest audio priority
    .class_b_priority = 5,         // Second audio priority
    .control_priority = 4,         // Control protocol priority
    .management_priority = 2,      // Management priority
    
    .class_a_bandwidth = 75000000, // 75% of bandwidth for Class A (75 Mbps on 100 Mbps link)
    .class_b_bandwidth = 12500000, // 12.5% for Class B (12.5 Mbps)
    
    .observation_interval = 125    // 125 microseconds (standard for professional audio)
};

static int configure_i219_vlan_filtering(intel_device_handle_t device, const avb_network_config_t* config)
{
    printf("Configuring I219 VLAN filtering...\n");
    
    // Configure VLAN filter table for all required VLANs
    uint16_t vlans[] = {
        config->management_vlan,
        config->audio_class_a_vlan,
        config->audio_class_b_vlan,
        config->control_vlan
    };
    
    for (int i = 0; i < 4; i++) {
        intel_vlan_tag_t vlan_config = {
            .vlan_id = vlans[i],
            .priority = (i == 0) ? config->management_priority :
                       (i == 1) ? config->class_a_priority :
                       (i == 2) ? config->class_b_priority :
                                  config->control_priority,
            .cfi = 0,  // Canonical Format Indicator
            .tpid = 0x8100  // Standard 802.1Q TPID
        };
        
        int result = intel_configure_vlan_filter(device, &vlan_config);
        if (result != INTEL_HAL_SUCCESS) {
            printf("Failed to configure VLAN %d: error %d\n", vlans[i], result);
            return result;
        }
        
        printf("  ✓ VLAN %d configured with priority %d\n", vlans[i], vlan_config.priority);
    }
    
    return INTEL_HAL_SUCCESS;
}

static int configure_i219_qos_mapping(intel_device_handle_t device, const avb_network_config_t* config)
{
    printf("Configuring I219 QoS priority mapping...\n");
    
    // Configure traffic class mapping for each priority level
    intel_qos_mapping_t qos_map[] = {
        {0, 0},  // Best effort traffic
        {1, 0},  // Background traffic  
        {2, 1},  // Management traffic
        {3, 1},  // General network traffic
        {4, 2},  // Control protocol traffic
        {5, 3},  // AVB Class B traffic
        {6, 3},  // AVB Class A traffic  
        {7, 3}   // Network management traffic
    };
    
    for (int i = 0; i < 8; i++) {
        int result = intel_configure_priority_mapping(device, qos_map[i].priority, qos_map[i].traffic_class);
        if (result != INTEL_HAL_SUCCESS) {
            printf("Failed to configure priority mapping %d->%d: error %d\n", 
                   qos_map[i].priority, qos_map[i].traffic_class, result);
            return result;
        }
        
        printf("  ✓ Priority %d mapped to traffic class %d\n", qos_map[i].priority, qos_map[i].traffic_class);
    }
    
    return INTEL_HAL_SUCCESS;
}

static int configure_i219_credit_based_shaper(intel_device_handle_t device, const avb_network_config_t* config)
{
    printf("Configuring I219 Credit-Based Shaper for AVB...\n");
    
    // Calculate credit parameters based on link speed and bandwidth allocation
    uint32_t link_speed = 100000000; // 100 Mbps for most I219 implementations
    
    // Class A configuration (highest priority AVB traffic)
    intel_cbs_config_t class_a_config = {
        .traffic_class = 3,
        .bandwidth_percent = (config->class_a_bandwidth * 100) / link_speed,
        .idle_slope = config->class_a_bandwidth / 8,  // Convert to bytes per second
        .send_slope = -(int32_t)((link_speed - config->class_a_bandwidth) / 8),
        .hi_credit = 1500 * 8,  // Maximum frame size in bits
        .lo_credit = -1500 * 8  // Negative of maximum frame size
    };
    
    int result = intel_configure_cbs(device, &class_a_config);
    if (result != INTEL_HAL_SUCCESS) {
        printf("Failed to configure Class A CBS: error %d\n", result);
        return result;
    }
    printf("  ✓ AVB Class A: %d%% bandwidth, idle slope %d bytes/sec\n", 
           class_a_config.bandwidth_percent, class_a_config.idle_slope);
    
    // Class B configuration (lower priority AVB traffic)
    intel_cbs_config_t class_b_config = {
        .traffic_class = 2,
        .bandwidth_percent = (config->class_b_bandwidth * 100) / link_speed,
        .idle_slope = config->class_b_bandwidth / 8,
        .send_slope = -(int32_t)((link_speed - config->class_a_bandwidth - config->class_b_bandwidth) / 8),
        .hi_credit = 1500 * 8,
        .lo_credit = -1500 * 8
    };
    
    result = intel_configure_cbs(device, &class_b_config);
    if (result != INTEL_HAL_SUCCESS) {
        printf("Failed to configure Class B CBS: error %d\n", result);
        return result;
    }
    printf("  ✓ AVB Class B: %d%% bandwidth, idle slope %d bytes/sec\n", 
           class_b_config.bandwidth_percent, class_b_config.idle_slope);
    
    return INTEL_HAL_SUCCESS;
}

static int configure_i219_advanced_qos(intel_device_handle_t device, const avb_network_config_t* config)
{
    printf("Configuring I219 advanced QoS features...\n");
    
    // Configure rate limiting for non-AVB traffic classes
    intel_rate_limit_config_t mgmt_limit = {
        .traffic_class = 1,
        .rate_limit_mbps = 10,  // Limit management traffic to 10 Mbps
        .burst_size = 1500      // Allow single frame bursts
    };
    
    int result = intel_configure_rate_limiting(device, &mgmt_limit);
    if (result != INTEL_HAL_SUCCESS) {
        printf("Failed to configure management rate limiting: error %d\n", result);
        return result;
    }
    printf("  ✓ Management traffic limited to %d Mbps\n", mgmt_limit.rate_limit_mbps);
    
    // Configure queue weights for WRR scheduling
    intel_queue_weight_t queue_weights[] = {
        {0, 1},  // Best effort: lowest weight
        {1, 2},  // Management: low weight
        {2, 4},  // Class B: medium weight  
        {3, 8}   // Class A: highest weight
    };
    
    for (int i = 0; i < 4; i++) {
        result = intel_configure_queue_weight(device, queue_weights[i].queue, queue_weights[i].weight);
        if (result != INTEL_HAL_SUCCESS) {
            printf("Failed to configure queue %d weight: error %d\n", queue_weights[i].queue, result);
            return result;
        }
        printf("  ✓ Queue %d configured with weight %d\n", queue_weights[i].queue, queue_weights[i].weight);
    }
    
    return INTEL_HAL_SUCCESS;
}

static int validate_i219_configuration(intel_device_handle_t device, const avb_network_config_t* config)
{
    printf("Validating I219 VLAN/QoS configuration...\n");
    
    // Validate VLAN configuration
    for (int vlan_id = config->management_vlan; vlan_id <= config->control_vlan; vlan_id += 100) {
        if (vlan_id > config->control_vlan) break;
        
        bool vlan_active = false;
        int result = intel_get_vlan_status(device, vlan_id, &vlan_active);
        if (result != INTEL_HAL_SUCCESS || !vlan_active) {
            printf("  ✗ VLAN %d validation failed\n", vlan_id);
            return INTEL_HAL_ERROR;
        }
        printf("  ✓ VLAN %d is active\n", vlan_id);
    }
    
    // Validate QoS configuration
    for (int priority = 0; priority < 8; priority++) {
        uint8_t traffic_class;
        int result = intel_get_priority_mapping(device, priority, &traffic_class);
        if (result != INTEL_HAL_SUCCESS) {
            printf("  ✗ Priority %d mapping validation failed\n", priority);
            return INTEL_HAL_ERROR;
        }
        printf("  ✓ Priority %d -> Traffic Class %d\n", priority, traffic_class);
    }
    
    // Validate CBS configuration
    for (int tc = 2; tc <= 3; tc++) {
        intel_cbs_status_t cbs_status;
        int result = intel_get_cbs_status(device, tc, &cbs_status);
        if (result != INTEL_HAL_SUCCESS || !cbs_status.enabled) {
            printf("  ✗ CBS validation failed for traffic class %d\n", tc);
            return INTEL_HAL_ERROR;
        }
        printf("  ✓ CBS active for TC %d: %d%% bandwidth\n", tc, cbs_status.bandwidth_percent);
    }
    
    return INTEL_HAL_SUCCESS;
}

int main(int argc, char* argv[])
{
    printf("Intel I219 Professional AVB Configuration Example\n");
    printf("==============================================\n\n");
    
    // Use configuration from command line or default
    avb_network_config_t* config = &default_avb_config;
    
    // Initialize Intel HAL
    int result = intel_hal_initialize();
    if (result != INTEL_HAL_SUCCESS) {
        printf("Failed to initialize Intel HAL: error %d\n", result);
        return 1;
    }
    
    // Find I219 device
    intel_device_info_t devices[16];
    int device_count = 16;
    
    result = intel_enumerate_devices(devices, &device_count);
    if (result != INTEL_HAL_SUCCESS || device_count == 0) {
        printf("No Intel devices found\n");
        intel_hal_cleanup();
        return 1;
    }
    
    intel_device_handle_t i219_device = NULL;
    for (int i = 0; i < device_count; i++) {
        if (devices[i].family == INTEL_DEVICE_I219) {
            result = intel_device_open(&devices[i], &i219_device);
            if (result == INTEL_HAL_SUCCESS) {
                printf("Found I219 device: %s (MAC: %02X:%02X:%02X:%02X:%02X:%02X)\n",
                       devices[i].description,
                       devices[i].mac_address[0], devices[i].mac_address[1], devices[i].mac_address[2],
                       devices[i].mac_address[3], devices[i].mac_address[4], devices[i].mac_address[5]);
                break;
            }
        }
    }
    
    if (!i219_device) {
        printf("No I219 device available for configuration\n");
        intel_hal_cleanup();
        return 1;
    }
    
    printf("\nConfiguring professional AVB network settings...\n");
    printf("  Management VLAN: %d (Priority %d)\n", config->management_vlan, config->management_priority);
    printf("  Audio Class A VLAN: %d (Priority %d, %d Mbps)\n", 
           config->audio_class_a_vlan, config->class_a_priority, config->class_a_bandwidth / 1000000);
    printf("  Audio Class B VLAN: %d (Priority %d, %d Mbps)\n", 
           config->audio_class_b_vlan, config->class_b_priority, config->class_b_bandwidth / 1000000);
    printf("  Control VLAN: %d (Priority %d)\n", config->control_vlan, config->control_priority);
    printf("\n");
    
    // Configure VLAN filtering
    result = configure_i219_vlan_filtering(i219_device, config);
    if (result != INTEL_HAL_SUCCESS) {
        printf("VLAN configuration failed\n");
        goto cleanup;
    }
    
    // Configure QoS priority mapping
    result = configure_i219_qos_mapping(i219_device, config);
    if (result != INTEL_HAL_SUCCESS) {
        printf("QoS configuration failed\n");
        goto cleanup;
    }
    
    // Configure Credit-Based Shaper for AVB
    result = configure_i219_credit_based_shaper(i219_device, config);
    if (result != INTEL_HAL_SUCCESS) {
        printf("CBS configuration failed\n");
        goto cleanup;
    }
    
    // Configure advanced QoS features
    result = configure_i219_advanced_qos(i219_device, config);
    if (result != INTEL_HAL_SUCCESS) {
        printf("Advanced QoS configuration failed\n");
        goto cleanup;
    }
    
    // Validate configuration
    result = validate_i219_configuration(i219_device, config);
    if (result != INTEL_HAL_SUCCESS) {
        printf("Configuration validation failed\n");
        goto cleanup;
    }
    
    printf("\n✅ I219 Professional AVB Configuration Complete!\n");
    printf("   The adapter is now configured for professional audio streaming\n");
    printf("   with IEEE 802.1Q VLAN filtering and 802.1p QoS priority mapping.\n");
    printf("   AVB Credit-Based Shaper is active for deterministic audio latency.\n\n");
    
    printf("Next steps:\n");
    printf("  1. Start gPTP daemon for time synchronization\n");
    printf("  2. Configure AVTP pipeline with matching VLAN/priority settings\n");
    printf("  3. Test with AVB-capable audio devices\n");
    
cleanup:
    if (i219_device) {
        intel_device_close(i219_device);
    }
    intel_hal_cleanup();
    
    return (result == INTEL_HAL_SUCCESS) ? 0 : 1;
}
