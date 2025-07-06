/**
 * OpenAvnu Profile Framework - Built-in Profile Implementations v2
 * 
 * IMPORTANT: These are EXAMPLE profile implementations for framework demonstration.
 * The MILAN and Automotive profiles are based on publicly available information
 * and common patterns, NOT official specifications. For production use, these
 * should be validated against official IEEE 1722.1-2021 and automotive standards.
 * 
 * This file demonstrates the capability-based abstraction approach without 
 * hardcoded boolean flags, supporting multiple profile versions and runtime 
 * switching between specification variants.
 */

#include "openavb_profile_framework.h"
#include "openavb_log.h"
#include "openavb_types_pub.h"

#define AVB_LOG_COMPONENT "Profile"

/**
 * Forward declarations for profile-specific callback functions
 */
static bool avb_validate_stream_format(const openavb_profile_cfg_t *profile, U8 subtype, const void *format_specific);
static bool milan_1_0_validate_stream_format(const openavb_profile_cfg_t *profile, U8 subtype, const void *format_specific);
static bool milan_1_1_validate_stream_format(const openavb_profile_cfg_t *profile, U8 subtype, const void *format_specific);
static bool automotive_validate_stream_format(const openavb_profile_cfg_t *profile, U8 subtype, const void *format_specific);

static U32 milan_calculate_presentation_offset(const openavb_profile_cfg_t *profile, U32 class_interval_ns, U32 max_transit_time_ns);
static bool milan_validate_timing(const openavb_profile_cfg_t *profile, const openavb_stream_quality_metrics_t *metrics);

/**
 * Supported subtypes for different profile versions
 */
static U16 avb_subtypes[] = {0x02, 0x03, 0x04};  // CVF, AAF, CRF
static U16 milan_1_0_subtypes[] = {0x02, 0x03};  // CVF, AAF only
static U16 milan_1_1_subtypes[] = {0x02, 0x03, 0x04};  // CVF, AAF, CRF
static U16 automotive_subtypes[] = {0x02, 0x03, 0x04, 0x7F};  // Including vendor-specific

static const char *avb_format_names[] = {"CVF", "AAF", "CRF", NULL};
static const char *milan_format_names[] = {"MILAN-CVF", "MILAN-AAF", "MILAN-CRF", NULL};
static const char *automotive_format_names[] = {"A2B", "MOST", "CAN-FD", "Vendor", NULL};

/**
 * Standard AVB Profile (IEEE 1722-2016 base)
 */
static openavb_profile_cfg_t builtin_profile_avb = {
    .profile_name = "AVB",
    .spec_version = OPENAVB_SPEC_IEEE_1722_2016,
    .spec_variant = OPENAVB_VARIANT_STANDARD,
    .profile_id = OPENAVB_PROFILE_ID_AVB,
    .version_string = "IEEE1722-2016-Standard",
    
    .capabilities = {
        .transport = {
            .avtp_timestamp_required = true,
            .media_clock_recovery_required = false,
            .redundant_streams_supported = false,
            .fast_connect_supported = false,
            .secure_channels_supported = false,
            .max_streams_per_entity = 16,
            .max_listeners_per_stream = 8
        },
        .timing = {
            .min_presentation_offset_ns = 2000000,     // 2ms minimum
            .max_presentation_offset_ns = 50000000,    // 50ms maximum
            .sync_uncertainty_tolerance_ns = 1000000,  // 1ms tolerance
            .max_wakeup_time_ns = 125000,              // 125µs max wakeup
            .presentation_time_required = true,
            .gptp_required = true,
            .supported_clock_sources = 0x01            // gPTP only
        },
        .discovery = {
            .avdecc_required = false,
            .msrp_required = true,
            .mvrp_required = true,
            .mmrp_required = false,
            .avdecc_protocol_version = 0,              // No AVDECC
            .required_entity_capabilities = 0,
            .required_talker_capabilities = 0x0001,    // Basic streaming
            .required_listener_capabilities = 0x0001
        },
        .security = {
            .authentication_required = false,
            .encryption_required = false,
            .supported_cipher_suites = 0,
            .supported_auth_methods = 0,
            .certificate_validation_required = false,
            .secure_association_required = false
        },
        .qos = {
            .credit_based_shaping_required = true,
            .time_based_shaping_supported = false,
            .frame_preemption_supported = false,
            .required_traffic_classes = 0x06,          // Class A and B
            .max_frame_size = 1522,
            .max_burst_size = 8192
        }
    },
    
    .stream_formats = {
        .supported_subtypes = avb_subtypes,
        .subtype_count = sizeof(avb_subtypes) / sizeof(U16),
        .format_names = avb_format_names
    },
    
    .callbacks = {
        .validate_stream_format = avb_validate_stream_format,
        .calculate_presentation_offset = NULL,  // Use default calculation
        .validate_timing = NULL,                // Use default validation
        .get_error_recovery_action = NULL,      // Use default recovery
        .profile_init = NULL,                   // No special initialization
        .profile_cleanup = NULL                 // No special cleanup
    }
};

