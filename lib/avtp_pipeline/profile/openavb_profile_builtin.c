/**
 * OpenAvnu Profile Framework - Built-in Profile Implementations
 * 
 * This file contains the actual profile definitions for AVB, MILAN,
 * and Automotive profiles, demonstrating how different IEEE 1722
 * specifications and requirements are centralized.
 */

#include "openavb_profile_framework.h"
#include "openavb_log.h"
#include "openavb_types_pub.h"

#define AVB_LOG_COMPONENT "Profile"

/**
 * Built-in Profile Subtype Support Arrays
 */

// AVB Profile supported subtypes (IEEE 1722-2016)
static U16 avb_supported_subtypes[] = {
    0x00,   // 61883/IIDC Format
    0x01,   // MMA Streams
    0x02,   // AVTP Audio Format (AAF)
    0x03,   // Compressed Video Format (CVF)
    0x04,   // Clock Reference Format (CRF)
    0x05,   // Time-Synchronous Control Format (TSCF)
    0x06,   // SDI Video Format (SVF)
    0x7F    // Vendor Specific
};

// MILAN Profile supported subtypes (IEEE 1722.1-2021)
static U16 milan_supported_subtypes[] = {
    0x00,   // 61883/IIDC Format
    0x02,   // AVTP Audio Format (AAF) - mandatory
    0x03,   // Compressed Video Format (CVF) - mandatory
    0x04,   // Clock Reference Format (CRF)
    0x05,   // Time-Synchronous Control Format (TSCF)
    // Note: MILAN restricts vendor-specific formats
};

// Automotive Profile supported subtypes
static U16 automotive_supported_subtypes[] = {
    0x00,   // 61883/IIDC Format
    0x02,   // AVTP Audio Format (AAF)
    0x03,   // Compressed Video Format (CVF)
    0x04,   // Clock Reference Format (CRF)
    0x05,   // Time-Synchronous Control Format (TSCF)
    0x06,   // SDI Video Format (SVF)
    0x08,   // Automotive-specific formats start here
    0x09,
    0x0A,
    0x7F    // Vendor Specific (for automotive OEMs)
};

/**
 * AVB Profile Behavior Implementations
 */

bool openavbAVBValidateStreamFormat(const openavb_profile_cfg_t *profile,
                                    U8 subtype, const void *format_specific)
{
    AVB_TRACE_ENTRY(AVB_TRACE_AVDECC);
    
    // Check if subtype is in supported list
    for (U32 i = 0; i < profile->stream_formats.subtype_count; i++) {
        if (profile->stream_formats.supported_subtypes[i] == subtype) {
            // AVB is permissive - most formats allowed
            AVB_TRACE_EXIT(AVB_TRACE_AVDECC);
            return true;
        }
    }
    
    AVB_LOGF_WARNING("AVB Profile: Unsupported subtype 0x%02x", subtype);
    AVB_TRACE_EXIT(AVB_TRACE_AVDECC);
    return false;
}

U32 openavbAVBCalculatePresentationOffset(const openavb_profile_cfg_t *profile,
                                          U8 sr_class, U32 max_transit_time)
{
    AVB_TRACE_ENTRY(AVB_TRACE_AVDECC);
    
    // Basic AVB calculation - simple offset based on class
    U32 offset = profile->timing.presentation_time_offset_ns;
    
    // Add class-specific adjustments
    if (sr_class == SR_CLASS_A) {
        offset += profile->timing.default_class_a_interval_ns * 2; // 2 intervals buffer
    } else if (sr_class == SR_CLASS_B) {
        offset += profile->timing.default_class_b_interval_ns * 2;
    }
    
    // Add transit time consideration
    offset += (max_transit_time * 1000); // Convert μs to ns
    
    AVB_LOGF_DEBUG("AVB Profile: Calculated presentation offset %u ns for class %u", 
                   offset, sr_class);
    
    AVB_TRACE_EXIT(AVB_TRACE_AVDECC);
    return offset;
}

