/**
 * OpenAvnu Unified AVDECC Configuration Parser
 * 
 * This provides configuration file support for the unified AVDECC API,
 * allowing users to specify implementation preferences and requirements.
 */

#include "openavb_unified_avdecc.h"
#include "openavb_log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define AVB_LOG_COMPONENT "Unified-AVDECC-Config"

// =============================================================================
// Configuration File Parsing
// =============================================================================

/**
 * Parse implementation type from string
 */
static openavb_avdecc_impl_type_t parse_implementation_type(const char *str) {
    if (!str) {
        return OPENAVB_AVDECC_IMPL_AUTO;
    }
    
    if (strcasecmp(str, "auto") == 0 || strcasecmp(str, "auto-select") == 0) {
        return OPENAVB_AVDECC_IMPL_AUTO;
    } else if (strcasecmp(str, "la-avdecc") == 0 || strcasecmp(str, "l-acoustics") == 0) {
        return OPENAVB_AVDECC_IMPL_LA_AVDECC;
    } else if (strcasecmp(str, "legacy") == 0 || strcasecmp(str, "avdecc-lib") == 0) {
        return OPENAVB_AVDECC_IMPL_LEGACY;
    } else if (strcasecmp(str, "simulation") == 0 || strcasecmp(str, "sim") == 0) {
        return OPENAVB_AVDECC_IMPL_SIMULATION;
    }
    
    return OPENAVB_AVDECC_IMPL_AUTO;
}

/**
 * Parse boolean from string
 */
static bool parse_boolean(const char *str) {
    if (!str) {
        return false;
    }
    
    return (strcasecmp(str, "true") == 0 || 
            strcasecmp(str, "yes") == 0 || 
            strcasecmp(str, "on") == 0 || 
            strcasecmp(str, "1") == 0);
}

/**
 * Trim whitespace from string
 */
static char* trim_whitespace(char *str) {
    char *end;
    
    // Trim leading space
    while (isspace((unsigned char)*str)) str++;
    
    if (*str == 0) return str;
    
    // Trim trailing space
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    
    end[1] = '\0';
    
    return str;
}

bool openavb_unified_avdecc_load_config(
    openavb_unified_avdecc_config_t *config,
    const char *config_file_path) {
    
    if (!config || !config_file_path) {
        AVB_LOG_ERROR("Invalid parameters for config loading");
        return false;
    }
    
    // Start with default configuration
    openavb_unified_avdecc_get_default_config(config);
    
    FILE *file = fopen(config_file_path, "r");
    if (!file) {
        AVB_LOG_INFO("Config file not found: %s (using defaults)", config_file_path);
        return true;  // Not an error, just use defaults
    }
    
    AVB_LOG_INFO("Loading unified AVDECC configuration from: %s", config_file_path);
    
    char line[256];
    int line_number = 0;
    bool in_avdecc_section = false;
    
    while (fgets(line, sizeof(line), file)) {
        line_number++;
        
        // Remove newline
        line[strcspn(line, "\r\n")] = 0;
        
        // Trim whitespace
        char *trimmed = trim_whitespace(line);
        
        // Skip empty lines and comments
        if (*trimmed == '\0' || *trimmed == '#' || *trimmed == ';') {
            continue;
        }
        
        // Check for section headers
        if (*trimmed == '[' && trimmed[strlen(trimmed)-1] == ']') {
            trimmed[strlen(trimmed)-1] = '\0';  // Remove closing bracket
            char *section_name = trim_whitespace(trimmed + 1);  // Remove opening bracket
            
            in_avdecc_section = (strcasecmp(section_name, "unified_avdecc") == 0 ||
                                strcasecmp(section_name, "avdecc") == 0);
            continue;
        }
        
        // Skip if not in AVDECC section
        if (!in_avdecc_section) {
            continue;
        }
        
        // Parse key=value pairs
        char *equals = strchr(trimmed, '=');
        if (!equals) {
            AVB_LOG_WARNING("Invalid line %d in config file: %s", line_number, trimmed);
            continue;
        }
        
        *equals = '\0';
        char *key = trim_whitespace(trimmed);
        char *value = trim_whitespace(equals + 1);
        
        // Remove quotes from value if present
        if ((*value == '"' && value[strlen(value)-1] == '"') ||
            (*value == '\'' && value[strlen(value)-1] == '\'')) {
            value[strlen(value)-1] = '\0';
            value++;
        }
        
        // Parse configuration parameters
        if (strcasecmp(key, "implementation") == 0 || strcasecmp(key, "preferred_implementation") == 0) {
            config->preferred_implementation = parse_implementation_type(value);
            AVB_LOG_INFO("  Implementation: %s", openavb_unified_avdecc_impl_type_to_string(config->preferred_implementation));
        }
        else if (strcasecmp(key, "milan_required") == 0 || strcasecmp(key, "milan_mode_required") == 0) {
            config->milan_mode_required = parse_boolean(value);
            AVB_LOG_INFO("  MILAN Required: %s", config->milan_mode_required ? "Yes" : "No");
        }
        else if (strcasecmp(key, "fast_connect_required") == 0 || strcasecmp(key, "fast_connect") == 0) {
            config->fast_connect_required = parse_boolean(value);
            AVB_LOG_INFO("  Fast Connect Required: %s", config->fast_connect_required ? "Yes" : "No");
        }
        else if (strcasecmp(key, "network_redundancy_required") == 0 || strcasecmp(key, "redundancy") == 0) {
            config->network_redundancy_required = parse_boolean(value);
            AVB_LOG_INFO("  Network Redundancy Required: %s", config->network_redundancy_required ? "Yes" : "No");
        }
        else if (strcasecmp(key, "discovery_timeout_ms") == 0 || strcasecmp(key, "discovery_timeout") == 0) {
            config->discovery_timeout_ms = (uint32_t)atoi(value);
            AVB_LOG_INFO("  Discovery Timeout: %u ms", config->discovery_timeout_ms);
        }
        else if (strcasecmp(key, "entity_name") == 0) {
            strncpy(config->entity_name, value, sizeof(config->entity_name) - 1);
            config->entity_name[sizeof(config->entity_name) - 1] = '\0';
            AVB_LOG_INFO("  Entity Name: %s", config->entity_name);
        }
        else if (strcasecmp(key, "interface_name") == 0 || strcasecmp(key, "interface") == 0) {
            strncpy(config->interface_name, value, sizeof(config->interface_name) - 1);
            config->interface_name[sizeof(config->interface_name) - 1] = '\0';
            AVB_LOG_INFO("  Interface: %s", config->interface_name);
        }
        else if (strcasecmp(key, "enable_logging") == 0 || strcasecmp(key, "logging") == 0) {
            config->enable_logging = parse_boolean(value);
            AVB_LOG_INFO("  Logging: %s", config->enable_logging ? "Enabled" : "Disabled");
        }
        else if (strcasecmp(key, "max_entities") == 0) {
            config->max_entities = (uint32_t)atoi(value);
            AVB_LOG_INFO("  Max Entities: %u", config->max_entities);
        }
        else {
            AVB_LOG_WARNING("Unknown configuration parameter: %s", key);
        }
    }
    
    fclose(file);
    
    // Validate loaded configuration
    if (!openavb_unified_avdecc_validate_config(config)) {
        AVB_LOG_ERROR("Configuration validation failed");
        return false;
    }
    
    AVB_LOG_INFO("✅ Configuration loaded successfully from %s", config_file_path);
    return true;
}

