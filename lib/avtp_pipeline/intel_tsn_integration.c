/******************************************************************************

  Copyright (c) 2025, Intel Corporation
  All rights reserved.

  AVTP Pipeline Intel TSN Integration
  
  This module provides Time-Sensitive Networking (TSN) integration for AVTP
  streaming using Intel hardware acceleration features including:
  - Time-Aware Shaper (TAS) - 802.1Qbv
  - Frame Preemption (FP) - 802.1Qbu  
  - Timed Transmission (LAUNCHTIME)
  - AVB queue bandwidth reservation

******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <stdbool.h>

#include "intel_tsn_integration.h"
// Use unified Intel Ethernet HAL instead of direct intel_avb
#include "../../thirdparty/intel-ethernet-hal/include/intel_ethernet_hal.h"

// Simple logging for Phase 3 (will be enhanced later)
#define TSN_LOG_INFO(msg, ...) printf("[TSN INFO] " msg "\n", ##__VA_ARGS__)
#define TSN_LOG_ERROR(msg, ...) printf("[TSN ERROR] " msg "\n", ##__VA_ARGS__)
#define TSN_LOG_WARNING(msg, ...) printf("[TSN WARNING] " msg "\n", ##__VA_ARGS__)

// AVB streaming constants (IEEE 802.1BA)
#define AVB_CLASS_A_CYCLE_TIME_NS    125000    // 125µs for Class A
#define AVB_CLASS_B_CYCLE_TIME_NS    250000    // 250µs for Class B
#define AVB_PRIORITY_CLASS_A         6         // SR_PVID for Class A
#define AVB_PRIORITY_CLASS_B         5         // SR_PVID for Class B
#define AVB_MIN_FRAGMENT_SIZE        64        // Minimum Ethernet frame size

/**
 * @brief TSN integration context for AVTP pipeline
 */
struct intel_tsn_context {
    intel_device_t *intel_device;     ///< Intel HAL device handle
    bool tas_enabled;                 ///< Time-Aware Shaper enabled
    bool fp_enabled;                  ///< Frame Preemption enabled
    bool timed_tx_enabled;            ///< Timed transmission enabled
    struct tsn_tas_config tas_config; ///< Current TAS configuration
    struct tsn_fp_config fp_config;   ///< Current FP configuration
    uint32_t reserved_bandwidth_bps;  ///< Reserved bandwidth in bits/second
};

static struct intel_tsn_context g_tsn_context = {0};

/**
 * @brief Initialize Intel TSN integration for AVTP pipeline
 */
int intel_tsn_init(const char *interface_name)
{
    int result;
    
    TSN_LOG_INFO("Initializing Intel TSN integration for interface: %s", interface_name);
    
    // Clear context
    memset(&g_tsn_context, 0, sizeof(g_tsn_context));
    
    // Allocate device structure
    g_tsn_context.intel_device = malloc(sizeof(intel_device_t));
    if (!g_tsn_context.intel_device) {
        TSN_LOG_ERROR("Failed to allocate device structure");
        return -ENOMEM;
    }
    
    // Initialize device structure
    memset(g_tsn_context.intel_device, 0, sizeof(intel_device_t));
    
    // Initialize Intel HAL
    intel_hal_result_t hal_result = intel_hal_init();
    if (hal_result != INTEL_HAL_SUCCESS) {
        TSN_LOG_ERROR("Failed to initialize Intel HAL: %d", hal_result);
        free(g_tsn_context.intel_device);
        g_tsn_context.intel_device = NULL;
        return -1;
    }
    
    // Open Intel device by interface name
    hal_result = intel_hal_open_device(interface_name, &g_tsn_context.intel_device);
    if (hal_result != INTEL_HAL_SUCCESS) {
        TSN_LOG_ERROR("Failed to open Intel device %s: %d", interface_name, hal_result);
        free(g_tsn_context.intel_device);
        g_tsn_context.intel_device = NULL;
        intel_hal_cleanup();
        return -1;
    }
    
    // Check TSN capabilities
    uint32_t caps = intel_get_capabilities(g_tsn_context.intel_device);
    
    if (caps & INTEL_CAP_TSN_TAS) {
        TSN_LOG_INFO("Time-Aware Shaper (TAS) capability detected");
    } else {
        TSN_LOG_WARNING("Time-Aware Shaper (TAS) not supported on this hardware");
    }
    
    if (caps & INTEL_CAP_TSN_FP) {
        TSN_LOG_INFO("Frame Preemption (FP) capability detected");
    } else {
        TSN_LOG_WARNING("Frame Preemption (FP) not supported on this hardware");
    }
    
    if (caps & INTEL_CAP_ENHANCED_TS) {
        TSN_LOG_INFO("Enhanced timestamping capability detected");
    }
    
    TSN_LOG_INFO("Intel TSN integration initialized successfully");
    return 0;
}

