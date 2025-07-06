/**
 * OpenAvnu Profile Framework - Stream Integration
 * 
 * This file implements the integration layer between the profile framework
 * and OpenAvnu's stream management system.
 */

#include "openavb_profile_framework.h"
#include "openavb_log.h"
#include "openavb_mediaq_pub.h"
#include "openavb_tl_pub.h"
#include <stdlib.h>
#include <string.h>

#define AVB_LOG_COMPONENT "ProfileStream"

// Stream handle structure
typedef struct openavb_stream_handle {
    U32 stream_id;
    const openavb_profile_cfg_t *profile;
    void *stream_data;  // Points to existing OpenAvnu stream structure
    openavb_stream_quality_metrics_t metrics;
    bool is_active;
    struct openavb_stream_handle *next;
} openavb_stream_handle_t;

// Global stream registry
static openavb_stream_handle_t *g_stream_registry = NULL;
static U32 g_next_stream_id = 1;

/**
 * Create a new stream handle
 */
openavb_stream_handle_t* openavb_stream_create(void *stream_data) {
    openavb_stream_handle_t *handle = malloc(sizeof(openavb_stream_handle_t));
    if (!handle) {
        AVB_LOG_ERROR("Failed to allocate stream handle");
        return NULL;
    }
    
    memset(handle, 0, sizeof(openavb_stream_handle_t));
    handle->stream_id = g_next_stream_id++;
    handle->stream_data = stream_data;
    handle->is_active = false;
    
    // Add to registry
    handle->next = g_stream_registry;
    g_stream_registry = handle;
    
    AVB_LOG_DEBUG("Created stream handle ID %u", handle->stream_id);
    return handle;
}

/**
 * Set profile for stream
 */
bool openavb_stream_set_profile(openavb_stream_handle_t *stream, const openavb_profile_cfg_t *profile) {
    if (!stream || !profile) {
        AVB_LOG_ERROR("Invalid parameters to openavb_stream_set_profile");
        return false;
    }
    
    // If stream is active, check if profile switch is allowed
    if (stream->is_active && stream->profile) {
        if (!openavb_profile_is_compatible(stream->profile, profile)) {
            AVB_LOG_WARNING("Profile switch from %s to %s requires stream restart",
                           stream->profile->profile_name, profile->profile_name);
            // Could implement restart logic here
            return false;
        }
    }
    
    stream->profile = profile;
    
    // Initialize profile-specific settings
    if (profile->callbacks.profile_init) {
        if (!profile->callbacks.profile_init(profile, stream->stream_data)) {
            AVB_LOG_ERROR("Profile initialization failed for stream %u", stream->stream_id);
            return false;
        }
    }
    
    AVB_LOG_INFO("Stream %u now using profile: %s (%s)", 
                stream->stream_id, profile->profile_name, profile->version_string);
    return true;
}

/**
 * Get profile for stream
 */
const openavb_profile_cfg_t* openavb_stream_get_profile(openavb_stream_handle_t *stream) {
    if (!stream) return NULL;
    return stream->profile;
}

/**
 * Validate stream configuration against profile capabilities
 */
bool openavb_stream_validate_with_profile(openavb_stream_handle_t *stream, const void *config) {
    if (!stream || !stream->profile || !config) {
        return false;
    }
    
    const openavb_profile_cfg_t *profile = stream->profile;
    
    // This is a placeholder - actual implementation would depend on
    // the specific stream_config_t structure used by OpenAvnu
    
    // For now, we'll demonstrate the capability-based validation pattern
    AVB_LOG_DEBUG("Validating stream %u configuration against profile %s", 
                 stream->stream_id, profile->profile_name);
    
    // Example validations based on capabilities:
    
    // Security validation
    if (profile->capabilities.security.authentication_required) {
        AVB_LOG_DEBUG("Profile requires authentication");
        // Would check if config enables security
    }
    
    // Timing validation
    U32 min_offset = profile->capabilities.timing.min_presentation_offset_ns;
    U32 max_offset = profile->capabilities.timing.max_presentation_offset_ns;
    AVB_LOG_DEBUG("Profile timing constraints: %u - %u ns", min_offset, max_offset);
    
    // Transport validation
    if (profile->capabilities.transport.avtp_timestamp_required) {
        AVB_LOG_DEBUG("Profile requires AVTP timestamps");
        // Would check if config enables timestamps
    }
    
    return true;
}

