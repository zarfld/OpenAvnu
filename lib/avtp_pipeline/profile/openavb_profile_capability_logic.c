/**
 * OpenAvnu Profile Framework - True Capability-Based Implementation
 * 
 * This file demonstrates PURE capability-based logic without any hardcoded
 * version switches or profile-specific assumptions. All decisions are made
 * based on the profile's capability matrix values.
 */

#include "openavb_profile_framework.h"
#include "openavb_log.h"

#define AVB_LOG_COMPONENT "CapabilityLogic"

/**
 * Capability-based stream validation
 * NO hardcoded version checks - pure capability-based decisions
 */
bool openavb_stream_validate_capabilities(const openavb_profile_cfg_t *profile,
                                         const stream_config_t *config)
{
    if (!profile || !config) {
        return false;
    }

    // Security capability validation
    if (profile->capabilities.security.authentication_required) {
        if (!config->security_enabled) {
            AVB_LOG_ERROR("Profile requires authentication but stream config has security disabled");
            return false;
        }
        
        // Check if requested cipher suite is supported
        U32 requested_cipher = config->cipher_suite;
        if (!(profile->capabilities.security.supported_cipher_suites & requested_cipher)) {
            AVB_LOGF_ERROR("Requested cipher suite 0x%x not supported by profile (supports: 0x%x)",
                          requested_cipher, profile->capabilities.security.supported_cipher_suites);
            return false;
        }
    }

    // Transport capability validation
    if (profile->capabilities.transport.avtp_timestamp_required) {
        if (!config->use_avtp_timestamps) {
            AVB_LOG_ERROR("Profile requires AVTP timestamps but stream config has them disabled");
            return false;
        }
    }

    if (profile->capabilities.transport.media_clock_recovery_required) {
        if (!config->media_clock_recovery) {
            AVB_LOG_ERROR("Profile requires media clock recovery but stream config has it disabled");
            return false;
        }
    }

    // Timing capability validation
    if (config->presentation_offset_ns < profile->capabilities.timing.min_presentation_offset_ns) {
        AVB_LOGF_ERROR("Presentation offset %u ns below profile minimum %u ns",
                      config->presentation_offset_ns,
                      profile->capabilities.timing.min_presentation_offset_ns);
        return false;
    }

    if (config->presentation_offset_ns > profile->capabilities.timing.max_presentation_offset_ns) {
        AVB_LOGF_ERROR("Presentation offset %u ns exceeds profile maximum %u ns",
                      config->presentation_offset_ns,
                      profile->capabilities.timing.max_presentation_offset_ns);
        return false;
    }

    // Discovery capability validation
    if (profile->capabilities.discovery.avdecc_required) {
        if (!config->avdecc_enabled) {
            AVB_LOG_ERROR("Profile requires AVDECC but stream config has it disabled");
            return false;
        }
    }

    // QoS capability validation
    if (profile->capabilities.qos.credit_based_shaping_required) {
        if (!config->credit_based_shaping) {
            AVB_LOG_ERROR("Profile requires credit-based shaping but stream config has it disabled");
            return false;
        }
    }

    if (config->max_frame_size > profile->capabilities.qos.max_frame_size) {
        AVB_LOGF_ERROR("Frame size %u exceeds profile maximum %u",
                      config->max_frame_size, profile->capabilities.qos.max_frame_size);
        return false;
    }

    return true;
}

/**
 * Capability-based timing configuration
 * Calculates optimal timing parameters based on profile capabilities
 */
bool openavb_stream_configure_timing(const openavb_profile_cfg_t *profile,
                                     stream_timing_config_t *timing_config)
{
    if (!profile || !timing_config) {
        return false;
    }

    // Calculate presentation offset based on capabilities
    U32 min_offset = profile->capabilities.timing.min_presentation_offset_ns;
    U32 max_offset = profile->capabilities.timing.max_presentation_offset_ns;
    U32 requested_offset = timing_config->requested_presentation_offset_ns;

    if (requested_offset < min_offset) {
        AVB_LOGF_INFO("Adjusting presentation offset from %u to minimum %u ns",
                     requested_offset, min_offset);
        timing_config->actual_presentation_offset_ns = min_offset;
    } else if (requested_offset > max_offset) {
        AVB_LOGF_INFO("Adjusting presentation offset from %u to maximum %u ns",
                     requested_offset, max_offset);
        timing_config->actual_presentation_offset_ns = max_offset;
    } else {
        timing_config->actual_presentation_offset_ns = requested_offset;
    }

    // Configure sync tolerance based on capabilities
    timing_config->sync_tolerance_ns = profile->capabilities.timing.sync_uncertainty_tolerance_ns;

    // Configure wakeup time based on capabilities
    timing_config->max_wakeup_time_ns = profile->capabilities.timing.max_wakeup_time_ns;

    // Enable features based on capabilities
    timing_config->presentation_time_enabled = profile->capabilities.timing.presentation_time_required;
    timing_config->gptp_required = profile->capabilities.timing.gptp_required;

    AVB_LOGF_INFO("Configured timing: offset=%u ns, tolerance=%u ns, wakeup=%u ns",
                 timing_config->actual_presentation_offset_ns,
                 timing_config->sync_tolerance_ns,
                 timing_config->max_wakeup_time_ns);

    return true;
}

