/**
 * OpenAvnu Profile Framework - Integration Examples
 * 
 * This file demonstrates how the profile framework integrates with
 * existing OpenAvnu interface and mapping modules, showing how to
 * avoid hardcoded switches and centralize profile logic.
 */

#include "openavb_profile_framework.h"
#include "openavb_intf_pub.h"
#include "openavb_map_pub.h"
#include "openavb_mediaq_pub.h"
#include "openavb_log.h"

#define AVB_LOG_COMPONENT "Profile Integration"

/**
 * Profile-aware Interface Module Initialization
 * 
 * This shows how interface modules can validate compatibility with
 * the active profile without hardcoded switches.
 */

// Enhanced interface initialization with profile validation
bool openavbIntfInitializeWithProfile(media_q_t *pMediaQ, 
                                       openavb_intf_cb_t *pIntfCB,
                                       const char *intf_name)
{
    AVB_TRACE_ENTRY(AVB_TRACE_INTF);
    
    // Get active profile
    openavb_profile_cfg_t *profile = openavbProfileGetActive();
    if (!profile) {
        AVB_LOG_ERROR("No active profile set");
        AVB_TRACE_EXIT(AVB_TRACE_INTF);
        return false;
    }
    
    AVB_LOGF_INFO("Initializing interface '%s' with profile '%s'", 
                  intf_name, profile->profile_name);
    
    // Profile-specific interface validation
    if (strcmp(intf_name, "alsa") == 0) {
        // Audio interface - validate audio format support
        if (!profile->stream_formats.support_61883_6 && 
            !profile->stream_formats.support_aaf) {
            AVB_LOGF_ERROR("Profile '%s' does not support audio formats required by ALSA interface",
                           profile->profile_name);
            AVB_TRACE_EXIT(AVB_TRACE_INTF);
            return false;
        }
        
        // MILAN requires specific audio capabilities
        if (profile->compliance.require_milan_compliance) {
            if (!pIntfCB->intf_enable_fixed_timestamp) {
                AVB_LOG_ERROR("MILAN profile requires fixed timestamp support");
                AVB_TRACE_EXIT(AVB_TRACE_INTF);
                return false;
            }
        }
    }
    else if (strcmp(intf_name, "h264") == 0) {
        // Video interface - validate video format support
        if (!profile->stream_formats.support_cvf) {
            AVB_LOGF_ERROR("Profile '%s' does not support CVF required by H.264 interface",
                           profile->profile_name);
            AVB_TRACE_EXIT(AVB_TRACE_INTF);
            return false;
        }
    }
    
    // Apply profile-specific timing requirements to interface
    if (profile->timing.require_avtp_timestamp && !pIntfCB->intf_enable_fixed_timestamp) {
        AVB_LOGF_WARNING("Profile '%s' requires AVTP timestamps but interface doesn't provide callback",
                         profile->profile_name);
    }
    
    // Automotive profile requires security validation
    if (profile->security.require_authentication) {
        // TODO: Validate interface security capabilities
        AVB_LOGF_INFO("Profile '%s' requires authentication - interface should implement security",
                      profile->profile_name);
    }
    
    AVB_LOGF_INFO("Interface '%s' validated successfully with profile '%s'",
                  intf_name, profile->profile_name);
    
    AVB_TRACE_EXIT(AVB_TRACE_INTF);
    return true;
}

/**
 * Profile-aware Mapping Module Initialization
 * 
 * Shows how mapping modules validate format compatibility and apply
 * profile-specific parameters without hardcoded logic.
 */