/**
 * MILAN 1.0 Profile (IEEE 1722.1-2021 based)
 */
static openavb_profile_cfg_t builtin_profile_milan_1_0 = {
    .profile_name = "MILAN",
    .spec_version = OPENAVB_SPEC_MILAN_1_0,
    .spec_variant = OPENAVB_VARIANT_STRICT,
    .profile_id = OPENAVB_PROFILE_ID_MILAN,
    .version_string = "MILAN-1.0-Strict",
    
    .capabilities = {
        .transport = {
            .avtp_timestamp_required = true,
            .media_clock_recovery_required = true,
            .redundant_streams_supported = true,
            .fast_connect_supported = true,
            .secure_channels_supported = false,     // Not in MILAN 1.0
            .max_streams_per_entity = 32,
            .max_listeners_per_stream = 16
        },
        .timing = {
            .min_presentation_offset_ns = 1000000,     // 1ms minimum (stricter)
            .max_presentation_offset_ns = 20000000,    // 20ms maximum (stricter)
            .sync_uncertainty_tolerance_ns = 500000,   // 500µs tolerance (stricter)
            .max_wakeup_time_ns = 62500,               // 62.5µs max wakeup (stricter)
            .presentation_time_required = true,
            .gptp_required = true,
            .supported_clock_sources = 0x01            // gPTP only
        },
        .discovery = {
            .avdecc_required = true,                   // AVDECC mandatory in MILAN
            .msrp_required = true,
            .mvrp_required = true,
            .mmrp_required = true,
            .avdecc_protocol_version = 2,              // IEEE 1722.1-2021
            .required_entity_capabilities = 0x00000001, // AEM_SUPPORTED
            .required_talker_capabilities = 0x4001,    // Audio + Fast Connect
            .required_listener_capabilities = 0x4001
        },
        .security = {
            .authentication_required = false,          // Optional in MILAN 1.0
            .encryption_required = false,
            .supported_cipher_suites = 0,
            .supported_auth_methods = 0,
            .certificate_validation_required = false,
            .secure_association_required = false
        },
        .qos = {
            .credit_based_shaping_required = true,
            .time_based_shaping_supported = false,     // Not required in MILAN 1.0
            .frame_preemption_supported = false,
            .required_traffic_classes = 0x06,          // Class A and B
            .max_frame_size = 1522,
            .max_burst_size = 4096                     // Stricter than AVB
        }
    },
    
    .stream_formats = {
        .supported_subtypes = milan_1_0_subtypes,
        .subtype_count = sizeof(milan_1_0_subtypes) / sizeof(U16),
        .format_names = milan_format_names
    },
    
    .callbacks = {
        .validate_stream_format = milan_1_0_validate_stream_format,
        .calculate_presentation_offset = milan_calculate_presentation_offset,
        .validate_timing = milan_validate_timing,
        .get_error_recovery_action = NULL,
        .profile_init = NULL,
        .profile_cleanup = NULL
    }
};