/**
 * Capability-based security configuration
 * Configures security features based on profile capabilities
 */
bool openavb_stream_configure_security(const openavb_profile_cfg_t *profile,
                                       stream_security_config_t *security_config)
{
    if (!profile || !security_config) {
        return false;
    }

    // Enable authentication if required
    if (profile->capabilities.security.authentication_required) {
        security_config->authentication_enabled = true;
        
        // Select best available authentication method
        U32 supported_auth = profile->capabilities.security.supported_auth_methods;
        if (supported_auth & AUTH_METHOD_CERTIFICATE) {
            security_config->auth_method = AUTH_METHOD_CERTIFICATE;
            security_config->certificate_validation = 
                profile->capabilities.security.certificate_validation_required;
            AVB_LOG_INFO("Enabled certificate-based authentication");
        } else if (supported_auth & AUTH_METHOD_PSK) {
            security_config->auth_method = AUTH_METHOD_PSK;
            AVB_LOG_INFO("Enabled PSK-based authentication");
        } else {
            AVB_LOG_ERROR("Profile requires authentication but no supported methods available");
            return false;
        }
    }

    // Enable encryption if required
    if (profile->capabilities.security.encryption_required) {
        security_config->encryption_enabled = true;
        
        // Select best available cipher suite
        U32 supported_ciphers = profile->capabilities.security.supported_cipher_suites;
        if (supported_ciphers & CIPHER_AES_256_GCM) {
            security_config->cipher_suite = CIPHER_AES_256_GCM;
            AVB_LOG_INFO("Enabled AES-256-GCM encryption");
        } else if (supported_ciphers & CIPHER_AES_128_GCM) {
            security_config->cipher_suite = CIPHER_AES_128_GCM;
            AVB_LOG_INFO("Enabled AES-128-GCM encryption");
        } else if (supported_ciphers & CIPHER_AES_256) {
            security_config->cipher_suite = CIPHER_AES_256;
            AVB_LOG_INFO("Enabled AES-256 encryption");
        } else if (supported_ciphers & CIPHER_AES_128) {
            security_config->cipher_suite = CIPHER_AES_128;
            AVB_LOG_INFO("Enabled AES-128 encryption");
        } else {
            AVB_LOG_ERROR("Profile requires encryption but no supported cipher suites available");
            return false;
        }
    }

    // Configure secure channels if supported
    if (profile->capabilities.security.secure_association_required) {
        security_config->secure_channels_enabled = true;
        AVB_LOG_INFO("Enabled secure channel associations");
    }

    return true;
}

/**
 * Capability-based QoS configuration
 * Configures quality of service based on profile capabilities
 */
bool openavb_stream_configure_qos(const openavb_profile_cfg_t *profile,
                                  stream_qos_config_t *qos_config)
{
    if (!profile || !qos_config) {
        return false;
    }

    // Configure traffic shaping based on capabilities
    if (profile->capabilities.qos.credit_based_shaping_required) {
        qos_config->credit_based_shaping_enabled = true;
        AVB_LOG_INFO("Enabled credit-based shaping (required by profile)");
    }

    if (profile->capabilities.qos.time_based_shaping_supported) {
        qos_config->time_based_shaping_available = true;
        AVB_LOG_INFO("Time-based shaping available");
    }

    if (profile->capabilities.qos.frame_preemption_supported) {
        qos_config->frame_preemption_available = true;
        AVB_LOG_INFO("Frame preemption available");
    }

    // Configure traffic classes based on requirements
    U8 required_classes = profile->capabilities.qos.required_traffic_classes;
    qos_config->available_traffic_classes = required_classes;

    if (required_classes & TRAFFIC_CLASS_A) {
        AVB_LOG_INFO("Class A traffic supported");
    }
    if (required_classes & TRAFFIC_CLASS_B) {
        AVB_LOG_INFO("Class B traffic supported");
    }
    if (required_classes & TRAFFIC_CLASS_CDT) {
        AVB_LOG_INFO("CDT traffic supported");
    }

    // Set frame size limits
    qos_config->max_frame_size = profile->capabilities.qos.max_frame_size;
    qos_config->max_burst_size = profile->capabilities.qos.max_burst_size;

    AVB_LOGF_INFO("QoS limits: max_frame=%u, max_burst=%u",
                 qos_config->max_frame_size, qos_config->max_burst_size);

    return true;
}