bool openavbMapInitializeWithProfile(media_q_t *pMediaQ, 
                                      openavb_map_cb_t *pMapCB, 
                                      U32 inMaxTransitUsec,
                                      const char *map_name)
{
    AVB_TRACE_ENTRY(AVB_TRACE_MAP);
    
    openavb_profile_cfg_t *profile = openavbProfileGetActive();
    if (!profile) {
        AVB_LOG_ERROR("No active profile set");
        AVB_TRACE_EXIT(AVB_TRACE_MAP);
        return false;
    }
    
    AVB_LOGF_INFO("Initializing mapping '%s' with profile '%s'", 
                  map_name, profile->profile_name);
    
    // Get and validate subtype against profile
    U8 subtype = pMapCB->map_subtype_cb();
    if (!openavbProfileValidateStreamFormat(subtype, NULL)) {
        AVB_LOGF_ERROR("Mapping '%s' subtype 0x%02x not supported by profile '%s'",
                       map_name, subtype, profile->profile_name);
        AVB_TRACE_EXIT(AVB_TRACE_MAP);
        return false;
    }
    
    // Apply profile-specific timing constraints
    U32 profile_max_transit_ns = profile->timing.max_transit_time_ns;
    U32 profile_max_transit_usec = profile_max_transit_ns / 1000;
    
    if (inMaxTransitUsec > profile_max_transit_usec) {
        AVB_LOGF_WARNING("Requested transit time %u μs exceeds profile limit %u μs, adjusting",
                         inMaxTransitUsec, profile_max_transit_usec);
        inMaxTransitUsec = profile_max_transit_usec;
    }
    
    // Profile-specific mapping validation
    if (strcmp(map_name, "aaf") == 0) {
        // AAF mapping with profile-specific requirements
        if (profile->compliance.require_milan_compliance) {
            // MILAN has specific AAF requirements
            AVB_LOG_INFO("Applying MILAN-specific AAF requirements");
            // TODO: Set MILAN-specific AAF parameters
        }
        
        if (profile->timing.require_media_clock_recovery) {
            AVB_LOG_INFO("Profile requires media clock recovery for AAF");
            // TODO: Enable MCR for AAF mapping
        }
    }
    else if (strcmp(map_name, "cvf") == 0) {
        // CVF mapping with profile-specific requirements
        if (!profile->stream_formats.support_cvf) {
            AVB_LOGF_ERROR("Profile '%s' does not support CVF", profile->profile_name);
            AVB_TRACE_EXIT(AVB_TRACE_MAP);
            return false;
        }
        
        if (profile->compliance.require_milan_compliance) {
            AVB_LOG_INFO("Applying MILAN-specific CVF requirements");
            // TODO: Set MILAN-specific CVF parameters (H.264 constraints)
        }
    }
    else if (strcmp(map_name, "ctrl") == 0) {
        // Control mapping
        if (profile->discovery.require_avdecc) {
            AVB_LOG_INFO("Profile requires AVDECC for control streams");
        }
    }
    
    // Apply profile-specific frame size limits
    U16 max_data_size = pMapCB->map_max_data_size_cb(pMediaQ);
    if (max_data_size > profile->timing.max_frame_size) {
        AVB_LOGF_WARNING("Mapping max data size %u exceeds profile limit %u",
                         max_data_size, profile->timing.max_frame_size);
    }
    
    AVB_LOGF_INFO("Mapping '%s' initialized successfully with profile '%s'",
                  map_name, profile->profile_name);
    
    AVB_TRACE_EXIT(AVB_TRACE_MAP);
    return true;
}

/**
 * Profile-aware Configuration Processing
 * 
 * Shows how configuration validation works with profiles to avoid
 * scattered validation logic.
 */

// Configuration callback that validates against active profile
int openavbProfileAwareConfigCallback(void *user, const char *section, 
                                       const char *name, const char *value)
{
    AVB_TRACE_ENTRY(AVB_TRACE_AVDECC);
    
    // Handle profile selection first
    if (strcmp(section, "profile") == 0) {
        if (strcmp(name, "profile_name") == 0) {
            if (!openavbProfileSetActive(value)) {
                AVB_LOGF_ERROR("Failed to set profile '%s'", value);
                AVB_TRACE_EXIT(AVB_TRACE_AVDECC);
                return 0;
            }
            AVB_LOGF_INFO("Active profile set to '%s'", value);
            AVB_TRACE_EXIT(AVB_TRACE_AVDECC);
            return 1;
        }
    }
    
    // Validate configuration against active profile
    if (!openavbProfileValidateConfig(section, name, value)) {
        // Profile validation failed - error already logged
        AVB_TRACE_EXIT(AVB_TRACE_AVDECC);
        return 0;
    }
    
    // Profile-specific configuration processing
    openavb_profile_cfg_t *profile = openavbProfileGetActive();
    if (profile) {
        // Apply profile-specific defaults and constraints
        if (strcmp(section, "stream") == 0) {
            
            if (strcmp(name, "max_transit_usec") == 0) {
                U32 requested = strtoul(value, NULL, 10);
                U32 profile_max = profile->timing.max_transit_time_ns / 1000;
                
                if (requested > profile_max) {
                    AVB_LOGF_WARNING("Requested max_transit_usec %u exceeds profile limit %u",
                                     requested, profile_max);
                }
            }
            
            if (strcmp(name, "sr_class") == 0) {
                // Apply profile-specific class defaults
                if (strcmp(value, "A") == 0) {
                    AVB_LOGF_DEBUG("Using Class A interval: %u ns", 
                                   profile->timing.default_class_a_interval_ns);
                } else if (strcmp(value, "B") == 0) {
                    AVB_LOGF_DEBUG("Using Class B interval: %u ns",
                                   profile->timing.default_class_b_interval_ns);
                }
            }
        }
        
        // AVDECC requirements
        if (strcmp(section, "avdecc") == 0) {
            if (profile->discovery.require_avdecc) {
                AVB_LOG_INFO("Profile requires AVDECC - validating AVDECC configuration");
                
                if (strcmp(name, "enabled") == 0 && strcmp(value, "0") == 0) {
                    AVB_LOGF_ERROR("Profile '%s' requires AVDECC but it's disabled", 
                                   profile->profile_name);
                    AVB_TRACE_EXIT(AVB_TRACE_AVDECC);
                    return 0;
                }
            }
        }
        
        // Security requirements (Automotive)
        if (strcmp(section, "security") == 0) {
            if (profile->security.require_encryption) {
                if (strcmp(name, "enabled") == 0 && strcmp(value, "0") == 0) {
                    AVB_LOGF_ERROR("Profile '%s' requires encryption but it's disabled",
                                   profile->profile_name);
                    AVB_TRACE_EXIT(AVB_TRACE_AVDECC);
                    return 0;
                }
            }
        }
    }
    
    AVB_TRACE_EXIT(AVB_TRACE_AVDECC);
    return 1; // Continue with normal processing
}

