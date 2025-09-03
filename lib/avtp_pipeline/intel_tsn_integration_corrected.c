/******************************************************************************

  Copyright (c) 2025, Intel Corporation
  All rights reserved.

  AVTP Pipeline Intel TSN Integration - CORRECTED VERSION
  
  This module provides Intel Time-Sensitive Networking (TSN) integration
  using the proper Intel HAL API functions.

******************************************************************************/

#include "intel_tsn_integration.h"
#include "../../thirdparty/intel-ethernet-hal/include/intel_ethernet_hal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Windows compatibility
#ifdef _WIN32
#include <time.h>
#include <windows.h>

typedef struct timespec {
    time_t tv_sec;
    long tv_nsec;
} timespec;

static int clock_gettime(int clk_id, struct timespec *tp) {
    LARGE_INTEGER frequency, counter;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&counter);
    
    tp->tv_sec = counter.QuadPart / frequency.QuadPart;
    tp->tv_nsec = ((counter.QuadPart % frequency.QuadPart) * 1000000000L) / frequency.QuadPart;
    return 0;
}

#define CLOCK_REALTIME 0
#else
#include <time.h>
#endif

// Logging macros
#define LOG_INFO(fmt, ...) printf("[TSN INFO] " fmt "\n", ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) printf("[TSN ERROR] " fmt "\n", ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) printf("[TSN DEBUG] " fmt "\n", ##__VA_ARGS__)

// AVB timing constants
#define AVB_CLASS_A_CYCLE_TIME_NS    125000     // 125 microseconds
#define AVB_CLASS_B_CYCLE_TIME_NS    250000     // 250 microseconds

/**
 * @brief TSN context structure using Intel HAL types
 */
typedef struct {
    intel_device_t *hal_device;           // Intel HAL device handle
    intel_tas_config_t tas_config;        // Time-Aware Shaper configuration
    intel_frame_preemption_config_t fp_config; // Frame Preemption configuration
    intel_tsn_status_t status;             // Current TSN status
    bool initialized;                      // Initialization status
} intel_tsn_context_t;

static intel_tsn_context_t g_tsn_context = {0};

/**
 * @brief Initialize Intel TSN integration with Intel HAL
 */
intel_tsn_result_t intel_tsn_init(const char *interface_name)
{
    LOG_INFO("Initializing Intel TSN integration for interface: %s", interface_name);
    
    // Clear context
    memset(&g_tsn_context, 0, sizeof(g_tsn_context));
    
    // Initialize Intel HAL
    intel_hal_result_t hal_result = intel_hal_init();
    if (hal_result != INTEL_HAL_SUCCESS) {
        LOG_ERROR("Failed to initialize Intel HAL: %d", hal_result);
        return INTEL_TSN_ERROR_INIT_FAILED;
    }
    
    // Open the Intel device
    hal_result = intel_hal_open_device(interface_name, &g_tsn_context.hal_device);
    if (hal_result != INTEL_HAL_SUCCESS || !g_tsn_context.hal_device) {
        LOG_ERROR("Failed to open Intel device: %s", interface_name);
        intel_hal_cleanup();
        return INTEL_TSN_ERROR_DEVICE_NOT_FOUND;
    }
    
    // Check device capabilities
    uint32_t capabilities;
    hal_result = intel_hal_get_capabilities(g_tsn_context.hal_device, &capabilities);
    if (hal_result != INTEL_HAL_SUCCESS) {
        LOG_ERROR("Failed to get device capabilities");
        intel_hal_close_device(g_tsn_context.hal_device);
        intel_hal_cleanup();
        return INTEL_TSN_ERROR_INIT_FAILED;
    }
    
    // Initialize status based on capabilities
    g_tsn_context.status.initialized = true;
    g_tsn_context.status.tas_capable = intel_hal_has_capability(g_tsn_context.hal_device, INTEL_CAP_TSN_TIME_AWARE_SHAPER);
    g_tsn_context.status.fp_capable = intel_hal_has_capability(g_tsn_context.hal_device, INTEL_CAP_TSN_FRAME_PREEMPTION);
    g_tsn_context.status.hardware_timestamping = intel_hal_has_capability(g_tsn_context.hal_device, INTEL_CAP_BASIC_IEEE1588);
    
    LOG_INFO("TSN Capabilities detected:");
    LOG_INFO("  Time-Aware Shaper: %s", g_tsn_context.status.tas_capable ? "YES" : "NO");
    LOG_INFO("  Frame Preemption: %s", g_tsn_context.status.fp_capable ? "YES" : "NO");
    LOG_INFO("  Hardware Timestamping: %s", g_tsn_context.status.hardware_timestamping ? "YES" : "NO");
    
    g_tsn_context.initialized = true;
    return INTEL_TSN_SUCCESS;
}

