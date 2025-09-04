#include "intel_tsn_integration.h"
#include <stdio.h>
#include <string.h>
#include <memory.h>

/**
 * Internal TSN context structure
 */
typedef struct {
    bool initialized;
    intel_device_t *intel_device;          /**< Intel device handle */
    intel_device_info_t device_info;       /**< Device information */
    uint32_t capabilities;                 /**< Device capabilities */
    
    /* Current TSN State */
    bool tas_enabled;                      /**< Time-Aware Shaper enabled */
    bool fp_enabled;                       /**< Frame Preemption enabled */
    bool timed_tx_enabled;                 /**< Timed transmission enabled */
    uint64_t reserved_bandwidth_bps;       /**< Reserved bandwidth in bps */
} intel_tsn_context_t;

/* Global context */
static intel_tsn_context_t g_tsn_context = {0};

/**
 * Initialize Intel TSN integration
 */
int intel_tsn_init(const char* interface_name)
{
    intel_hal_result_t hal_result;
    
    printf("Initializing Intel TSN integration for interface: %s\n", interface_name ? interface_name : "(auto-detect)");
    
    if (g_tsn_context.initialized) {
        printf("TSN integration already initialized\n");
        return 0;
    }
    
    // Initialize Intel HAL
    hal_result = intel_hal_init();
    if (hal_result != INTEL_HAL_SUCCESS) {
        printf("Failed to initialize Intel HAL: %d\n", hal_result);
        return -EIO;
    }
    
    // Open Intel device (first available if interface_name is NULL)
    hal_result = intel_hal_open_device(interface_name, &g_tsn_context.intel_device);
    if (hal_result != INTEL_HAL_SUCCESS) {
        printf("Failed to open Intel device: %d (%s)\n", hal_result, intel_hal_get_last_error());
        intel_hal_cleanup();
        return -ENODEV;
    }
    
    // Get device information
    hal_result = intel_hal_get_device_info(g_tsn_context.intel_device, &g_tsn_context.device_info);
    if (hal_result != INTEL_HAL_SUCCESS) {
        printf("Failed to get device info: %d (%s)\n", hal_result, intel_hal_get_last_error());
        intel_hal_close_device(g_tsn_context.intel_device);
        intel_hal_cleanup();
        return -EIO;
    }
    
    // Get device capabilities
    hal_result = intel_hal_get_capabilities(g_tsn_context.intel_device, &g_tsn_context.capabilities);
    if (hal_result != INTEL_HAL_SUCCESS) {
        printf("Failed to get device capabilities: %d (%s)\n", hal_result, intel_hal_get_last_error());
        intel_hal_close_device(g_tsn_context.intel_device);
        intel_hal_cleanup();
        return -EIO;
    }
    
    g_tsn_context.initialized = true;
    
    printf("TSN integration initialized successfully\n");
    if (g_tsn_context.intel_device) {
        printf("Device: %s (Family: %u)\n", g_tsn_context.device_info.device_name, g_tsn_context.device_info.family);
    } else {
        printf("Device: %s (Simulation)\n", g_tsn_context.device_info.device_name);
    }
    printf("Capabilities: 0x%08X\n", g_tsn_context.capabilities);
    
    // Log TSN capabilities
    if (g_tsn_context.capabilities & INTEL_CAP_TSN_TAS) {
        printf("  - Time-Aware Shaper (TAS) supported\n");
    }
    if (g_tsn_context.capabilities & INTEL_CAP_TSN_FP) {
        printf("  - Frame Preemption supported\n");
    }
    if (g_tsn_context.capabilities & INTEL_CAP_BASIC_1588) {
        printf("  - Hardware timestamping supported\n");
    }
    
    return 0;
}

/**
 * Get TSN status and capabilities
 */