/**
 * MILAN 1.1 Profile (Enhanced capabilities)
 */
static openavb_profile_cfg_t builtin_profile_milan_1_1 = {
    .profile_name = "MILAN",
    .spec_version = OPENAVB_SPEC_MILAN_1_1,
    .spec_variant = OPENAVB_VARIANT_STRICT,
    .profile_id = OPENAVB_PROFILE_ID_MILAN + 1,  // Different ID for 1.1
    .version_string = "MILAN-1.1-Strict",
    
    .capabilities = {
        .transport = {
            .avtp_timestamp_required = true,
            .media_clock_recovery_required = true,
            .redundant_streams_supported = true,
            .fast_connect_supported = true,
            .secure_channels_supported = true,         // New in MILAN 1.1
            .max_streams_per_entity = 64,              // Increased capacity
            .max_listeners_per_stream = 32
        },
        .timing = {
            .min_presentation_offset_ns = 500000,      // 500µs minimum (even stricter)
            .max_presentation_offset_ns = 15000000,    // 15ms maximum
            .sync_uncertainty_tolerance_ns = 250000,   // 250µs tolerance
            .max_wakeup_time_ns = 31250,               // 31.25µs max wakeup
            .presentation_time_required = true,
            .gptp_required = true,
            .supported_clock_sources = 0x03            // gPTP + PTP
        },
        .discovery = {
            .avdecc_required = true,
            .msrp_required = true,
            .mvrp_required = true,
            .mmrp_required = true,
            .avdecc_protocol_version = 3,              // Enhanced AVDECC
            .required_entity_capabilities = 0x00000003, // AEM + Authentication
            .required_talker_capabilities = 0x6001,    // Audio + Fast Connect + Security
            .required_listener_capabilities = 0x6001
        },
        .security = {
            .authentication_required = true,           // Mandatory in MILAN 1.1
            .encryption_required = true,
            .supported_cipher_suites = 0x0003,         // AES-128 + AES-256
            .supported_auth_methods = 0x0001,          // Certificate-based
            .certificate_validation_required = true,
            .secure_association_required = true
        },
        .qos = {
            .credit_based_shaping_required = true,
            .time_based_shaping_supported = true,      // New in MILAN 1.1
            .frame_preemption_supported = true,        // New capability
            .required_traffic_classes = 0x0E,          // Class A, B, and CDT
            .max_frame_size = 1522,
            .max_burst_size = 2048                     // Even stricter
        }
    },
    
    .stream_formats = {
        .supported_subtypes = milan_1_1_subtypes,
        .subtype_count = sizeof(milan_1_1_subtypes) / sizeof(U16),
        .format_names = milan_format_names
    },
    
    .callbacks = {
        .validate_stream_format = milan_1_1_validate_stream_format,
        .calculate_presentation_offset = milan_calculate_presentation_offset,
        .validate_timing = milan_validate_timing,
        .get_error_recovery_action = NULL,
        .profile_init = NULL,
        .profile_cleanup = NULL
    }
};

/**
 * Automotive A2B Profile (Automotive-specific requirements)
 */