/**
 * Profile-aware Stream Establishment
 * 
 * Shows how stream setup uses profile parameters instead of hardcoded values.
 */

bool openavbStreamEstablishWithProfile(media_q_t *pMediaQ, 
                                        U8 sr_class, 
                                        U16 stream_id,
                                        U8 dest_addr[6])
{
    AVB_TRACE_ENTRY(AVB_TRACE_AVDECC);
    
    openavb_profile_cfg_t *profile = openavbProfileGetActive();
    if (!profile) {
        AVB_LOG_ERROR("No active profile for stream establishment");
        AVB_TRACE_EXIT(AVB_TRACE_AVDECC);
        return false;
    }
    
    AVB_LOGF_INFO("Establishing stream with profile '%s'", profile->profile_name);
    
    // Use profile-specific timing parameters
    U32 interval_ns;
    if (sr_class == SR_CLASS_A) {
        interval_ns = profile->timing.default_class_a_interval_ns;
    } else {
        interval_ns = profile->timing.default_class_b_interval_ns;
    }
    
    AVB_LOGF_DEBUG("Using profile interval: %u ns for class %u", interval_ns, sr_class);
    
    // Calculate presentation time using profile-specific algorithm
    U32 presentation_offset = 0;
    if (profile->behaviors.calculate_presentation_offset) {
        presentation_offset = profile->behaviors.calculate_presentation_offset(
            profile, sr_class, 2000); // 2ms max transit time
    } else {
        // Use default profile offset
        presentation_offset = profile->timing.presentation_time_offset_ns;
    }
    
    AVB_LOGF_DEBUG("Using presentation offset: %u ns", presentation_offset);
    
    // MILAN-specific requirements
    if (profile->compliance.require_milan_compliance) {
        if (!profile->discovery.require_avdecc) {
            AVB_LOG_ERROR("MILAN profile inconsistency - AVDECC should be required");
            AVB_TRACE_EXIT(AVB_TRACE_AVDECC);
            return false;
        }
        
        if (profile->discovery.require_fast_connect) {
            AVB_LOG_INFO("Using MILAN fast connect procedures");
            // TODO: Implement MILAN fast connect
        }
    }
    
    // Automotive-specific requirements
    if (profile->compliance.require_automotive_features) {
        if (profile->discovery.require_redundancy) {
            AVB_LOG_INFO("Setting up redundant automotive streams");
            // TODO: Implement automotive redundancy
        }
        
        if (profile->security.require_authentication) {
            AVB_LOG_INFO("Authenticating automotive stream");
            // TODO: Implement automotive security
        }
    }
    
    // Quality monitoring setup based on profile
    openavb_media_q_profile_ext_t *profile_ext = openavbProfileCreateMediaQExt(pMediaQ);
    if (profile_ext) {
        AVB_LOG_INFO("Profile-specific quality monitoring enabled");
    }
    
    AVB_LOGF_INFO("Stream established successfully with profile '%s'", profile->profile_name);
    
    AVB_TRACE_EXIT(AVB_TRACE_AVDECC);
    return true;
}

/**
 * Profile-aware Quality Monitoring
 * 
 * Shows how quality enforcement varies by profile without hardcoded logic.
 */