int intel_tsn_get_status(intel_tsn_status_t* status)
{
    if (!status) {
        return -EINVAL;
    }
    
    if (!g_tsn_context.initialized) {
        memset(status, 0, sizeof(*status));
        return -ENODEV;
    }
    
    // Fill status structure with real hardware information
    strncpy(status->interface_name, g_tsn_context.device_info.device_name, sizeof(status->interface_name) - 1);
    status->interface_name[sizeof(status->interface_name) - 1] = '\0';
    
    snprintf(status->device_id, sizeof(status->device_id), "%04X:%04X", 
             g_tsn_context.device_info.vendor_id, g_tsn_context.device_info.device_id);
             
    status->device_family = g_tsn_context.device_info.family;
    
    strncpy(status->driver_version, g_tsn_context.device_info.description, sizeof(status->driver_version) - 1);
    status->driver_version[sizeof(status->driver_version) - 1] = '\0';
    
    // Map capabilities to boolean flags
    status->tas_capable = (g_tsn_context.capabilities & INTEL_CAP_TSN_TAS) != 0;
    status->fp_capable = (g_tsn_context.capabilities & INTEL_CAP_TSN_FP) != 0;
    status->hardware_timestamping = (g_tsn_context.capabilities & INTEL_CAP_BASIC_1588) != 0;
    
    // Current state
    status->tas_enabled = g_tsn_context.tas_enabled;
    status->fp_enabled = g_tsn_context.fp_enabled;
    status->timed_tx_enabled = g_tsn_context.timed_tx_enabled;
    status->reserved_bandwidth_bps = g_tsn_context.reserved_bandwidth_bps;
    
    return 0;
}

/**
 * Configure Time-Aware Shaper for specific stream class
 */
int intel_tsn_configure_tas(intel_avb_stream_class_t stream_class, int bandwidth_mbps)
{
    intel_hal_result_t hal_result;
    intel_tas_config_t tas_config;
    
    printf("Configuring TAS for stream class %s with %d Mbps\n", 
           (stream_class == INTEL_AVB_CLASS_A) ? "A" : "B", bandwidth_mbps);
    
    if (!g_tsn_context.initialized) {
        printf("TSN integration not initialized\n");
        return -EINVAL;
    }
    
    if (!g_tsn_context.intel_device) {
        printf("No Intel device available for TAS configuration\n");
        return -ENODEV;
    }
    
    // Check if hardware supports TAS
    if (!(g_tsn_context.capabilities & INTEL_CAP_TSN_TAS)) {
        printf("TAS not supported on this hardware (Family: %d)\n", g_tsn_context.device_info.family);
        return -ENOTSUP;
    }
    
    // Configure TAS based on stream class and bandwidth requirements
    memset(&tas_config, 0, sizeof(tas_config));
    
    // Basic TAS configuration for AVB streams
    tas_config.cycle_time = 1000000; // 1ms cycle time (1000000 ns)
    tas_config.base_time = 0; // Start immediately
    tas_config.gate_control_list_length = 2;
    
    // Calculate time slots based on bandwidth
    uint32_t reserved_time_ns = (bandwidth_mbps * 1000 * 8) / 1000; // Convert Mbps to ns per cycle
    uint32_t remaining_time_ns = (uint32_t)(tas_config.cycle_time - reserved_time_ns);
    
    // Gate control entry 0: AVB stream time slot
    tas_config.gate_control_list[0].gate_states = (1 << stream_class); // Open gate for stream class
    tas_config.gate_control_list[0].time_interval = reserved_time_ns;
    
    // Gate control entry 1: Best effort traffic time slot  
    tas_config.gate_control_list[1].gate_states = 0xFF & ~(1 << stream_class); // Open all other gates
    tas_config.gate_control_list[1].time_interval = remaining_time_ns;
    
    printf("Programming TAS with:\n");
    printf("  Cycle Time: %llu ns\n", tas_config.cycle_time);
    printf("  Stream Class %s Queue: %d ns reserved per cycle\n", 
           (stream_class == INTEL_AVB_CLASS_A) ? "A" : "B", reserved_time_ns);
    printf("  Best Effort Queues: %d ns per cycle\n", remaining_time_ns);
    
    // Configure TAS on hardware using Intel HAL
    hal_result = intel_hal_setup_time_aware_shaper(g_tsn_context.intel_device, &tas_config);
    if (hal_result != INTEL_HAL_SUCCESS) {
        printf("Failed to configure TAS: %d (%s)\n", hal_result, intel_hal_get_last_error());
        return -EIO;
    }
    
    g_tsn_context.tas_enabled = true;
    g_tsn_context.reserved_bandwidth_bps = (uint64_t)bandwidth_mbps * 1000000;
    
    printf("TAS configuration applied successfully to hardware\n");
    return 0;
}