bool openavb_unified_avdecc_save_config(
    const openavb_unified_avdecc_config_t *config,
    const char *config_file_path) {
    
    if (!config || !config_file_path) {
        AVB_LOG_ERROR("Invalid parameters for config saving");
        return false;
    }
    
    FILE *file = fopen(config_file_path, "w");
    if (!file) {
        AVB_LOG_ERROR("Failed to open config file for writing: %s", config_file_path);
        return false;
    }
    
    AVB_LOG_INFO("Saving unified AVDECC configuration to: %s", config_file_path);
    
    // Write header
    fprintf(file, "# OpenAvnu Unified AVDECC Configuration\n");
    fprintf(file, "# Generated automatically - modify as needed\n");
    fprintf(file, "\n");
    
    // Write unified_avdecc section
    fprintf(file, "[unified_avdecc]\n");
    fprintf(file, "\n");
    
    // Implementation selection
    fprintf(file, "# AVDECC Implementation Selection\n");
    fprintf(file, "# Options: auto, la-avdecc, legacy, simulation\n");
    const char *impl_str = "auto";
    switch (config->preferred_implementation) {
        case OPENAVB_AVDECC_IMPL_LA_AVDECC: impl_str = "la-avdecc"; break;
        case OPENAVB_AVDECC_IMPL_LEGACY: impl_str = "legacy"; break;
        case OPENAVB_AVDECC_IMPL_SIMULATION: impl_str = "simulation"; break;
        default: impl_str = "auto"; break;
    }
    fprintf(file, "implementation = %s\n", impl_str);
    fprintf(file, "\n");
    
    // Requirements
    fprintf(file, "# Feature Requirements\n");
    fprintf(file, "milan_required = %s\n", config->milan_mode_required ? "true" : "false");
    fprintf(file, "fast_connect_required = %s\n", config->fast_connect_required ? "true" : "false");
    fprintf(file, "network_redundancy_required = %s\n", config->network_redundancy_required ? "true" : "false");
    fprintf(file, "\n");
    
    // Network settings
    fprintf(file, "# Network Settings\n");
    fprintf(file, "discovery_timeout_ms = %u\n", config->discovery_timeout_ms);
    fprintf(file, "entity_name = \"%s\"\n", config->entity_name);
    fprintf(file, "interface_name = \"%s\"\n", config->interface_name);
    fprintf(file, "max_entities = %u\n", config->max_entities);
    fprintf(file, "\n");
    
    // Logging
    fprintf(file, "# Logging\n");
    fprintf(file, "enable_logging = %s\n", config->enable_logging ? "true" : "false");
    fprintf(file, "\n");
    
    // Write example configurations as comments
    fprintf(file, "# Example Configurations:\n");
    fprintf(file, "#\n");
    fprintf(file, "# For MILAN devices:\n");
    fprintf(file, "#   implementation = la-avdecc\n");
    fprintf(file, "#   milan_required = true\n");
    fprintf(file, "#   fast_connect_required = true\n");
    fprintf(file, "#\n");
    fprintf(file, "# For legacy compatibility:\n");
    fprintf(file, "#   implementation = legacy\n");
    fprintf(file, "#   milan_required = false\n");
    fprintf(file, "#\n");
    fprintf(file, "# For testing without hardware:\n");
    fprintf(file, "#   implementation = simulation\n");
    
    fclose(file);
    
    AVB_LOG_INFO("✅ Configuration saved successfully to %s", config_file_path);
    return true;
}