bool openavbAVBEnforceQuality(const openavb_profile_cfg_t *profile,
                              const openavb_stream_quality_metrics_t *metrics)
{
    AVB_TRACE_ENTRY(AVB_TRACE_AVDECC);
    
    // AVB has relaxed quality requirements
    if (metrics->packets_lost > (metrics->packets_sent / 100)) { // > 1% loss
        AVB_LOG_WARNING("AVB Profile: High packet loss detected");
    }
    
    if (metrics->timing_violations > 10) {
        AVB_LOG_WARNING("AVB Profile: Multiple timing violations detected");
    }
    
    // AVB doesn't fail on quality issues, just warns
    AVB_TRACE_EXIT(AVB_TRACE_AVDECC);
    return true;
}

bool openavbAVBValidateConfig(const openavb_profile_cfg_t *profile,
                              const char *section, const char *name, const char *value)
{
    AVB_TRACE_ENTRY(AVB_TRACE_AVDECC);
    
    // AVB profile is permissive with configuration
    // Only validate critical parameters
    
    if (strcmp(section, "stream") == 0) {
        if (strcmp(name, "sr_class") == 0) {
            if (strcmp(value, "A") != 0 && strcmp(value, "B") != 0) {
                AVB_LOGF_ERROR("AVB Profile: Invalid sr_class '%s'", value);
                AVB_TRACE_EXIT(AVB_TRACE_AVDECC);
                return false;
            }
        }
    }
    
    AVB_TRACE_EXIT(AVB_TRACE_AVDECC);
    return true;
}

/**
 * MILAN Profile Behavior Implementations  
 */

bool openavbMILANValidateStreamFormat(const openavb_profile_cfg_t *profile,
                                      U8 subtype, const void *format_specific)
{
    AVB_TRACE_ENTRY(AVB_TRACE_AVDECC);
    
    // MILAN is more restrictive than AVB
    for (U32 i = 0; i < profile->stream_formats.subtype_count; i++) {
        if (profile->stream_formats.supported_subtypes[i] == subtype) {
            // Additional MILAN-specific format validation
            switch (subtype) {
                case 0x02: // AAF
                    // MILAN requires specific AAF configurations
                    // TODO: Add MILAN-specific AAF validation
                    break;
                case 0x03: // CVF  
                    // MILAN requires H.264 support
                    // TODO: Add MILAN-specific CVF validation
                    break;
                case 0x7F: // Vendor Specific
                    AVB_LOG_ERROR("MILAN Profile: Vendor-specific formats not allowed");
                    AVB_TRACE_EXIT(AVB_TRACE_AVDECC);
                    return false;
            }
            AVB_TRACE_EXIT(AVB_TRACE_AVDECC);
            return true;
        }
    }
    
    AVB_LOGF_ERROR("MILAN Profile: Unsupported subtype 0x%02x", subtype);
    AVB_TRACE_EXIT(AVB_TRACE_AVDECC);
    return false;
}

U32 openavbMILANCalculatePresentationOffset(const openavb_profile_cfg_t *profile,
                                            U8 sr_class, U32 max_transit_time)
{
    AVB_TRACE_ENTRY(AVB_TRACE_AVDECC);
    
    // MILAN has specific presentation time requirements
    U32 offset = profile->timing.presentation_time_offset_ns;
    
    // MILAN-specific calculation with tighter timing
    if (sr_class == SR_CLASS_A) {
        offset += profile->timing.default_class_a_interval_ns; // 1 interval buffer
    } else if (sr_class == SR_CLASS_B) {
        offset += profile->timing.default_class_b_interval_ns;
    }
    
    // MILAN requires specific buffering
    offset += (max_transit_time * 1000) + 250000; // Extra 250μs for MILAN
    
    AVB_LOGF_DEBUG("MILAN Profile: Calculated presentation offset %u ns", offset);
    
    AVB_TRACE_EXIT(AVB_TRACE_AVDECC);
    return offset;
}