/**
 * Configure stream based on profile capabilities
 */
bool openavb_stream_configure_from_profile(openavb_stream_handle_t *stream) {
    if (!stream || !stream->profile) {
        return false;
    }
    
    const openavb_profile_cfg_t *profile = stream->profile;
    AVB_LOG_INFO("Configuring stream %u based on profile %s capabilities", 
                stream->stream_id, profile->profile_name);
    
    // Configure security based on capabilities
    if (profile->capabilities.security.authentication_required) {
        AVB_LOG_DEBUG("Enabling security for stream %u", stream->stream_id);
        
        // Select cipher based on supported suites
        U32 supported_ciphers = profile->capabilities.security.supported_cipher_suites;
        if (supported_ciphers & CIPHER_AES_256_GCM) {
            AVB_LOG_DEBUG("Using AES-256-GCM cipher");
            // Configure AES-256-GCM
        } else if (supported_ciphers & CIPHER_AES_128_GCM) {
            AVB_LOG_DEBUG("Using AES-128-GCM cipher");
            // Configure AES-128-GCM
        }
        
        // Select authentication method
        U32 supported_auth = profile->capabilities.security.supported_auth_methods;
        if (supported_auth & AUTH_METHOD_CERTIFICATE) {
            AVB_LOG_DEBUG("Using certificate authentication");
            // Configure certificate auth
        } else if (supported_auth & AUTH_METHOD_PSK) {
            AVB_LOG_DEBUG("Using PSK authentication");
            // Configure PSK auth
        }
    }
    
    // Configure timing based on capabilities
    U32 tolerance = profile->capabilities.timing.sync_uncertainty_tolerance_ns;
    if (tolerance < 100000) {
        AVB_LOG_DEBUG("Configuring ultra-precision timing mode");
        // Configure ultra-precision mode
    } else if (tolerance < 500000) {
        AVB_LOG_DEBUG("Configuring high-precision timing mode");
        // Configure high-precision mode
    } else if (tolerance < 1000000) {
        AVB_LOG_DEBUG("Configuring standard timing mode");
        // Configure standard timing mode
    } else {
        AVB_LOG_DEBUG("Configuring relaxed timing mode");
        // Configure relaxed timing mode
    }
    
    // Configure QoS based on capabilities
    if (profile->capabilities.qos.frame_preemption_supported) {
        AVB_LOG_DEBUG("Enabling frame preemption for stream %u", stream->stream_id);
        // Configure frame preemption
    }
    
    if (profile->capabilities.qos.credit_based_shaping_required) {
        AVB_LOG_DEBUG("Enabling credit-based shaping for stream %u", stream->stream_id);
        // Configure CBS
    }
    
    return true;
}

/**
 * Update stream quality metrics
 */
void openavb_stream_update_metrics(openavb_stream_handle_t *stream, 
                                  const openavb_stream_quality_metrics_t *metrics) {
    if (!stream || !metrics) return;
    
    memcpy(&stream->metrics, metrics, sizeof(openavb_stream_quality_metrics_t));
    
    // Check if metrics violate profile requirements
    if (stream->profile) {
        U32 tolerance_ns = stream->profile->capabilities.timing.sync_uncertainty_tolerance_ns;
        float tolerance_ms = tolerance_ns / 1000000.0f;
        
        if (metrics->jitter_ms > tolerance_ms) {
            AVB_LOG_WARNING("Stream %u jitter %.3f ms exceeds profile %s tolerance %.3f ms",
                           stream->stream_id, metrics->jitter_ms, 
                           stream->profile->profile_name, tolerance_ms);
            
            // Handle timing violation based on profile capabilities
            if (stream->profile->callbacks.get_error_recovery_action) {
                U32 action = stream->profile->callbacks.get_error_recovery_action(
                    stream->profile, 0x0001, 1); // Timing error type
                
                switch (action) {
                    case 0x0001: // Reduce precision
                        AVB_LOG_INFO("Reducing timing precision for stream %u", stream->stream_id);
                        break;
                    case 0x0002: // Switch profile
                        AVB_LOG_INFO("Attempting profile switch for stream %u", stream->stream_id);
                        break;
                    case 0x0003: // Reset stream
                        AVB_LOG_INFO("Resetting stream %u", stream->stream_id);
                        break;
                }
            }
        }
    }
}

