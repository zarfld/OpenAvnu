/**
 * OpenAvnu Profile Framework - Core Implementation
 * 
 * This file implements the core profile framework functionality including
 * profile registration, management, and capability-based decision making.
 */

#include "openavb_profile_framework.h"
#include "openavb_log.h"
#include "openavb_types_pub.h"
#include <stdlib.h>
#include <string.h>

#define AVB_LOG_COMPONENT "ProfileFramework"

// Maximum number of registered profiles
#define MAX_PROFILES 64

// Profile registry
static openavb_profile_cfg_t* g_profile_registry[MAX_PROFILES];
static U32 g_profile_count = 0;
static bool g_framework_initialized = false;

/**
 * Initialize the profile framework
 */
bool openavb_profile_framework_init(void)
{
    if (g_framework_initialized) {
        AVB_LOG_INFO("Profile framework already initialized");
        return true;
    }
    
    // Clear registry
    memset(g_profile_registry, 0, sizeof(g_profile_registry));
    g_profile_count = 0;
    
    // Register built-in profiles
    extern openavb_profile_cfg_t builtin_profile_avb;
    extern openavb_profile_cfg_t builtin_profile_milan_1_0;
    extern openavb_profile_cfg_t builtin_profile_milan_1_1;
    extern openavb_profile_cfg_t builtin_profile_automotive;
    
    if (!openavb_profile_register(&builtin_profile_avb) ||
        !openavb_profile_register(&builtin_profile_milan_1_0) ||
        !openavb_profile_register(&builtin_profile_milan_1_1) ||
        !openavb_profile_register(&builtin_profile_automotive)) {
        AVB_LOG_ERROR("Failed to register built-in profiles");
        return false;
    }
    
    // Register L-Acoustics AVDECC profiles (if available)
    extern bool openavb_profile_register_la_avdecc_profiles(void);
    if (!openavb_profile_register_la_avdecc_profiles()) {
        AVB_LOG_WARNING("L-Acoustics AVDECC profiles registration failed or not available");
        // Continue anyway - this is optional
    }
    
    g_framework_initialized = true;
    AVB_LOG_INFO("Profile framework initialized with %d profiles", g_profile_count);
    return true;
}

/**
 * Register a profile with the framework
 */
bool openavb_profile_register(openavb_profile_cfg_t *profile)
{
    if (!profile) {
        AVB_LOG_ERROR("Cannot register null profile");
        return false;
    }
    
    if (g_profile_count >= MAX_PROFILES) {
        AVB_LOG_ERROR("Profile registry full, cannot register %s", profile->profile_name);
        return false;
    }
    
    // Check for duplicate profile IDs
    for (U32 i = 0; i < g_profile_count; i++) {
        if (g_profile_registry[i]->profile_id == profile->profile_id) {
            AVB_LOG_ERROR("Profile ID %d already registered", profile->profile_id);
            return false;
        }
    }
    
    g_profile_registry[g_profile_count++] = profile;
    AVB_LOG_INFO("Registered profile: %s (%s)", profile->profile_name, profile->version_string);
    return true;
}

/**
 * Get profile by name and version
 */
openavb_profile_cfg_t* openavb_profile_get_by_name(const char *name, const char *version)
{
    if (!name) return NULL;
    
    for (U32 i = 0; i < g_profile_count; i++) {
        if (strcmp(g_profile_registry[i]->profile_name, name) == 0) {
            if (!version || strcmp(g_profile_registry[i]->version_string, version) == 0) {
                return g_profile_registry[i];
            }
        }
    }
    
    return NULL;
}

/**
 * Get profile by specification version and variant
 */
openavb_profile_cfg_t* openavb_profile_get_by_version(openavb_spec_version_e version, 
                                                      openavb_spec_variant_e variant)
{
    for (U32 i = 0; i < g_profile_count; i++) {
        if (g_profile_registry[i]->spec_version == version && 
            g_profile_registry[i]->spec_variant == variant) {
            return g_profile_registry[i];
        }
    }
    
    return NULL;
}