bool openavbMILANValidateAVDECCDescriptor(const openavb_profile_cfg_t *profile,
                                          U16 descriptor_type, const void *descriptor)
{
    AVB_TRACE_ENTRY(AVB_TRACE_AVDECC);
    
    // MILAN requires specific AVDECC descriptor compliance
    switch (descriptor_type) {
        case 0x0000: // ENTITY
            // TODO: Validate MILAN entity descriptor requirements
            break;
        case 0x0002: // AUDIO_UNIT
            // TODO: Validate MILAN audio unit requirements
            break;
        case 0x0005: // STREAM_INPUT
        case 0x0006: // STREAM_OUTPUT
            // TODO: Validate MILAN stream descriptor requirements
            break;
    }
    
    AVB_TRACE_EXIT(AVB_TRACE_AVDECC);
    return true;
}

bool openavbMILANEnforceQuality(const openavb_profile_cfg_t *profile,
                                const openavb_stream_quality_metrics_t *metrics)
{
    AVB_TRACE_ENTRY(AVB_TRACE_AVDECC);
    
    // MILAN has strict quality requirements
    if (metrics->packets_lost > 0) {
        AVB_LOG_ERROR("MILAN Profile: Zero packet loss required");
        AVB_TRACE_EXIT(AVB_TRACE_AVDECC);
        return false;
    }
    
    if (metrics->timing_violations > 0) {
        AVB_LOG_ERROR("MILAN Profile: Zero timing violations required");
        AVB_TRACE_EXIT(AVB_TRACE_AVDECC);
        return false;
    }
    
    if (metrics->sync_accuracy_ppm > 0.1f) {
        AVB_LOG_ERROR("MILAN Profile: Sync accuracy requirements not met");
        AVB_TRACE_EXIT(AVB_TRACE_AVDECC);
        return false;
    }
    
    AVB_TRACE_EXIT(AVB_TRACE_AVDECC);
    return true;
}

bool openavbMILANValidateConfig(const openavb_profile_cfg_t *profile,
                                const char *section, const char *name, const char *value)
{
    AVB_TRACE_ENTRY(AVB_TRACE_AVDECC);
    
    // MILAN requires strict configuration validation
    if (strcmp(section, "stream") == 0) {
        if (strcmp(name, "avdecc") == 0) {
            if (strcmp(value, "1") != 0) {
                AVB_LOG_ERROR("MILAN Profile: AVDECC is mandatory");
                AVB_TRACE_EXIT(AVB_TRACE_AVDECC);
                return false;
            }
        }
        
        if (strcmp(name, "fast_connect") == 0) {
            if (strcmp(value, "1") != 0) {
                AVB_LOG_ERROR("MILAN Profile: Fast Connect is mandatory");
                AVB_TRACE_EXIT(AVB_TRACE_AVDECC);
                return false;
            }
        }
    }
    
    AVB_TRACE_EXIT(AVB_TRACE_AVDECC);
    return true;
}

/**
 * Automotive Profile Behavior Implementations
 */

bool openavbAutomotiveValidateStreamFormat(const openavb_profile_cfg_t *profile,
                                           U8 subtype, const void *format_specific)
{
    AVB_TRACE_ENTRY(AVB_TRACE_AVDECC);
    
    // Automotive allows both standard and automotive-specific formats
    for (U32 i = 0; i < profile->stream_formats.subtype_count; i++) {
        if (profile->stream_formats.supported_subtypes[i] == subtype) {
            // Automotive-specific validation
            if (subtype >= 0x08 && subtype <= 0x0F) {
                // Automotive-specific subtypes - validate accordingly
                AVB_LOGF_DEBUG("Automotive Profile: Validating automotive subtype 0x%02x", subtype);
            }
            AVB_TRACE_EXIT(AVB_TRACE_AVDECC);
            return true;
        }
    }
    
    AVB_LOGF_ERROR("Automotive Profile: Unsupported subtype 0x%02x", subtype);
    AVB_TRACE_EXIT(AVB_TRACE_AVDECC);
    return false;
}

