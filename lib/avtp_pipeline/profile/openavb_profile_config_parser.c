/**
 * OpenAvnu Profile Framework - Configuration Parser
 * 
 * This file implements INI file parsing for profile selection and
 * configuration validation against profile capabilities.
 */

#include "openavb_profile_framework.h"
#include "openavb_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define AVB_LOG_COMPONENT "ProfileConfig"

// Configuration parsing state
typedef struct config_parser_state {
    char current_section[64];
    openavb_profile_cfg_t *selected_profile;
    bool profile_auto_select;
    struct {
        bool required_fast_connect;
        bool required_security;
        U32 min_timing_precision_ns;
        U32 max_streams_needed;
    } requirements;
} config_parser_state_t;

/**
 * Trim whitespace from string
 */
static char* trim_whitespace(char* str) {
    char* end;
    
    // Trim leading space
    while (isspace((unsigned char)*str)) str++;
    
    if (*str == 0) return str;
    
    // Trim trailing space
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    
    end[1] = '\0';
    return str;
}

/**
 * Parse profile selection from configuration
 */
static bool parse_profile_selection(config_parser_state_t *state, const char *name, const char *value) {
    if (strcmp(name, "profile_name") == 0) {
        state->selected_profile = openavb_profile_get_by_name(value, NULL);
        if (!state->selected_profile) {
            AVB_LOG_ERROR("Profile '%s' not found", value);
            return false;
        }
        AVB_LOG_INFO("Selected profile: %s", value);
        return true;
    }
    
    if (strcmp(name, "profile_version") == 0) {
        // This would be used with profile_name to select specific version
        AVB_LOG_DEBUG("Profile version specified: %s", value);
        return true;
    }
    
    if (strcmp(name, "profile_auto_select") == 0) {
        state->profile_auto_select = (strcmp(value, "true") == 0);
        AVB_LOG_INFO("Profile auto-selection: %s", state->profile_auto_select ? "enabled" : "disabled");
        return true;
    }
    
    if (strcmp(name, "required_fast_connect") == 0) {
        state->requirements.required_fast_connect = (strcmp(value, "true") == 0);
        return true;
    }
    
    if (strcmp(name, "required_security") == 0) {
        state->requirements.required_security = (strcmp(value, "true") == 0);
        return true;
    }
    
    if (strcmp(name, "min_timing_precision_ns") == 0) {
        state->requirements.min_timing_precision_ns = strtoul(value, NULL, 10);
        return true;
    }
    
    if (strcmp(name, "max_streams_needed") == 0) {
        state->requirements.max_streams_needed = strtoul(value, NULL, 10);
        return true;
    }
    
    return false;
}

/**
 * Auto-select profile based on requirements
 */
static openavb_profile_cfg_t* auto_select_profile(const config_parser_state_t *state) {
    const openavb_profile_cfg_t* profiles[64];
    U32 profile_count = openavb_profile_list_all(profiles, 64);
    
    openavb_profile_cfg_t *best_match = NULL;
    int best_score = -1;
    
    for (U32 i = 0; i < profile_count; i++) {
        const openavb_profile_cfg_t *profile = profiles[i];
        int score = 0;
        bool disqualified = false;
        
        // Check fast connect requirement
        if (state->requirements.required_fast_connect) {
            if (profile->capabilities.transport.fast_connect_supported) {
                score += 10;
            } else {
                disqualified = true;
            }
        }
        
        // Check security requirement
        if (state->requirements.required_security) {
            if (profile->capabilities.security.authentication_required) {
                score += 10;
            } else {
                disqualified = true;
            }
        }
        
        // Check timing precision requirement
        if (state->requirements.min_timing_precision_ns > 0) {
            if (profile->capabilities.timing.sync_uncertainty_tolerance_ns <= state->requirements.min_timing_precision_ns) {
                score += 5;
            } else {
                disqualified = true;
            }
        }
        
        // Check stream capacity requirement
        if (state->requirements.max_streams_needed > 0) {
            if (profile->capabilities.transport.max_streams_per_entity >= state->requirements.max_streams_needed) {
                score += 5;
            } else {
                disqualified = true;
            }
        }
        
        // Prefer more capable profiles (higher spec versions)
        if (profile->spec_version >= OPENAVB_SPEC_MILAN_1_0) {
            score += 3;
        }
        
        if (!disqualified && score > best_score) {
            best_score = score;
            best_match = (openavb_profile_cfg_t*)profile;
        }
    }
    
    if (best_match) {
        AVB_LOG_INFO("Auto-selected profile: %s (score: %d)", best_match->profile_name, best_score);
    } else {
        AVB_LOG_WARNING("No profile matches requirements, falling back to AVB");
        best_match = openavb_profile_get_by_name("AVB", NULL);
    }
    
    return best_match;
}

/**
 * Validate stream configuration against profile
 */