/**
 * Capability-based transport configuration
 * Configures transport features based on profile capabilities
 */
bool openavb_stream_configure_transport(const openavb_profile_cfg_t *profile,
                                        stream_transport_config_t *transport_config)
{
    if (!profile || !transport_config) {
        return false;
    }

    // Configure based on transport capabilities
    transport_config->avtp_timestamps_enabled = 
        profile->capabilities.transport.avtp_timestamp_required;
    
    transport_config->media_clock_recovery_enabled = 
        profile->capabilities.transport.media_clock_recovery_required;

    if (profile->capabilities.transport.fast_connect_supported) {
        transport_config->fast_connect_available = true;
        AVB_LOG_INFO("Fast connect available");
    }

    if (profile->capabilities.transport.redundant_streams_supported) {
        transport_config->redundancy_available = true;
        AVB_LOG_INFO("Stream redundancy available");
    }

    if (profile->capabilities.transport.secure_channels_supported) {
        transport_config->secure_channels_available = true;
        AVB_LOG_INFO("Secure channels available");
    }

    // Set capacity limits
    transport_config->max_streams_per_entity = 
        profile->capabilities.transport.max_streams_per_entity;
    transport_config->max_listeners_per_stream = 
        profile->capabilities.transport.max_listeners_per_stream;

    AVB_LOGF_INFO("Transport limits: max_streams=%u, max_listeners=%u",
                 transport_config->max_streams_per_entity,
                 transport_config->max_listeners_per_stream);

    return true;
}

/**
 * Capability-based error handling
 * Handles errors based on profile capabilities and requirements
 */
U32 openavb_stream_handle_error_by_capability(const openavb_profile_cfg_t *profile,
                                              U32 error_type, U32 error_count,
                                              const openavb_stream_quality_metrics_t *metrics)
{
    if (!profile) {
        return ERROR_ACTION_IGNORE;
    }

    U32 action = ERROR_ACTION_LOG;

    switch (error_type) {
        case ERROR_TIMING_VIOLATION:
            // Strict timing profiles have lower tolerance
            if (profile->capabilities.timing.sync_uncertainty_tolerance_ns < 250000) {
                // Very strict timing requirements
                if (error_count > 1) {
                    action = ERROR_ACTION_RESTART_STREAM;
                    AVB_LOG_ERROR("Multiple timing violations in strict timing profile - restarting stream");
                } else {
                    action = ERROR_ACTION_ADJUST_TIMING;
                    AVB_LOG_WARNING("Timing violation in strict profile - adjusting parameters");
                }
            } else if (profile->capabilities.timing.sync_uncertainty_tolerance_ns < 1000000) {
                // Moderate timing requirements
                if (error_count > 5) {
                    action = ERROR_ACTION_RESTART_STREAM;
                } else {
                    action = ERROR_ACTION_ADJUST_TIMING;
                }
            } else {
                // Lenient timing requirements
                if (error_count > 10) {
                    action = ERROR_ACTION_RESTART_STREAM;
                } else {
                    action = ERROR_ACTION_LOG;
                }
            }
            break;

        case ERROR_SECURITY_FAILURE:
            if (profile->capabilities.security.authentication_required) {
                // Security is mandatory - must restart
                action = ERROR_ACTION_RESTART_STREAM;
                AVB_LOG_ERROR("Security failure in security-required profile - restarting stream");
            } else {
                // Security is optional - degrade gracefully
                action = ERROR_ACTION_DISABLE_SECURITY;
                AVB_LOG_WARNING("Security failure in optional security profile - disabling security");
            }
            break;

        case ERROR_CAPACITY_EXCEEDED:
            if (metrics && metrics->packets_lost > 0) {
                // Packet loss indicates real capacity issue
                if (profile->capabilities.qos.frame_preemption_supported) {
                    action = ERROR_ACTION_ENABLE_PREEMPTION;
                    AVB_LOG_INFO("Enabling frame preemption to handle capacity issues");
                } else if (profile->capabilities.qos.time_based_shaping_supported) {
                    action = ERROR_ACTION_ADJUST_SHAPING;
                    AVB_LOG_INFO("Adjusting time-based shaping to handle capacity issues");
                } else {
                    action = ERROR_ACTION_REDUCE_STREAMS;
                    AVB_LOG_WARNING("Reducing stream count to handle capacity issues");
                }
            }
            break;

        default:
            action = ERROR_ACTION_LOG;
            break;
    }

    return action;
}