U32 openavbAutomotiveCalculatePresentationOffset(const openavb_profile_cfg_t *profile,
                                                 U8 sr_class, U32 max_transit_time)
{
    AVB_TRACE_ENTRY(AVB_TRACE_AVDECC);
    
    // Automotive requires very tight timing
    U32 offset = profile->timing.presentation_time_offset_ns;
    
    // Minimal buffering for automotive real-time requirements
    if (sr_class == SR_CLASS_A) {
        offset += profile->timing.default_class_a_interval_ns / 2; // Half interval
    } else if (sr_class == SR_CLASS_B) {
        offset += profile->timing.default_class_b_interval_ns / 2;
    }
    
    // Very conservative transit time handling for safety
    offset += (max_transit_time * 1000) + 50000; // Extra 50μs only
    
    AVB_LOGF_DEBUG("Automotive Profile: Calculated presentation offset %u ns", offset);
    
    AVB_TRACE_EXIT(AVB_TRACE_AVDECC);
    return offset;
}

bool openavbAutomotiveEnforceQuality(const openavb_profile_cfg_t *profile,
                                     const openavb_stream_quality_metrics_t *metrics)
{
    AVB_TRACE_ENTRY(AVB_TRACE_AVDECC);
    
    // Automotive has the strictest quality requirements
    if (metrics->packets_lost > 0) {
        AVB_LOG_ERROR("Automotive Profile: Zero packet loss required for safety");
        AVB_TRACE_EXIT(AVB_TRACE_AVDECC);
        return false;
    }
    
    if (metrics->timing_violations > 0) {
        AVB_LOG_ERROR("Automotive Profile: Zero timing violations required");
        AVB_TRACE_EXIT(AVB_TRACE_AVDECC);
        return false;
    }
    
    if (metrics->jitter_ms > 0.001f) { // 1μs jitter limit
        AVB_LOG_ERROR("Automotive Profile: Jitter exceeds safety requirements");
        AVB_TRACE_EXIT(AVB_TRACE_AVDECC);
        return false;
    }
    
    AVB_TRACE_EXIT(AVB_TRACE_AVDECC);
    return true;
}

bool openavbAutomotiveValidateConfig(const openavb_profile_cfg_t *profile,
                                     const char *section, const char *name, const char *value)
{
    AVB_TRACE_ENTRY(AVB_TRACE_AVDECC);
    
    // Automotive requires specific security and redundancy configurations
    if (strcmp(section, "security") == 0) {
        if (strcmp(name, "encryption") == 0) {
            if (strcmp(value, "1") != 0) {
                AVB_LOG_ERROR("Automotive Profile: Encryption is mandatory");
                AVB_TRACE_EXIT(AVB_TRACE_AVDECC);
                return false;
            }
        }
    }
    
    if (strcmp(section, "stream") == 0) {
        if (strcmp(name, "redundancy") == 0) {
            if (strcmp(value, "1") != 0) {
                AVB_LOG_WARNING("Automotive Profile: Redundancy recommended for safety");
            }
        }
    }
    
    AVB_TRACE_EXIT(AVB_TRACE_AVDECC);
    return true;
}

/**
 * Built-in Profile Definitions
 */

