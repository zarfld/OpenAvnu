/*
 * OpenAvnu Shaper Configuration
 * 
 * Copyright (c) 2024, The OpenAvnu Contributors
 * SPDX-License-Identifier: BSD-3-Clause
 * 
 * Shaper configuration file parsing and management
 */

#include "shaper_common.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int shaper_config_load(const char* config_file, shaper_config_t* config)
{
    if (!config_file || !config) {
        return -1;
    }
    
    // Set defaults
    strncpy(config->interface_name, "eth0", sizeof(config->interface_name) - 1);
    config->interface_name[sizeof(config->interface_name) - 1] = '\0';
    config->intel_tsn_enabled = false;
    config->max_flows = 64;
    config->max_bandwidth_mbps = 1000;
    config->monitoring_interval_ms = 1000;
    
    // Try to open config file
    FILE* file = fopen(config_file, "r");
    if (!file) {
        printf("Config file %s not found, using defaults\n", config_file);
        return 0; // Use defaults
    }
    
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        // Remove newline
        line[strcspn(line, "\n")] = 0;
        
        // Skip comments and empty lines
        if (line[0] == '#' || line[0] == '\0') {
            continue;
        }
        
        // Parse key=value pairs
        char* equals = strchr(line, '=');
        if (!equals) {
            continue;
        }
        
        *equals = '\0';
        char* key = line;
        char* value = equals + 1;
        
        // Process configuration options
        if (strcmp(key, "interface") == 0) {
            strncpy(config->interface_name, value, sizeof(config->interface_name) - 1);
            config->interface_name[sizeof(config->interface_name) - 1] = '\0';
        } else if (strcmp(key, "intel_tsn") == 0) {
            config->intel_tsn_enabled = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
        } else if (strcmp(key, "max_flows") == 0) {
            config->max_flows = (uint32_t)atoi(value);
        } else if (strcmp(key, "max_bandwidth_mbps") == 0) {
            config->max_bandwidth_mbps = (uint32_t)atoi(value);
        } else if (strcmp(key, "monitoring_interval_ms") == 0) {
            config->monitoring_interval_ms = (uint32_t)atoi(value);
        }
    }
    
    fclose(file);
    printf("Loaded shaper configuration from %s\n", config_file);
    return 0;
}

int shaper_config_save(const char* config_file, const shaper_config_t* config)
{
    if (!config_file || !config) {
        return -1;
    }
    
    FILE* file = fopen(config_file, "w");
    if (!file) {
        return -1;
    }
    
    fprintf(file, "# OpenAvnu Shaper Configuration\n");
    fprintf(file, "interface=%s\n", config->interface_name);
    fprintf(file, "intel_tsn=%s\n", config->intel_tsn_enabled ? "true" : "false");
    fprintf(file, "max_flows=%u\n", config->max_flows);
    fprintf(file, "max_bandwidth_mbps=%u\n", config->max_bandwidth_mbps);
    fprintf(file, "monitoring_interval_ms=%u\n", config->monitoring_interval_ms);
    
    fclose(file);
    return 0;
}