/**
 * @brief Configure Time-Aware Shaper for AVB streaming
 */
int intel_tsn_configure_tas(intel_avb_stream_class_t stream_class, uint32_t bandwidth_mbps)
{
    int result;
    
    if (!g_tsn_context.intel_device) {
        TSN_LOG_ERROR("TSN integration not initialized");
        return -EINVAL;
    }
    
    // Check TAS capability
    uint32_t caps = intel_get_capabilities(g_tsn_context.intel_device);
    if (!(caps & INTEL_CAP_TSN_TAS)) {
        TSN_LOG_WARNING("TAS not supported on this hardware, skipping configuration");
        return -ENOTSUP;
    }
    
    // Configure TAS based on AVB class
    memset(&g_tsn_context.tas_config, 0, sizeof(g_tsn_context.tas_config));
    
    switch (stream_class) {
    case INTEL_AVB_CLASS_A:
        g_tsn_context.tas_config.cycle_time_ns = AVB_CLASS_A_CYCLE_TIME_NS;
        // Class A gets priority queue 6 - open for AVB traffic
        g_tsn_context.tas_config.gate_states[6] = 1; // Open
        g_tsn_context.tas_config.gate_durations[6] = AVB_CLASS_A_CYCLE_TIME_NS * 75 / 100; // 75% of cycle
        break;
        
    case INTEL_AVB_CLASS_B:
        g_tsn_context.tas_config.cycle_time_ns = AVB_CLASS_B_CYCLE_TIME_NS;
        // Class B gets priority queue 5 - open for AVB traffic
        g_tsn_context.tas_config.gate_states[5] = 1; // Open
        g_tsn_context.tas_config.gate_durations[5] = AVB_CLASS_B_CYCLE_TIME_NS * 75 / 100; // 75% of cycle
        break;
        
    default:
        TSN_LOG_ERROR("Invalid AVB stream class: %d", stream_class);
        return -EINVAL;
    }
    
    TSN_LOG_INFO("Configuring TAS for AVB Class %s, cycle time: %u ns, bandwidth: %u Mbps",
                 (stream_class == INTEL_AVB_CLASS_A) ? "A" : "B",
                 g_tsn_context.tas_config.cycle_time_ns, bandwidth_mbps);
    
    // Apply TAS configuration
    result = intel_hal_setup_time_aware_shaper(g_tsn_context.intel_device, &g_tsn_context.tas_config);
    if (result != 0) {
        TSN_LOG_ERROR("Failed to configure Time-Aware Shaper: %d", result);
        return result;
    }
    
    g_tsn_context.tas_enabled = true;
    g_tsn_context.reserved_bandwidth_bps = bandwidth_mbps * 1000000; // Convert to bps
    
    TSN_LOG_INFO("Time-Aware Shaper configured successfully");
    return 0;
}

/**
 * @brief Configure Frame Preemption for low-latency AVB streams
 */
int intel_tsn_configure_frame_preemption(bool enable_preemption)
{
    int result;
    
    if (!g_tsn_context.intel_device) {
        TSN_LOG_ERROR("TSN integration not initialized");
        return -EINVAL;
    }
    
    // Check FP capability
    uint32_t caps = intel_get_capabilities(g_tsn_context.intel_device);
    if (!(caps & INTEL_CAP_TSN_FP)) {
        TSN_LOG_WARNING("Frame Preemption not supported on this hardware, skipping configuration");
        return -ENOTSUP;
    }
    
    if (!enable_preemption) {
        TSN_LOG_INFO("Frame Preemption disabled by configuration");
        g_tsn_context.fp_enabled = false;
        return 0;
    }
    
    // Configure Frame Preemption
    memset(&g_tsn_context.fp_config, 0, sizeof(g_tsn_context.fp_config));
    
    // Configure preemptable queues (lower priority queues can be preempted)
    // AVB Class A (queue 6) and Class B (queue 5) are non-preemptable
    // Best effort traffic (queues 0-4) are preemptable
    g_tsn_context.fp_config.preemptable_queues = 0x1F; // Queues 0-4 preemptable
    g_tsn_context.fp_config.min_fragment_size = AVB_MIN_FRAGMENT_SIZE;
    
    TSN_LOG_INFO("Configuring Frame Preemption: preemptable_queues=0x%02X, min_fragment=%u",
                 g_tsn_context.fp_config.preemptable_queues,
                 g_tsn_context.fp_config.min_fragment_size);
    
    // Apply FP configuration
    result = intel_hal_setup_frame_preemption(g_tsn_context.intel_device, &g_tsn_context.fp_config);
    if (result != 0) {
        TSN_LOG_ERROR("Failed to configure Frame Preemption: %d", result);
        return result;
    }
    
    g_tsn_context.fp_enabled = true;
    
    TSN_LOG_INFO("Frame Preemption configured successfully");
    return 0;
}