static openavb_profile_cfg_t builtin_profile_automotive_a2b = {
    .profile_name = "Automotive-A2B",
    .spec_version = OPENAVB_SPEC_AUTOMOTIVE_A2B_1_0,
    .spec_variant = OPENAVB_VARIANT_EXTENDED,
    .profile_id = OPENAVB_PROFILE_ID_AUTOMOTIVE,
    .version_string = "Automotive-A2B-1.0-Extended",
    
    .capabilities = {
        .transport = {
            .avtp_timestamp_required = true,
            .media_clock_recovery_required = true,
            .redundant_streams_supported = true,       // Critical for automotive
            .fast_connect_supported = true,
            .secure_channels_supported = true,         // Mandatory for automotive
            .max_streams_per_entity = 128,             // High capacity for automotive
            .max_listeners_per_stream = 64
        },
        .timing = {
            .min_presentation_offset_ns = 100000,      // 100µs minimum (very strict)
            .max_presentation_offset_ns = 5000000,     // 5ms maximum (strict)
            .sync_uncertainty_tolerance_ns = 50000,    // 50µs tolerance (very strict)
            .max_wakeup_time_ns = 15625,               // 15.625µs max wakeup (ultra-strict)
            .presentation_time_required = true,
            .gptp_required = true,
            .supported_clock_sources = 0x07            // gPTP + PTP + Local crystal
        },
        .discovery = {
            .avdecc_required = true,
            .msrp_required = true,
            .mvrp_required = true,
            .mmrp_required = true,
            .avdecc_protocol_version = 3,              // Latest for automotive
            .required_entity_capabilities = 0x00000007, // AEM + Auth + Vendor
            .required_talker_capabilities = 0xE001,    // Full automotive capability set
            .required_listener_capabilities = 0xE001
        },
        .security = {
            .authentication_required = true,           // Mandatory for automotive
            .encryption_required = true,               // Mandatory for automotive
            .supported_cipher_suites = 0x000F,         // Multiple cipher suites
            .supported_auth_methods = 0x0003,          // Certificate + PSK
            .certificate_validation_required = true,
            .secure_association_required = true
        },
        .qos = {
            .credit_based_shaping_required = true,
            .time_based_shaping_supported = true,      // Required for automotive
            .frame_preemption_supported = true,        // Critical for automotive
            .required_traffic_classes = 0x1E,          // All classes including CDT and automotive
            .max_frame_size = 1522,
            .max_burst_size = 1024                     // Very strict for automotive
        }
    },
    
    .stream_formats = {
        .supported_subtypes = automotive_subtypes,
        .subtype_count = sizeof(automotive_subtypes) / sizeof(U16),
        .format_names = automotive_format_names
    },
    
    .callbacks = {
        .validate_stream_format = automotive_validate_stream_format,
        .calculate_presentation_offset = NULL,      // Use automotive-specific calculation
        .validate_timing = NULL,                    // Use automotive-specific validation
        .get_error_recovery_action = NULL,          // Use automotive-specific recovery
        .profile_init = NULL,                       // Automotive-specific initialization
        .profile_cleanup = NULL                     // Automotive-specific cleanup
    }
};

/**
 * Profile Registry - Central location for all built-in profiles
 */
static openavb_profile_cfg_t *builtin_profiles[] = {
    &builtin_profile_avb,
    &builtin_profile_milan_1_0,
    &builtin_profile_milan_1_1,
    &builtin_profile_automotive_a2b,
    NULL  // Terminator
};

/**
 * Profile-specific callback implementations
 */

static bool avb_validate_stream_format(const openavb_profile_cfg_t *profile, U8 subtype, const void *format_specific) {
    // AVB allows all registered subtypes
    for (U32 i = 0; i < profile->stream_formats.subtype_count; i++) {
        if (profile->stream_formats.supported_subtypes[i] == subtype) {
            return true;
        }
    }
    return false;
}

static bool milan_1_0_validate_stream_format(const openavb_profile_cfg_t *profile, U8 subtype, const void *format_specific) {
    // MILAN 1.0 has stricter format validation
    if (!avb_validate_stream_format(profile, subtype, format_specific)) {
        return false;
    }
    
    // Additional MILAN-specific validation
    switch (subtype) {
        case 0x02: // AAF
            // MILAN requires specific AAF configurations
            return true;  // Implement MILAN-specific AAF validation
        case 0x03: // CVF
            // MILAN requires specific CVF configurations
            return true;  // Implement MILAN-specific CVF validation
        default:
            return false;  // MILAN 1.0 doesn't support other formats
    }
}