/**
 * @brief Configure Time-Aware Shaper for AVB stream class
 */
intel_tsn_result_t intel_tsn_configure_tas(intel_avb_stream_class_t stream_class)
{
    if (!g_tsn_context.initialized || !g_tsn_context.hal_device) {
        LOG_ERROR("TSN not initialized");
        return INTEL_TSN_ERROR_NOT_INITIALIZED;
    }
    
    LOG_INFO("Configuring Time-Aware Shaper for %s", 
             stream_class == INTEL_AVB_CLASS_A ? "Class A" : "Class B");
    
    // Configure TAS based on stream class
    memset(&g_tsn_context.tas_config, 0, sizeof(g_tsn_context.tas_config));
    
    if (stream_class == INTEL_AVB_CLASS_A) {
        g_tsn_context.tas_config.cycle_time = AVB_CLASS_A_CYCLE_TIME_NS;
        g_tsn_context.tas_config.gate_control_list_length = 2;
        
        // Gate 0: Class A traffic (queue 6) - 75% of cycle
        g_tsn_context.tas_config.gate_control_list[0].gate_states = 0x40; // Queue 6 open
        g_tsn_context.tas_config.gate_control_list[0].time_interval = AVB_CLASS_A_CYCLE_TIME_NS * 75 / 100;
        
        // Gate 1: Other traffic - remaining 25%
        g_tsn_context.tas_config.gate_control_list[1].gate_states = 0x3F; // Queues 0-5 open
        g_tsn_context.tas_config.gate_control_list[1].time_interval = AVB_CLASS_A_CYCLE_TIME_NS * 25 / 100;
        
    } else { // Class B
        g_tsn_context.tas_config.cycle_time = AVB_CLASS_B_CYCLE_TIME_NS;
        g_tsn_context.tas_config.gate_control_list_length = 3;
        
        // Gate 0: Class A traffic (queue 6) - 30% of cycle  
        g_tsn_context.tas_config.gate_control_list[0].gate_states = 0x40; // Queue 6 open
        g_tsn_context.tas_config.gate_control_list[0].time_interval = AVB_CLASS_B_CYCLE_TIME_NS * 30 / 100;
        
        // Gate 1: Class B traffic (queue 5) - 45% of cycle
        g_tsn_context.tas_config.gate_control_list[1].gate_states = 0x20; // Queue 5 open  
        g_tsn_context.tas_config.gate_control_list[1].time_interval = AVB_CLASS_B_CYCLE_TIME_NS * 45 / 100;
        
        // Gate 2: Other traffic - remaining 25%
        g_tsn_context.tas_config.gate_control_list[2].gate_states = 0x1F; // Queues 0-4 open
        g_tsn_context.tas_config.gate_control_list[2].time_interval = AVB_CLASS_B_CYCLE_TIME_NS * 25 / 100;
    }
    
    // Set base time to current time + 1ms
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    g_tsn_context.tas_config.base_time = (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec + 1000000ULL;
    
    // Apply TAS configuration to hardware
    intel_hal_result_t result = intel_hal_setup_time_aware_shaper(g_tsn_context.hal_device, &g_tsn_context.tas_config);
    
    if (result == INTEL_HAL_SUCCESS) {
        g_tsn_context.status.tas_enabled = true;
        LOG_INFO("Time-Aware Shaper configured successfully");
        return INTEL_TSN_SUCCESS;
    } else {
        LOG_ERROR("Failed to configure Time-Aware Shaper: %d", result);
        return INTEL_TSN_ERROR_CONFIGURATION_FAILED;
    }
}

/**
 * @brief Configure Frame Preemption for express traffic
 */
intel_tsn_result_t intel_tsn_configure_frame_preemption(void)
{
    if (!g_tsn_context.initialized || !g_tsn_context.hal_device) {
        LOG_ERROR("TSN not initialized");
        return INTEL_TSN_ERROR_NOT_INITIALIZED;
    }
    
    if (!g_tsn_context.status.fp_capable) {
        LOG_INFO("Frame Preemption not supported on this device - skipping");
        return INTEL_TSN_SUCCESS;
    }
    
    LOG_INFO("Configuring Frame Preemption");
    
    // Configure frame preemption
    memset(&g_tsn_context.fp_config, 0, sizeof(g_tsn_context.fp_config));
    
    // Make lower priority queues (0-4) preemptible, keep AVB queues (5-6) express
    g_tsn_context.fp_config.preemptible_queues = 0x1F; // Queues 0-4
    g_tsn_context.fp_config.additional_fragment_size = 64; // Minimum fragment size
    g_tsn_context.fp_config.verify_disable = false; // Enable verification
    g_tsn_context.fp_config.verify_time = 10000; // 10ms verification time
    
    // Apply frame preemption configuration to hardware
    intel_hal_result_t result = intel_hal_setup_frame_preemption(g_tsn_context.hal_device, &g_tsn_context.fp_config);
    
    if (result == INTEL_HAL_SUCCESS) {
        g_tsn_context.status.fp_enabled = true;
        LOG_INFO("Frame Preemption configured successfully");
        return INTEL_TSN_SUCCESS;
    } else {
        LOG_ERROR("Failed to configure Frame Preemption: %d", result);
        return INTEL_TSN_ERROR_CONFIGURATION_FAILED;
    }
}

/**
 * @brief Transmit packet with precise timing
 */
intel_tsn_result_t intel_tsn_transmit_timed_packet(const void *packet_data, size_t packet_length, 
                                                   uint64_t launch_time_ns, intel_avb_stream_class_t stream_class)
{
    if (!g_tsn_context.initialized || !g_tsn_context.hal_device) {
        LOG_ERROR("TSN not initialized");
        return INTEL_TSN_ERROR_NOT_INITIALIZED;
    }
    
    if (!packet_data || packet_length == 0) {
        LOG_ERROR("Invalid packet data");
        return INTEL_TSN_ERROR_INVALID_PARAMETER;
    }
    
    LOG_DEBUG("Transmitting timed packet: %zu bytes, launch time: %llu ns", 
              packet_length, (unsigned long long)launch_time_ns);
    
    // Create timed packet structure
    intel_timed_packet_t timed_packet = {0};
    timed_packet.packet_data = (void *)packet_data;
    timed_packet.packet_length = packet_length;
    timed_packet.launch_time = launch_time_ns;
    timed_packet.queue = (stream_class == INTEL_AVB_CLASS_A) ? 6 : 5; // AVB queue assignment
    
    // Transmit via Intel HAL
    intel_hal_result_t result = intel_hal_xmit_timed_packet(g_tsn_context.hal_device, &timed_packet);
    
    if (result == INTEL_HAL_SUCCESS) {
        LOG_DEBUG("Timed packet transmitted successfully");
        return INTEL_TSN_SUCCESS;
    } else {
        LOG_ERROR("Failed to transmit timed packet: %d", result);
        return INTEL_TSN_ERROR_TRANSMISSION_FAILED;
    }
}

/**
 * @brief Get current TSN status and capabilities
 */
intel_tsn_result_t intel_tsn_get_status(intel_tsn_status_t *status)
{
    if (!status) {
        return INTEL_TSN_ERROR_INVALID_PARAMETER;
    }
    
    if (!g_tsn_context.initialized) {
        memset(status, 0, sizeof(intel_tsn_status_t));
        return INTEL_TSN_ERROR_NOT_INITIALIZED;
    }
    
    // Update runtime status
    if (g_tsn_context.status.tas_capable) {
        bool tas_enabled;
        uint64_t current_time;
        intel_hal_get_tas_status(g_tsn_context.hal_device, &tas_enabled, &current_time);
        g_tsn_context.status.tas_enabled = tas_enabled;
    }
    
    if (g_tsn_context.status.fp_capable) {
        bool fp_enabled;
        uint8_t active_queues;
        intel_hal_get_frame_preemption_status(g_tsn_context.hal_device, &fp_enabled, &active_queues);
        g_tsn_context.status.fp_enabled = fp_enabled;
    }
    
    // Copy status
    *status = g_tsn_context.status;
    
    return INTEL_TSN_SUCCESS;
}

/**
 * @brief Cleanup TSN integration
 */
intel_tsn_result_t intel_tsn_cleanup(void)
{
    LOG_INFO("Cleaning up Intel TSN integration");
    
    if (g_tsn_context.hal_device) {
        intel_hal_close_device(g_tsn_context.hal_device);
        g_tsn_context.hal_device = NULL;
    }
    
    intel_hal_cleanup();
    
    memset(&g_tsn_context, 0, sizeof(g_tsn_context));
    
    LOG_INFO("TSN integration cleanup complete");
    return INTEL_TSN_SUCCESS;
}