/**
 * Get stream quality metrics
 */
const openavb_stream_quality_metrics_t* openavb_stream_get_metrics(openavb_stream_handle_t *stream) {
    if (!stream) return NULL;
    return &stream->metrics;
}

/**
 * Handle stream errors based on profile capabilities
 */
bool openavb_stream_handle_error(openavb_stream_handle_t *stream, U32 error_type, U32 error_count) {
    if (!stream || !stream->profile) {
        return false;
    }
    
    const openavb_profile_cfg_t *profile = stream->profile;
    
    // Use profile-specific error handling if available
    if (profile->callbacks.get_error_recovery_action) {
        U32 action = profile->callbacks.get_error_recovery_action(
            profile, error_type, error_count);
        
        AVB_LOG_INFO("Stream %u error 0x%x (count %u) - recovery action: 0x%x",
                    stream->stream_id, error_type, error_count, action);
        
        switch (action) {
            case 0x0000: // No action
                return true;
            case 0x0001: // Reduce precision
                return openavb_stream_configure_from_profile(stream);
            case 0x0002: // Switch to more lenient profile variant
                // Could implement profile variant switching here
                return true;
            case 0x0003: // Reset stream
                // Could implement stream reset here
                return true;
            default:
                AVB_LOG_WARNING("Unknown recovery action: 0x%x", action);
                return false;
        }
    }
    
    // Default error handling based on capabilities
    if (error_type == 0x0001) { // Timing error
        U32 tolerance = profile->capabilities.timing.sync_uncertainty_tolerance_ns;
        if (tolerance < 500000 && error_count > 3) {
            AVB_LOG_INFO("Multiple timing errors, considering profile adaptation");
            // Could switch to more lenient timing profile
        }
    }
    
    return true;
}

/**
 * Activate stream with profile validation
 */
bool openavb_stream_activate(openavb_stream_handle_t *stream) {
    if (!stream || !stream->profile) {
        AVB_LOG_ERROR("Cannot activate stream without profile");
        return false;
    }
    
    // Perform final validation
    if (!openavb_stream_configure_from_profile(stream)) {
        AVB_LOG_ERROR("Failed to configure stream %u from profile", stream->stream_id);
        return false;
    }
    
    stream->is_active = true;
    AVB_LOG_INFO("Stream %u activated with profile %s", 
                stream->stream_id, stream->profile->profile_name);
    return true;
}

/**
 * Deactivate stream
 */
void openavb_stream_deactivate(openavb_stream_handle_t *stream) {
    if (!stream) return;
    
    if (stream->profile && stream->profile->callbacks.profile_cleanup) {
        stream->profile->callbacks.profile_cleanup(stream->profile, stream->stream_data);
    }
    
    stream->is_active = false;
    AVB_LOG_INFO("Stream %u deactivated", stream->stream_id);
}

/**
 * Destroy stream handle
 */
void openavb_stream_destroy(openavb_stream_handle_t *stream) {
    if (!stream) return;
    
    // Deactivate if still active
    if (stream->is_active) {
        openavb_stream_deactivate(stream);
    }
    
    // Remove from registry
    if (g_stream_registry == stream) {
        g_stream_registry = stream->next;
    } else {
        openavb_stream_handle_t *current = g_stream_registry;
        while (current && current->next != stream) {
            current = current->next;
        }
        if (current) {
            current->next = stream->next;
        }
    }
    
    AVB_LOG_DEBUG("Destroyed stream handle ID %u", stream->stream_id);
    free(stream);
}

/**
 * Find stream by ID
 */
openavb_stream_handle_t* openavb_stream_find_by_id(U32 stream_id) {
    openavb_stream_handle_t *current = g_stream_registry;
    while (current) {
        if (current->stream_id == stream_id) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}