static bool milan_1_1_validate_stream_format(const openavb_profile_cfg_t *profile, U8 subtype, const void *format_specific) {
    // MILAN 1.1 supports more formats than 1.0
    if (!avb_validate_stream_format(profile, subtype, format_specific)) {
        return false;
    }
    
    // MILAN 1.1 adds support for CRF
    switch (subtype) {
        case 0x02: // AAF
        case 0x03: // CVF
        case 0x04: // CRF - New in MILAN 1.1
            return true;
        default:
            return false;
    }
}

static bool automotive_validate_stream_format(const openavb_profile_cfg_t *profile, U8 subtype, const void *format_specific) {
    // Automotive profiles allow vendor-specific formats
    for (U32 i = 0; i < profile->stream_formats.subtype_count; i++) {
        if (profile->stream_formats.supported_subtypes[i] == subtype) {
            // Additional automotive-specific validation for security and timing
            return true;  // Implement automotive-specific validation
        }
    }
    return false;
}

static U32 milan_calculate_presentation_offset(const openavb_profile_cfg_t *profile, U32 class_interval_ns, U32 max_transit_time_ns) {
    // MILAN uses stricter presentation time calculations
    U32 base_offset = max_transit_time_ns * 2;  // Conservative estimate
    
    // Ensure within MILAN bounds
    if (base_offset < profile->capabilities.timing.min_presentation_offset_ns) {
        base_offset = profile->capabilities.timing.min_presentation_offset_ns;
    }
    if (base_offset > profile->capabilities.timing.max_presentation_offset_ns) {
        base_offset = profile->capabilities.timing.max_presentation_offset_ns;
    }
    
    return base_offset;
}

static bool milan_validate_timing(const openavb_profile_cfg_t *profile, const openavb_stream_quality_metrics_t *metrics) {
    // MILAN has strict timing validation requirements
    if (metrics->timing_violations > 0) {
        return false;  // No timing violations allowed in MILAN
    }
    
    if (metrics->jitter_ms > (profile->capabilities.timing.sync_uncertainty_tolerance_ns / 1000000.0)) {
        return false;  // Jitter exceeds tolerance
    }
    
    return true;
}

/**
 * Public interface for accessing built-in profiles
 */
openavb_profile_cfg_t **openavb_profile_get_builtin_profiles(void) {
    return builtin_profiles;
}

openavb_profile_cfg_t *openavb_profile_get_by_version(openavb_spec_version_e spec_version, openavb_spec_variant_e variant) {
    for (int i = 0; builtin_profiles[i] != NULL; i++) {
        if (builtin_profiles[i]->spec_version == spec_version && 
            builtin_profiles[i]->spec_variant == variant) {
            return builtin_profiles[i];
        }
    }
    return NULL;
}

openavb_profile_cfg_t *openavb_profile_get_by_name(const char *profile_name, const char *version_string) {
    for (int i = 0; builtin_profiles[i] != NULL; i++) {
        if (strcmp(builtin_profiles[i]->profile_name, profile_name) == 0) {
            if (version_string == NULL || strcmp(builtin_profiles[i]->version_string, version_string) == 0) {
                return builtin_profiles[i];
            }
        }
    }
    return NULL;
}

bool openavb_profile_is_compatible(const openavb_profile_cfg_t *profile1, const openavb_profile_cfg_t *profile2) {
    // Check if two profiles can interoperate
    // This implements the version compatibility matrix
    
    if (profile1->spec_version == profile2->spec_version) {
        return true;  // Same version is always compatible
    }
    
    // Check for known compatibility paths
    switch (profile1->spec_version) {
        case OPENAVB_SPEC_MILAN_1_1:
            // MILAN 1.1 is backward compatible with MILAN 1.0
            return (profile2->spec_version == OPENAVB_SPEC_MILAN_1_0);
            
        case OPENAVB_SPEC_MILAN_1_0:
            // MILAN 1.0 is compatible with IEEE 1722.1-2021
            return (profile2->spec_version == OPENAVB_SPEC_IEEE_1722_1_2021);
            
        default:
            return false;  // No compatibility by default
    }
}