static bool validate_stream_config(const openavb_profile_cfg_t *profile, 
                                  const char *name, const char *value) {
    if (!profile) return true; // No profile selected yet
    
    if (strcmp(name, "presentation_offset_ns") == 0) {
        U32 offset = strtoul(value, NULL, 10);
        if (offset < profile->capabilities.timing.min_presentation_offset_ns ||
            offset > profile->capabilities.timing.max_presentation_offset_ns) {
            AVB_LOG_ERROR("Presentation offset %u ns violates profile %s limits (%u-%u ns)",
                         offset, profile->profile_name,
                         profile->capabilities.timing.min_presentation_offset_ns,
                         profile->capabilities.timing.max_presentation_offset_ns);
            return false;
        }
    }
    
    if (strcmp(name, "security_enabled") == 0) {
        bool enabled = (strcmp(value, "true") == 0);
        if (profile->capabilities.security.authentication_required && !enabled) {
            AVB_LOG_ERROR("Profile %s requires security but security_enabled=false", 
                         profile->profile_name);
            return false;
        }
    }
    
    if (strcmp(name, "use_avtp_timestamps") == 0) {
        bool enabled = (strcmp(value, "true") == 0);
        if (profile->capabilities.transport.avtp_timestamp_required && !enabled) {
            AVB_LOG_ERROR("Profile %s requires AVTP timestamps but use_avtp_timestamps=false", 
                         profile->profile_name);
            return false;
        }
    }
    
    return true;
}

/**
 * Parse a single configuration line
 */
static bool parse_config_line(config_parser_state_t *state, const char *line) {
    char *line_copy = strdup(line);
    char *trimmed = trim_whitespace(line_copy);
    
    // Skip empty lines and comments
    if (strlen(trimmed) == 0 || trimmed[0] == '#' || trimmed[0] == ';') {
        free(line_copy);
        return true;
    }
    
    // Check for section header
    if (trimmed[0] == '[') {
        char *end = strchr(trimmed, ']');
        if (end) {
            *end = '\0';
            strncpy(state->current_section, trimmed + 1, sizeof(state->current_section) - 1);
            state->current_section[sizeof(state->current_section) - 1] = '\0';
            AVB_LOG_DEBUG("Entering section: %s", state->current_section);
        }
        free(line_copy);
        return true;
    }
    
    // Parse key=value pair
    char *equals = strchr(trimmed, '=');
    if (!equals) {
        AVB_LOG_WARNING("Invalid configuration line: %s", trimmed);
        free(line_copy);
        return false;
    }
    
    *equals = '\0';
    char *key = trim_whitespace(trimmed);
    char *value = trim_whitespace(equals + 1);
    
    bool result = true;
    if (strcmp(state->current_section, "profile") == 0) {
        result = parse_profile_selection(state, key, value);
    } else if (strcmp(state->current_section, "stream") == 0) {
        result = validate_stream_config(state->selected_profile, key, value);
    }
    
    free(line_copy);
    return result;
}

/**
 * Parse configuration file and return selected profile
 */
bool openavb_profile_parse_config(const char *config_file, openavb_profile_cfg_t **selected_profile) {
    if (!config_file || !selected_profile) {
        AVB_LOG_ERROR("Invalid parameters to openavb_profile_parse_config");
        return false;
    }
    
    FILE *file = fopen(config_file, "r");
    if (!file) {
        AVB_LOG_ERROR("Cannot open configuration file: %s", config_file);
        return false;
    }
    
    config_parser_state_t state = {0};
    strcpy(state.current_section, "");
    
    char line[512];
    int line_number = 0;
    bool success = true;
    
    while (fgets(line, sizeof(line), file)) {
        line_number++;
        
        // Remove newline
        char *newline = strchr(line, '\n');
        if (newline) *newline = '\0';
        
        if (!parse_config_line(&state, line)) {
            AVB_LOG_ERROR("Configuration error at line %d: %s", line_number, line);
            success = false;
            break;
        }
    }
    
    fclose(file);
    
    if (!success) {
        return false;
    }
    
    // Determine final profile selection
    if (state.profile_auto_select) {
        *selected_profile = auto_select_profile(&state);
    } else if (state.selected_profile) {
        *selected_profile = state.selected_profile;
    } else {
        AVB_LOG_WARNING("No profile specified, using default AVB profile");
        *selected_profile = openavb_profile_get_by_name("AVB", NULL);
    }
    
    if (*selected_profile) {
        AVB_LOG_INFO("Final profile selection: %s (%s)", 
                    (*selected_profile)->profile_name, 
                    (*selected_profile)->version_string);
        return true;
    } else {
        AVB_LOG_ERROR("Failed to select any profile");
        return false;
    }
}

/**
 * Validate entire configuration file against profile
 */
bool openavb_profile_validate_config_file(const char *config_file, 
                                         const openavb_profile_cfg_t *profile) {
    if (!config_file || !profile) {
        return false;
    }
    
    FILE *file = fopen(config_file, "r");
    if (!file) {
        return false;
    }
    
    config_parser_state_t state = {0};
    state.selected_profile = (openavb_profile_cfg_t*)profile;
    strcpy(state.current_section, "");
    
    char line[512];
    int line_number = 0;
    bool success = true;
    
    while (fgets(line, sizeof(line), file)) {
        line_number++;
        
        char *newline = strchr(line, '\n');
        if (newline) *newline = '\0';
        
        if (!parse_config_line(&state, line)) {
            AVB_LOG_ERROR("Validation error at line %d: %s", line_number, line);
            success = false;
        }
    }
    
    fclose(file);
    return success;
}