/**
 * Configure Frame Preemption
 */
int intel_tsn_configure_frame_preemption(bool enable)
{
    intel_hal_result_t hal_result;
    intel_frame_preemption_config_t fp_config;
    
    printf("Configuring Frame Preemption: %s\n", enable ? "enabled" : "disabled");
    
    if (!g_tsn_context.initialized) {
        printf("TSN integration not initialized\n");
        return -EINVAL;
    }
    
    if (!g_tsn_context.intel_device) {
        printf("No Intel device available for Frame Preemption configuration\n");
        return -ENODEV;
    }
    
    // Check if hardware supports Frame Preemption
    if (!(g_tsn_context.capabilities & INTEL_CAP_TSN_FP)) {
        printf("Frame Preemption not supported on this hardware (Family: %d)\n", g_tsn_context.device_info.family);
        return -ENOTSUP;
    }
    
    // Configure Frame Preemption parameters
    memset(&fp_config, 0, sizeof(fp_config));
    
    if (enable) {
        // Make all queues preemptible except the highest priority (Class A)
        fp_config.preemptible_queues = 0xFE; // All queues except queue 7 (highest)
        fp_config.additional_fragment_size = 64; // 64 byte minimum fragment size
        fp_config.verify_disable = false; // Enable verification
        fp_config.verify_time = 10000; // 10ms verification time
    } else {
        // Disable preemption - all queues express
        fp_config.preemptible_queues = 0x00;
        fp_config.additional_fragment_size = 0;
        fp_config.verify_disable = true;
        fp_config.verify_time = 0;
    }
    
    printf("Programming Frame Preemption with:\n");
    printf("  Enable: %s\n", enable ? "true" : "false");
    printf("  Preemptible Queues: 0x%02X\n", fp_config.preemptible_queues);
    printf("  Fragment Size: %d bytes\n", fp_config.additional_fragment_size);
    printf("  Verification: %s\n", fp_config.verify_disable ? "disabled" : "enabled");
    
    // Configure Frame Preemption on hardware using Intel HAL
    hal_result = intel_hal_setup_frame_preemption(g_tsn_context.intel_device, &fp_config);
    if (hal_result != INTEL_HAL_SUCCESS) {
        printf("Failed to configure Frame Preemption: %d (%s)\n", hal_result, intel_hal_get_last_error());
        return -EIO;
    }
    
    g_tsn_context.fp_enabled = enable;
    
    printf("Frame Preemption configuration applied successfully to hardware\n");
    return 0;
}

/**
 * Configure timed transmission for specific stream class
 */
