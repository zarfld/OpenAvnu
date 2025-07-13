/*
 * OpenAvnu Shaper Common Implementation
 * 
 * Copyright (c) 2024, The OpenAvnu Contributors
 * SPDX-License-Identifier: BSD-3-Clause
 * 
 * Cross-platform shaper common functions
 */

#include "shaper_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global shaper state
static bool g_shaper_initialized = false;
static shaper_config_t g_shaper_config = {0};
static shaper_stats_t g_shaper_stats = {0};

int shaper_common_init(const shaper_config_t* config)
{
    if (g_shaper_initialized) {
        return 0; // Already initialized
    }
    
    if (!config) {
        return -1;
    }
    
    // Copy configuration
    memcpy(&g_shaper_config, config, sizeof(shaper_config_t));
    
    // Initialize statistics
    memset(&g_shaper_stats, 0, sizeof(shaper_stats_t));
    g_shaper_stats.start_time = 0; // Platform-specific implementation needed
    
    g_shaper_initialized = true;
    
    printf("Shaper common initialized for interface: %s\n", config->interface_name);
    if (config->intel_tsn_enabled) {
        printf("Intel TSN enabled\n");
    }
    
    return 0;
}

int shaper_common_start(void)
{
    if (!g_shaper_initialized) {
        return -1;
    }
    
    printf("Shaper common started\n");
    return 0;
}

void shaper_common_stop(void)
{
    if (!g_shaper_initialized) {
        return;
    }
    
    printf("Shaper common stopped\n");
}

void shaper_common_shutdown(void)
{
    if (!g_shaper_initialized) {
        return;
    }
    
    g_shaper_initialized = false;
    memset(&g_shaper_config, 0, sizeof(shaper_config_t));
    memset(&g_shaper_stats, 0, sizeof(shaper_stats_t));
    
    printf("Shaper common shutdown complete\n");
}

int shaper_common_add_flow(const shaper_flow_t* flow)
{
    if (!g_shaper_initialized || !flow) {
        return -1;
    }
    
    printf("Adding flow ID: %u, Priority: %u, Bandwidth: %llu bps\n", 
           flow->id, flow->priority, (unsigned long long)flow->max_bandwidth_bps);
    
    g_shaper_stats.flows_configured++;
    g_shaper_stats.active_flows++;
    
    return 0;
}

int shaper_common_remove_flow(uint32_t flow_id)
{
    if (!g_shaper_initialized) {
        return -1;
    }
    
    printf("Removing flow ID: %u\n", flow_id);
    
    if (g_shaper_stats.active_flows > 0) {
        g_shaper_stats.active_flows--;
    }
    g_shaper_stats.flows_removed++;
    
    return 0;
}

int shaper_common_get_stats(shaper_stats_t* stats)
{
    if (!g_shaper_initialized || !stats) {
        return -1;
    }
    
    memcpy(stats, &g_shaper_stats, sizeof(shaper_stats_t));
    return 0;
}