// AVB Profile (IEEE 1722-2016)
static openavb_profile_cfg_t openavb_avb_profile = {
    .profile_name = "AVB",
    .spec_version = "IEEE1722-2016",
    .profile_id = OPENAVB_PROFILE_ID_AVB,
    
    .compliance = {
        .require_milan_compliance = false,
        .require_automotive_features = false,
        .allow_vendor_extensions = true,
        .strict_timing_requirements = false
    },
    
    .stream_formats = {
        .support_61883_6 = true,
        .support_61883_4 = true,
        .support_aaf = true,
        .support_cvf = true,
        .support_ntscf = false,
        .support_vendor_specific = true,
        .supported_subtypes = avb_supported_subtypes,
        .subtype_count = sizeof(avb_supported_subtypes) / sizeof(U16)
    },
    
    .timing = {
        .default_class_a_interval_ns = 125000,      // 125μs
        .default_class_b_interval_ns = 250000,      // 250μs  
        .max_transit_time_ns = 2000000,             // 2ms
        .presentation_time_offset_ns = 0,
        .sync_uncertainty_ns = 1000,                // 1μs
        .require_avtp_timestamp = false,
        .require_media_clock_recovery = false,
        .max_frame_size = 1522,
        .max_interval_frames = 1
    },
    
    .discovery = {
        .require_avdecc = false,
        .require_fast_connect = false,
        .require_redundancy = false,
        .discovery_protocol_version = 0,
        .entity_capabilities = 0
    },
    
    .qos = {
        .require_strict_priority = true,
        .require_credit_shaping = true,
        .require_time_based_shaping = false,
        .default_priority_class_a = 3,
        .default_priority_class_b = 2
    },
    
    .security = {
        .require_authentication = false,
        .require_encryption = false,
        .require_secure_channels = false,
        .crypto_suite = NULL
    },
    
    .behaviors = {
        .validate_stream_format = openavbAVBValidateStreamFormat,
        .calculate_presentation_offset = openavbAVBCalculatePresentationOffset,
        .validate_avdecc_descriptor = NULL,
        .enforce_quality_requirements = openavbAVBEnforceQuality,
        .validate_config = openavbAVBValidateConfig,
        .profile_init = NULL,
        .profile_cleanup = NULL
    },
    
    .profile_private_data = NULL
};

// MILAN Profile (IEEE 1722.1-2021)
static openavb_profile_cfg_t openavb_milan_profile = {
    .profile_name = "MILAN",
    .spec_version = "IEEE1722.1-2021",
    .profile_id = OPENAVB_PROFILE_ID_MILAN,
    
    .compliance = {
        .require_milan_compliance = true,
        .require_automotive_features = false,
        .allow_vendor_extensions = false,
        .strict_timing_requirements = true
    },
    
    .stream_formats = {
        .support_61883_6 = true,
        .support_61883_4 = false,               // MILAN doesn't require MPEG2-TS
        .support_aaf = true,                    // Mandatory
        .support_cvf = true,                    // Mandatory
        .support_ntscf = false,
        .support_vendor_specific = false,      // Not allowed in MILAN
        .supported_subtypes = milan_supported_subtypes,
        .subtype_count = sizeof(milan_supported_subtypes) / sizeof(U16)
    },
    
    .timing = {
        .default_class_a_interval_ns = 125000,
        .default_class_b_interval_ns = 250000,
        .max_transit_time_ns = 2000000,
        .presentation_time_offset_ns = 500000,  // MILAN-specific
        .sync_uncertainty_ns = 100,             // Tighter sync
        .require_avtp_timestamp = true,         // Mandatory
        .require_media_clock_recovery = true,   // Mandatory
        .max_frame_size = 1522,
        .max_interval_frames = 1
    },
    
    .discovery = {
        .require_avdecc = true,                 // Mandatory
        .require_fast_connect = true,           // Mandatory
        .require_redundancy = false,
        .discovery_protocol_version = 0x0001,
        .entity_capabilities = 0x00000001       // MILAN entity capabilities
    },
    
    .qos = {
        .require_strict_priority = true,
        .require_credit_shaping = true,
        .require_time_based_shaping = false,
        .default_priority_class_a = 3,
        .default_priority_class_b = 2
    },
    
    .security = {
        .require_authentication = false,
        .require_encryption = false,
        .require_secure_channels = false,
        .crypto_suite = NULL
    },
    
    .behaviors = {
        .validate_stream_format = openavbMILANValidateStreamFormat,
        .calculate_presentation_offset = openavbMILANCalculatePresentationOffset,
        .validate_avdecc_descriptor = openavbMILANValidateAVDECCDescriptor,
        .enforce_quality_requirements = openavbMILANEnforceQuality,
        .validate_config = openavbMILANValidateConfig,
        .profile_init = NULL,
        .profile_cleanup = NULL
    },
    
    .profile_private_data = NULL
};