int intel_tsn_configure_timed_tx(intel_avb_stream_class_t stream_class, uint64_t tx_time_us)
{
    printf("Configuring timed transmission for stream class %s at time %llu us\n",
           (stream_class == INTEL_AVB_CLASS_A) ? "A" : "B", tx_time_us);
    
    if (!g_tsn_context.initialized) {
        printf("TSN integration not initialized\n");
        return -EINVAL;
    }
    
    if (!g_tsn_context.intel_device) {
        printf("No Intel device available for timed transmission\n");
        return -ENODEV;
    }
    
    // Check if hardware supports timed transmission (1588/PTP)
    if (!(g_tsn_context.capabilities & INTEL_CAP_BASIC_1588)) {
        printf("Timed transmission not supported on this hardware (Family: %d)\n", g_tsn_context.device_info.family);
        return -ENOTSUP;
    }
    
    // Enable timestamping on the hardware
    intel_hal_result_t hal_result = intel_hal_enable_timestamping(g_tsn_context.intel_device, true);
    if (hal_result != INTEL_HAL_SUCCESS) {
        printf("Failed to enable timestamping: %d (%s)\n", hal_result, intel_hal_get_last_error());
        return -EIO;
    }
    
    g_tsn_context.timed_tx_enabled = true;
    
    printf("Timed transmission enabled on hardware\n");
    printf("Use intel_tsn_transmit_timed_packet() for actual packet transmission\n");
    return 0;
}

/**
 * Transmit a packet with precise timing (LAUNCHTIME)
 */
int intel_tsn_transmit_timed_packet(intel_avb_stream_class_t stream_class, 
                                   const void* packet_data, size_t packet_length, 
                                   uint64_t launch_time_ns)
{
    intel_hal_result_t hal_result;
    intel_timed_packet_t timed_packet;
    
    if (!g_tsn_context.initialized) {
        printf("TSN integration not initialized\n");
        return -EINVAL;
    }
    
    if (!g_tsn_context.intel_device) {
        printf("No Intel device available for timed packet transmission\n");
        return -ENODEV;
    }
    
    // Check if hardware supports timed transmission
    if (!(g_tsn_context.capabilities & INTEL_CAP_BASIC_1588)) {
        printf("Timed transmission not supported on this hardware\n");
        return -ENOTSUP;
    }
    
    if (!g_tsn_context.timed_tx_enabled) {
        printf("Timed transmission not configured. Call intel_tsn_configure_timed_tx() first\n");
        return -EINVAL;
    }
    
    // Prepare timed packet structure
    memset(&timed_packet, 0, sizeof(timed_packet));
    timed_packet.packet_data = (void*)packet_data;
    timed_packet.packet_length = packet_length;
    timed_packet.launch_time = launch_time_ns;
    timed_packet.queue = (stream_class == INTEL_AVB_CLASS_A) ? INTEL_AVB_CLASS_A : INTEL_AVB_CLASS_B;
    
    printf("Transmitting timed packet:\n");
    printf("  Stream Class: %s (Queue %d)\n", 
           (stream_class == INTEL_AVB_CLASS_A) ? "A" : "B", timed_packet.queue);
    printf("  Packet Size: %zu bytes\n", packet_length);
    printf("  Launch Time: %llu ns\n", launch_time_ns);
    
    // Transmit packet using Intel HAL
    hal_result = intel_hal_xmit_timed_packet(g_tsn_context.intel_device, &timed_packet);
    if (hal_result != INTEL_HAL_SUCCESS) {
        printf("Failed to transmit timed packet: %d (%s)\n", hal_result, intel_hal_get_last_error());
        return -EIO;
    }
    
    printf("Timed packet transmitted successfully via hardware\n");
    return 0;
}

/**
 * Get TSN capabilities of the Intel adapter
 */
uint32_t intel_tsn_get_capabilities(void)
{
    if (!g_tsn_context.initialized) {
        return 0;
    }
    
    return g_tsn_context.capabilities;
}

/**
 * Cleanup Intel TSN integration
 */
void intel_tsn_cleanup(void)
{
    if (!g_tsn_context.initialized) {
        return;
    }
    
    printf("Cleaning up Intel TSN integration\n");
    
    if (g_tsn_context.intel_device) {
        intel_hal_close_device(g_tsn_context.intel_device);
        g_tsn_context.intel_device = NULL;
    }
    
    intel_hal_cleanup();
    
    memset(&g_tsn_context, 0, sizeof(g_tsn_context));
    
    printf("Intel TSN integration cleaned up\n");
}
