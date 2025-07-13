/*
 * OpenAvnu Shaper Common Definitions
 * 
 * Copyright (c) 2024, The OpenAvnu Contributors
 * SPDX-License-Identifier: BSD-3-Clause
 * 
 * Common shaper definitions shared across platforms
 */

#ifndef OPENAVNU_SHAPER_COMMON_H
#define OPENAVNU_SHAPER_COMMON_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Cross-platform shaper configuration
typedef struct {
    char interface_name[64];
    bool intel_tsn_enabled;
    uint32_t max_flows;
    uint32_t max_bandwidth_mbps;
    uint32_t monitoring_interval_ms;
} shaper_config_t;

// Traffic shaper flow
typedef struct {
    uint32_t id;
    uint8_t priority;
    uint64_t max_bandwidth_bps;
    uint32_t max_latency_us;
    char source_addr[46];  // IPv6 compatible
    char dest_addr[46];
    uint16_t source_port;
    uint16_t dest_port;
    uint8_t protocol;
    bool active;
} shaper_flow_t;

// Traffic class definition
typedef struct {
    uint32_t id;
    uint8_t priority;
    uint32_t max_bandwidth_percent;
    uint32_t max_latency_us;
    char name[32];
} traffic_class_t;

// Shaper statistics
typedef struct {
    uint64_t start_time;
    uint64_t uptime_ms;
    uint32_t active_flows;
    uint32_t flows_configured;
    uint32_t flows_removed;
    uint64_t bytes_shaped;
    uint64_t packets_shaped;
    uint32_t hardware_accelerated_flows;
    uint32_t hardware_queues_used;
    bool intel_tsn_active;
    uint32_t errors;
} shaper_stats_t;

// Common shaper API
int shaper_common_init(const shaper_config_t* config);
int shaper_common_start(void);
void shaper_common_stop(void);
void shaper_common_shutdown(void);

// Flow management
int shaper_common_add_flow(const shaper_flow_t* flow);
int shaper_common_remove_flow(uint32_t flow_id);

// Statistics
int shaper_common_get_stats(shaper_stats_t* stats);

#ifdef __cplusplus
}

// C++ namespace for modern implementation
namespace openavnu {
namespace shaper {

// Type aliases for C++ code
using ShaperConfig = shaper_config_t;
using ShaperFlow = shaper_flow_t;
using TrafficClass = traffic_class_t;
using ShaperStats = shaper_stats_t;

} // namespace shaper
} // namespace openavnu

#endif // __cplusplus

#endif // OPENAVNU_SHAPER_COMMON_H
