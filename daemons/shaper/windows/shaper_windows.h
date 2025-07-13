/*******************************************************************************
Copyright (c) 2025, Intel Corporation
All rights reserved.

Windows Traffic Shaper Implementation with Intel HAL Integration
Modern Windows traffic shaping using Intel TSN capabilities and QoS APIs
*******************************************************************************/

#ifndef SHAPER_WINDOWS_H
#define SHAPER_WINDOWS_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <qos2.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef OPENAVNU_BUILD_INTEL_HAL
#include "intel_ethernet_hal.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Windows Shaper configuration
typedef struct {
    char interface_name[256];
    uint32_t bandwidth_kbps;
    bool tsn_enabled;
    bool qos_enabled;
    bool intel_hal_available;
} shaper_windows_config_t;

// Traffic class configuration  
typedef struct {
    uint8_t priority;
    uint32_t max_bandwidth_kbps;
    uint32_t burst_size_bytes;
    bool enabled;
    HANDLE qos_flow_handle;
} shaper_traffic_class_t;

// Windows Shaper context
typedef struct {
    bool initialized;
    char interface_name[256];
    HANDLE qos_handle;
    shaper_traffic_class_t traffic_classes[8]; // 8 priority levels
    uint64_t stats_packets_shaped;
    uint64_t stats_bytes_shaped;
    uint64_t stats_packets_dropped;
#ifdef OPENAVNU_BUILD_INTEL_HAL
    intel_device_t *intel_device;
    bool intel_tsn_enabled;
#endif
} shaper_windows_context_t;

// Windows Shaper API
int shaper_windows_init(const shaper_windows_config_t *config);
void shaper_windows_cleanup(void);
bool shaper_windows_is_initialized(void);

// Traffic shaping functions
int shaper_windows_configure_class(uint8_t priority, uint32_t max_bandwidth_kbps, uint32_t burst_size_bytes);
int shaper_windows_enable_class(uint8_t priority, bool enable);
int shaper_windows_apply_configuration(void);

// Intel TSN integration
int shaper_windows_enable_intel_tsn(void);
int shaper_windows_configure_tsn_queues(void);
bool shaper_windows_supports_tsn(void);

// Statistics and monitoring
void shaper_windows_get_statistics(uint64_t *packets_shaped, uint64_t *bytes_shaped, uint64_t *packets_dropped);
void shaper_windows_reset_statistics(void);
int shaper_windows_get_queue_depth(uint8_t priority);

// Windows QoS integration
int shaper_windows_init_qos(void);
void shaper_windows_cleanup_qos(void);
int shaper_windows_create_qos_flow(uint8_t priority, uint32_t bandwidth_kbps);
void shaper_windows_remove_qos_flow(uint8_t priority);

#ifdef __cplusplus
}
#endif

#endif /* SHAPER_WINDOWS_H */