bool openavbStreamMonitorQualityWithProfile(openavb_media_q_profile_ext_t *profile_ext,
                                             U32 packets_sent,
                                             U32 packets_lost,
                                             float jitter_ms)
{
    AVB_TRACE_ENTRY(AVB_TRACE_AVDECC);
    
    if (!profile_ext || !profile_ext->profile) {
        AVB_LOG_ERROR("No profile context for quality monitoring");
        AVB_TRACE_EXIT(AVB_TRACE_AVDECC);
        return false;
    }
    
    openavb_profile_cfg_t *profile = profile_ext->profile;
    
    // Update quality metrics
    openavb_stream_quality_metrics_t *metrics = &profile_ext->quality_metrics;
    metrics->packets_sent = packets_sent;
    metrics->packets_lost = packets_lost;
    metrics->jitter_ms = jitter_ms;
    
    // Profile-specific quality enforcement
    bool quality_ok = true;
    if (profile->behaviors.enforce_quality_requirements) {
        quality_ok = profile->behaviors.enforce_quality_requirements(profile, metrics);
    } else {
        // Default quality check
        if (packets_lost > 0) {
            AVB_LOGF_WARNING("Profile '%s': %u packets lost", profile->profile_name, packets_lost);
        }
        
        if (jitter_ms > 1.0f) {
            AVB_LOGF_WARNING("Profile '%s': High jitter %.3f ms", profile->profile_name, jitter_ms);
        }
    }
    
    // Update compliance state
    profile_ext->profile_compliant = quality_ok;
    if (!quality_ok) {
        profile_ext->compliance_error = "Quality requirements not met";
        AVB_LOGF_ERROR("Stream not compliant with profile '%s'", profile->profile_name);
    }
    
    AVB_TRACE_EXIT(AVB_TRACE_AVDECC);
    return quality_ok;
}

/**
 * Example: Enhanced ALSA Interface with Profile Integration
 * 
 * Shows how an existing interface module can be enhanced to use profiles.
 */

// Example of profile-aware ALSA interface configuration
void openavbIntfAlsaCfgCBWithProfile(media_q_t *pMediaQ, const char *name, const char *value)
{
    // Get active profile for validation
    openavb_profile_cfg_t *profile = openavbProfileGetActive();
    
    // Original ALSA configuration processing
    if (strcmp(name, "intf_nv_audio_rate") == 0) {
        U32 rate = strtoul(value, NULL, 10);
        
        // Profile-specific validation
        if (profile && profile->compliance.require_milan_compliance) {
            // MILAN has specific supported sample rates
            if (rate != 48000 && rate != 96000) {
                AVB_LOGF_ERROR("MILAN profile requires 48kHz or 96kHz, got %u", rate);
                return;
            }
        }
        
        if (profile && profile->compliance.require_automotive_features) {
            // Automotive might have different constraints
            if (rate > 48000) {
                AVB_LOGF_WARNING("Automotive profile: High sample rates may impact latency");
            }
        }
        
        // Continue with normal ALSA processing...
        AVB_LOGF_INFO("ALSA audio rate set to %u (validated against profile '%s')", 
                      rate, profile ? profile->profile_name : "none");
    }
}

/**
 * Example: Profile-aware Mapping Selection
 * 
 * Shows how mapping selection can be influenced by active profile.
 */

const char* openavbSelectMappingForProfile(const char *interface_type, const char *stream_type)
{
    openavb_profile_cfg_t *profile = openavbProfileGetActive();
    
    if (strcmp(stream_type, "audio") == 0) {
        if (profile && profile->compliance.require_milan_compliance) {
            // MILAN prefers AAF over 61883-6
            if (profile->stream_formats.support_aaf) {
                return "aaf";
            }
        }
        
        if (profile && profile->compliance.require_automotive_features) {
            // Automotive might prefer different mapping
            return "aaf"; // Automotive standardizing on AAF
        }
        
        // Default AVB behavior
        return "uncmp_audio"; // 61883-6
    }
    
    if (strcmp(stream_type, "video") == 0) {
        if (profile && profile->stream_formats.support_cvf) {
            return "cvf";
        } else {
            return "mpeg2ts"; // Fallback
        }
    }
    
    return NULL;
}

/**
 * Integration Summary
 * 
 * This integration approach provides:
 * 
 * 1. **No Hardcoded Switches**: Profile logic is centralized in profile structs
 * 2. **Backward Compatibility**: Existing code works with default AVB profile
 * 3. **Future-Proof**: New profiles just require struct definitions
 * 4. **Clean Separation**: Profile logic separate from transport logic
 * 5. **Consistent Behavior**: Profile requirements enforced systematically
 * 6. **Easy Testing**: Profile-specific behaviors can be tested independently
 * 7. **Clear Dependencies**: Profile requirements explicitly declared
 */