/**
 * @brief Transmit AVTP packet with precise timing (LAUNCHTIME)
 */
int intel_tsn_transmit_timed_packet(void *packet_data, uint32_t packet_len,
                                   intel_avb_stream_class_t stream_class,
                                   uint64_t launch_time_ns)
{
    int result;
    struct intel_packet packet;
    uint8_t queue_index;
    
    if (!g_tsn_context.intel_device) {
        TSN_LOG_ERROR("TSN integration not initialized");
        return -EINVAL;
    }
    
    if (!packet_data || packet_len == 0) {
        TSN_LOG_ERROR("Invalid packet data or length");
        return -EINVAL;
    }
    
    // Determine queue based on AVB class
    switch (stream_class) {
    case INTEL_AVB_CLASS_A:
        queue_index = AVB_PRIORITY_CLASS_A;
        break;
    case INTEL_AVB_CLASS_B:
        queue_index = AVB_PRIORITY_CLASS_B;
        break;
    default:
        TSN_LOG_ERROR("Invalid AVB stream class: %d", stream_class);
        return -EINVAL;
    }
    
    // Prepare Intel packet structure
    memset(&packet, 0, sizeof(packet));
    packet.vaddr = packet_data;
    packet.len = packet_len;
    packet.flags = INTEL_PACKET_LAUNCHTIME; // Enable timed transmission
    packet.attime = launch_time_ns;
    
    TSN_LOG_INFO("Transmitting timed packet: len=%u, queue=%u, launch_time=%llu ns",
                 packet_len, queue_index, (unsigned long long)launch_time_ns);
    
    // Transmit packet
    result = intel_hal_xmit_timed_packet(g_tsn_context.intel_device, &packet);
    if (result != 0) {
        TSN_LOG_ERROR("Failed to transmit timed packet: %d", result);
        return result;
    }
    
    // Log actual transmission time for debugging
    if (packet.dmatime != 0) {
        int64_t timing_error_ns = (int64_t)packet.dmatime - (int64_t)launch_time_ns;
        TSN_LOG_INFO("Packet transmitted: actual_time=%llu ns, timing_error=%lld ns",
                     (unsigned long long)packet.dmatime, (long long)timing_error_ns);
    }
    
    g_tsn_context.timed_tx_enabled = true;
    
    return 0;
}

/**
 * @brief Get TSN integration status and capabilities
 */
int intel_tsn_get_status(intel_tsn_status_t *status)
{
    if (!status) {
        return -EINVAL;
    }
    
    memset(status, 0, sizeof(intel_tsn_status_t));
    
    if (!g_tsn_context.intel_device) {
        status->initialized = false;
        return 0;
    }
    
    status->initialized = true;
    status->tas_enabled = g_tsn_context.tas_enabled;
    status->fp_enabled = g_tsn_context.fp_enabled;
    status->timed_tx_enabled = g_tsn_context.timed_tx_enabled;
    status->reserved_bandwidth_bps = g_tsn_context.reserved_bandwidth_bps;
    
    // Get hardware capabilities
    uint32_t caps = intel_get_capabilities(g_tsn_context.intel_device);
    status->tas_capable = (caps & INTEL_CAP_TSN_TAS) != 0;
    status->fp_capable = (caps & INTEL_CAP_TSN_FP) != 0;
    status->hardware_timestamping = (caps & INTEL_CAP_ENHANCED_TS) != 0;
    
    return 0;
}

/**
 * @brief Cleanup TSN integration resources
 */
void intel_tsn_cleanup(void)
{
    TSN_LOG_INFO("Cleaning up Intel TSN integration");
    
    if (g_tsn_context.intel_device) {
        intel_detach(g_tsn_context.intel_device);
        free(g_tsn_context.intel_device);
        g_tsn_context.intel_device = NULL;
    }
    
    // Reset context
    memset(&g_tsn_context, 0, sizeof(g_tsn_context));
    
    TSN_LOG_INFO("Intel TSN integration cleanup complete");
}