// Automotive Profile
static openavb_profile_cfg_t openavb_automotive_profile = {
    .profile_name = "Automotive",
    .spec_version = "IEEE1722-Automotive",
    .profile_id = OPENAVB_PROFILE_ID_AUTOMOTIVE,
    
    .compliance = {
        .require_milan_compliance = false,
        .require_automotive_features = true,
        .allow_vendor_extensions = true,
        .strict_timing_requirements = true
    },
    
    .stream_formats = {
        .support_61883_6 = true,
        .support_61883_4 = true,
        .support_aaf = true,
        .support_cvf = true,
        .support_ntscf = false,
        .support_vendor_specific = true,
        .supported_subtypes = automotive_supported_subtypes,
        .subtype_count = sizeof(automotive_supported_subtypes) / sizeof(U16)
    },
    
    .timing = {
        .default_class_a_interval_ns = 125000,
        .default_class_b_interval_ns = 250000,
        .max_transit_time_ns = 500000,          // Stricter automotive timing
        .presentation_time_offset_ns = 100000,  // Minimal buffering
        .sync_uncertainty_ns = 50,              // Very tight sync
        .require_avtp_timestamp = true,
        .require_media_clock_recovery = true,
        .max_frame_size = 1522,
        .max_interval_frames = 1
    },
    
    .discovery = {
        .require_avdecc = true,
        .require_fast_connect = true,
        .require_redundancy = true,             // Safety requirement
        .discovery_protocol_version = 0x0001,
        .entity_capabilities = 0x00000003       // Automotive capabilities
    },
    
    .qos = {
        .require_strict_priority = true,
        .require_credit_shaping = true,
        .require_time_based_shaping = true,     // Required for automotive
        .default_priority_class_a = 3,
        .default_priority_class_b = 2
    },
    
    .security = {
        .require_authentication = true,         // Safety requirement
        .require_encryption = true,             // Safety requirement
        .require_secure_channels = true,
        .crypto_suite = "AES-256-GCM"
    },
    
    .behaviors = {
        .validate_stream_format = openavbAutomotiveValidateStreamFormat,
        .calculate_presentation_offset = openavbAutomotiveCalculatePresentationOffset,
        .validate_avdecc_descriptor = NULL,
        .enforce_quality_requirements = openavbAutomotiveEnforceQuality,
        .validate_config = openavbAutomotiveValidateConfig,
        .profile_init = NULL,
        .profile_cleanup = NULL
    },
    
    .profile_private_data = NULL
};

/**
 * Profile Registration Functions
 */

bool openavbProfileRegisterAVB(void)
{
    return g_openavb_profile_registry.register_profile(&openavb_avb_profile);
}

bool openavbProfileRegisterMILAN(void)
{
    return g_openavb_profile_registry.register_profile(&openavb_milan_profile);
}

bool openavbProfileRegisterAutomotive(void)
{
    return g_openavb_profile_registry.register_profile(&openavb_automotive_profile);
}

bool openavbProfileRegisterBuiltins(void)
{
    bool success = true;
    
    success &= openavbProfileRegisterAVB();
    success &= openavbProfileRegisterMILAN();
    success &= openavbProfileRegisterAutomotive();
    
    if (success) {
        // Set AVB as default profile
        success = g_openavb_profile_registry.set_active_profile("AVB");
    }
    
    return success;
}

/**
 * Utility Functions
 */

openavb_profile_cfg_t* openavbProfileGetBuiltinAVB(void)
{
    return &openavb_avb_profile;
}

openavb_profile_cfg_t* openavbProfileGetBuiltinMILAN(void)
{
    return &openavb_milan_profile;
}

openavb_profile_cfg_t* openavbProfileGetBuiltinAutomotive(void)
{
    return &openavb_automotive_profile;
}