/**
 * Capability-based profile compatibility checking
 * Checks if two profiles can interoperate based on their capabilities
 */
bool openavb_profiles_compatible_by_capability(const openavb_profile_cfg_t *profile1,
                                              const openavb_profile_cfg_t *profile2)
{
    if (!profile1 || !profile2) {
        return false;
    }

    // Security compatibility
    bool sec1_required = profile1->capabilities.security.authentication_required;
    bool sec2_required = profile2->capabilities.security.authentication_required;
    
    if (sec1_required != sec2_required) {
        // One requires security, the other doesn't - check if optional security is possible
        const openavb_profile_cfg_t *sec_profile = sec1_required ? profile1 : profile2;
        const openavb_profile_cfg_t *non_sec_profile = sec1_required ? profile2 : profile1;
        
        // Non-security profile must support optional security for compatibility
        if (!non_sec_profile->capabilities.security.supported_cipher_suites) {
            AVB_LOG_INFO("Profiles incompatible: security mismatch with no fallback");
            return false;
        }
    }

    // Timing compatibility - check for overlap in presentation offset ranges
    U32 min1 = profile1->capabilities.timing.min_presentation_offset_ns;
    U32 max1 = profile1->capabilities.timing.max_presentation_offset_ns;
    U32 min2 = profile2->capabilities.timing.min_presentation_offset_ns;
    U32 max2 = profile2->capabilities.timing.max_presentation_offset_ns;
    
    if (min1 > max2 || min2 > max1) {
        AVB_LOGF_INFO("Profiles incompatible: no timing overlap (%u-%u vs %u-%u)",
                     min1, max1, min2, max2);
        return false;
    }

    // Transport capability compatibility
    bool fast1 = profile1->capabilities.transport.fast_connect_supported;
    bool fast2 = profile2->capabilities.transport.fast_connect_supported;
    
    // Fast connect compatibility - at least one must support it if either requires it
    if ((fast1 || fast2) && !(fast1 && fast2)) {
        // Asymmetric fast connect support is acceptable
        AVB_LOG_INFO("Asymmetric fast connect support - will negotiate");
    }

    // QoS compatibility - frame size limits must be compatible
    U32 max_frame1 = profile1->capabilities.qos.max_frame_size;
    U32 max_frame2 = profile2->capabilities.qos.max_frame_size;
    
    if (max_frame1 != max_frame2) {
        // Use smaller frame size
        U32 min_frame = (max_frame1 < max_frame2) ? max_frame1 : max_frame2;
        AVB_LOGF_INFO("Frame size negotiation: using %u bytes", min_frame);
    }

    AVB_LOGF_INFO("Profiles %s and %s are compatible",
                 profile1->version_string, profile2->version_string);
    return true;
}

/**
 * Example of comprehensive capability-based stream setup
 * NO hardcoded profile checks - everything based on capabilities
 */
bool openavb_stream_setup_by_capabilities(const openavb_profile_cfg_t *profile,
                                         const stream_config_t *base_config,
                                         stream_runtime_config_t *runtime_config)
{
    if (!profile || !base_config || !runtime_config) {
        return false;
    }

    AVB_LOGF_INFO("Setting up stream with profile: %s", profile->version_string);

    // Validate basic compatibility
    if (!openavb_stream_validate_capabilities(profile, base_config)) {
        AVB_LOG_ERROR("Stream configuration incompatible with profile capabilities");
        return false;
    }

    // Configure each subsystem based on capabilities
    if (!openavb_stream_configure_timing(profile, &runtime_config->timing)) {
        AVB_LOG_ERROR("Failed to configure timing based on profile capabilities");
        return false;
    }

    if (!openavb_stream_configure_security(profile, &runtime_config->security)) {
        AVB_LOG_ERROR("Failed to configure security based on profile capabilities");
        return false;
    }

    if (!openavb_stream_configure_qos(profile, &runtime_config->qos)) {
        AVB_LOG_ERROR("Failed to configure QoS based on profile capabilities");
        return false;
    }

    if (!openavb_stream_configure_transport(profile, &runtime_config->transport)) {
        AVB_LOG_ERROR("Failed to configure transport based on profile capabilities");
        return false;
    }

    AVB_LOG_INFO("Stream successfully configured using capability-based logic");
    return true;
}