/**
 * Check if two profiles are compatible
 */
bool openavb_profile_is_compatible(const openavb_profile_cfg_t *profile1,
                                   const openavb_profile_cfg_t *profile2)
{
    if (!profile1 || !profile2) return false;
    
    // Security compatibility check
    bool sec1_required = profile1->capabilities.security.authentication_required;
    bool sec2_required = profile2->capabilities.security.authentication_required;
    
    if (sec1_required && !sec2_required) {
        // Check if profile2 can support optional security
        if (!profile2->capabilities.security.supported_cipher_suites) {
            return false;
        }
    }
    
    // Check for common cipher suites
    if (sec1_required || sec2_required) {
        U32 common_ciphers = profile1->capabilities.security.supported_cipher_suites & 
                            profile2->capabilities.security.supported_cipher_suites;
        if (!common_ciphers) {
            return false;
        }
    }
    
    // Timing compatibility - check for range overlap
    U32 min1 = profile1->capabilities.timing.min_presentation_offset_ns;
    U32 max1 = profile1->capabilities.timing.max_presentation_offset_ns;
    U32 min2 = profile2->capabilities.timing.min_presentation_offset_ns;
    U32 max2 = profile2->capabilities.timing.max_presentation_offset_ns;
    
    if (min1 > max2 || min2 > max1) {
        return false; // No timing overlap
    }
    
    // QoS compatibility
    if (profile1->capabilities.qos.max_frame_size < profile2->capabilities.qos.max_frame_size) {
        if (profile1->capabilities.qos.max_frame_size < 1522) { // Minimum Ethernet frame
            return false;
        }
    }
    
    return true;
}

/**
 * Validate stream configuration against profile
 */
bool openavb_stream_validate_config(const openavb_profile_cfg_t *profile, const void *config)
{
    if (!profile || !config) return false;
    
    // This would be implemented based on your stream_config_t structure
    // For now, return true as a placeholder
    AVB_LOG_DEBUG("Stream validation for profile %s", profile->profile_name);
    return true;
}

/**
 * Get profile capability matrix
 */
const openavb_capability_matrix_t* openavb_profile_get_capabilities(const openavb_profile_cfg_t *profile)
{
    if (!profile) return NULL;
    return &profile->capabilities;
}

/**
 * List all registered profiles
 */
U32 openavb_profile_list_all(const openavb_profile_cfg_t** profiles, U32 max_count)
{
    if (!profiles || max_count == 0) return 0;
    
    U32 count = (g_profile_count < max_count) ? g_profile_count : max_count;
    for (U32 i = 0; i < count; i++) {
        profiles[i] = g_profile_registry[i];
    }
    
    return count;
}

/**
 * Profile framework cleanup
 */
void openavb_profile_framework_cleanup(void)
{
    if (!g_framework_initialized) return;
    
    // Call cleanup callbacks for all profiles
    for (U32 i = 0; i < g_profile_count; i++) {
        if (g_profile_registry[i]->callbacks.profile_cleanup) {
            g_profile_registry[i]->callbacks.profile_cleanup(g_profile_registry[i], NULL);
        }
    }
    
    memset(g_profile_registry, 0, sizeof(g_profile_registry));
    g_profile_count = 0;
    g_framework_initialized = false;
    
    AVB_LOG_INFO("Profile framework cleanup complete");
}

/**
 * Alias for backward compatibility
 * Some code expects openavb_profile_framework_initialize() instead of openavb_profile_framework_init()
 */
bool openavb_profile_framework_initialize(void)
{
    return openavb_profile_framework_init();
}

/**
 * Cleanup alias for backward compatibility  
 */
void openavb_profile_framework_cleanup(void)
{
    openavb_profile_framework_deinit();
}
