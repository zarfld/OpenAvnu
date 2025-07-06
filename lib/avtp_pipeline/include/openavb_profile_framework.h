/**
 * OpenAvnu Profile Framework - Core Implementation
 * 
 * This file demonstrates how the profile framework integrates with
 * the existing OpenAvnu architecture, providing centralized profile
 * management without hardcoded switches.
 */

#ifndef OPENAVB_PROFILE_FRAMEWORK_H
#define OPENAVB_PROFILE_FRAMEWORK_H

#include "openavb_types_pub.h"
#include "openavb_mediaq_pub.h"
#include "openavb_intf_pub.h"
#include "openavb_map_pub.h"

#ifdef __cplusplus
extern "C" {
#endif

// Profile IDs for built-in profiles
#define OPENAVB_PROFILE_ID_AVB          0x0001
#define OPENAVB_PROFILE_ID_MILAN        0x0002
#define OPENAVB_PROFILE_ID_AUTOMOTIVE   0x0003
#define OPENAVB_PROFILE_ID_CUSTOM_BASE  0x8000

/**
 * Stream quality metrics for profile enforcement
 */
typedef struct openavb_stream_quality_metrics {
    U32 packets_sent;
    U32 packets_lost;
    U32 late_packets;
    U32 timing_violations;
    float jitter_ms;
    float sync_accuracy_ppm;
    U32 buffer_underruns;
    U32 buffer_overruns;
} openavb_stream_quality_metrics_t;

/**
 * Specification and version abstraction
 * This allows proper handling of multiple spec versions and variants
 */
typedef enum {
    OPENAVB_SPEC_IEEE_1722_2011,
    OPENAVB_SPEC_IEEE_1722_2016,
    OPENAVB_SPEC_IEEE_1722_1_2013,
    OPENAVB_SPEC_IEEE_1722_1_2021,
    OPENAVB_SPEC_MILAN_1_0,
    OPENAVB_SPEC_MILAN_1_1,
    OPENAVB_SPEC_AUTOMOTIVE_A2B_1_0,
    OPENAVB_SPEC_AUTOMOTIVE_MOST_1_5,
    OPENAVB_SPEC_AUTOMOTIVE_SOME_IP_1_3,
    OPENAVB_SPEC_CUSTOM_BASE = 0x8000
} openavb_spec_version_e;

typedef enum {
    OPENAVB_VARIANT_STANDARD,           // Standard implementation
    OPENAVB_VARIANT_STRICT,             // Strict compliance mode
    OPENAVB_VARIANT_EXTENDED,           // With vendor extensions
    OPENAVB_VARIANT_INTEROP,            // Interoperability mode
    OPENAVB_VARIANT_LEGACY              // Legacy compatibility
} openavb_spec_variant_e;

/**
 * Capability matrix for version-specific features
 * This replaces hardcoded boolean flags with versioned capabilities
 */
typedef struct openavb_capability_matrix {
    /// Core transport capabilities
    struct {
        bool avtp_timestamp_required;
        bool media_clock_recovery_required;
        bool redundant_streams_supported;
        bool fast_connect_supported;
        bool secure_channels_supported;
        U32 max_streams_per_entity;
        U32 max_listeners_per_stream;
    } transport;
    
    /// Timing and synchronization capabilities
    struct {
        U32 min_presentation_offset_ns;
        U32 max_presentation_offset_ns;
        U32 sync_uncertainty_tolerance_ns;
        U32 max_wakeup_time_ns;
        bool presentation_time_required;
        bool gptp_required;
        U8 supported_clock_sources;        // Bitmask of supported sources
    } timing;
    
    /// Discovery and control capabilities
    struct {
        bool avdecc_required;
        bool msrp_required;
        bool mvrp_required;
        bool mmrp_required;
        U16 avdecc_protocol_version;
        U32 required_entity_capabilities;
        U32 required_talker_capabilities;
        U32 required_listener_capabilities;
    } discovery;
    
    /// Security capabilities (version-specific)
    struct {
        bool authentication_required;
        bool encryption_required;
        U32 supported_cipher_suites;       // Bitmask of supported ciphers
        U32 supported_auth_methods;        // Bitmask of auth methods
        bool certificate_validation_required;
        bool secure_association_required;
    } security;
    
    /// Quality of Service capabilities
    struct {
        bool credit_based_shaping_required;
        bool time_based_shaping_supported;
        bool frame_preemption_supported;
        U8 required_traffic_classes;       // Bitmask
        U32 max_frame_size;
        U32 max_burst_size;
    } qos;
    
} openavb_capability_matrix_t;

/**
 * Core profile configuration structure with proper abstraction
 * Replaces hardcoded flags with versioned specifications
 */
typedef struct openavb_profile_cfg {
    /// Profile identification with proper versioning
    const char *profile_name;           // "AVB", "MILAN", "Automotive", "Custom"
    openavb_spec_version_e spec_version; // Enumerated specification version
    openavb_spec_variant_e spec_variant; // Implementation variant
    U32 profile_id;                     // Unique numeric identifier
    const char *version_string;         // Human-readable: "MILAN-1.1-Strict"
    
    /// Version-specific capability matrix (replaces hardcoded flags)
    openavb_capability_matrix_t capabilities;
    
    /// Supported stream formats (derived from spec version)
    struct {
        U16 *supported_subtypes;        // Array of supported AVTP subtypes
        U32 subtype_count;
        const char **format_names;      // Human-readable format names
    } stream_formats;
    
    /// Profile-specific behavior callbacks
    struct {
        /// Validate stream format against profile requirements
        bool (*validate_stream_format)(const struct openavb_profile_cfg *profile,
                                       U8 subtype, const void *format_specific);
        
        /// Calculate profile-specific presentation time offset
        U32 (*calculate_presentation_offset)(const struct openavb_profile_cfg *profile,
                                             U32 class_interval_ns, U32 max_transit_time_ns);
        
        /// Check timing requirements compliance
        bool (*validate_timing)(const struct openavb_profile_cfg *profile,
                               const openavb_stream_quality_metrics_t *metrics);
        
        /// Get profile-specific error recovery actions
        U32 (*get_error_recovery_action)(const struct openavb_profile_cfg *profile,
                                         U32 error_type, U32 error_count);
        
        /// Custom initialization for profile-specific features
        bool (*profile_init)(const struct openavb_profile_cfg *profile,
                            void *context);
        
        /// Custom cleanup for profile-specific features
        void (*profile_cleanup)(const struct openavb_profile_cfg *profile,
                               void *context);
    } callbacks;
                                             U8 sr_class, U32 max_transit_time);
        
        /// Validate AVDECC descriptor compliance
        bool (*validate_avdecc_descriptor)(const struct openavb_profile_cfg *profile,
                                           U16 descriptor_type, const void *descriptor);
        
        /// Enforce profile-specific quality requirements
        bool (*enforce_quality_requirements)(const struct openavb_profile_cfg *profile,
                                             const openavb_stream_quality_metrics_t *metrics);
        
        /// Validate configuration parameters
        bool (*validate_config)(const struct openavb_profile_cfg *profile,
                               const char *section, const char *name, const char *value);
        
        /// Profile-specific initialization
        bool (*profile_init)(struct openavb_profile_cfg *profile);
        
        /// Profile-specific cleanup
        void (*profile_cleanup)(struct openavb_profile_cfg *profile);
    } behaviors;
    
    // Extension point for profile-specific private data
    void *profile_private_data;
    
} openavb_profile_cfg_t;

/**
 * Profile registry for centralized profile management
 */
typedef struct openavb_profile_registry {
    openavb_profile_cfg_t **profiles;
    U32 profile_count;
    U32 max_profiles;
    openavb_profile_cfg_t *active_profile;
    
    // Management functions
    bool (*register_profile)(openavb_profile_cfg_t *profile);
    openavb_profile_cfg_t* (*get_profile_by_name)(const char *profile_name);
    openavb_profile_cfg_t* (*get_profile_by_id)(U32 profile_id);
    bool (*set_active_profile)(const char *profile_name);
    bool (*validate_profile_compatibility)(openavb_profile_cfg_t *profile1, 
                                           openavb_profile_cfg_t *profile2);
} openavb_profile_registry_t;

/**
 * Enhanced media queue with profile awareness
 */
typedef struct openavb_media_q_profile_ext {
    // Reference to base media queue
    media_q_t *base_media_q;
    
    // Associated profile
    openavb_profile_cfg_t *profile;
    
    // Profile-specific metrics
    openavb_stream_quality_metrics_t quality_metrics;
    
    // Compliance state
    bool profile_compliant;
    const char *compliance_error;
    
    // Profile-specific configuration
    void *profile_config_data;
    
} openavb_media_q_profile_ext_t;

// Global profile registry
extern openavb_profile_registry_t g_openavb_profile_registry;

/**
 * Profile Framework API Functions
 */

// Core profile management
bool openavbProfileFrameworkInit(void);
void openavbProfileFrameworkCleanup(void);

// Built-in profile registration
bool openavbProfileRegisterBuiltins(void);
bool openavbProfileRegisterAVB(void);
bool openavbProfileRegisterMILAN(void);
bool openavbProfileRegisterAutomotive(void);

// Custom profile management
bool openavbProfileRegisterCustom(openavb_profile_cfg_t *custom_profile);
bool openavbProfileUnregister(const char *profile_name);

// Active profile management
bool openavbProfileSetActive(const char *profile_name);
openavb_profile_cfg_t* openavbProfileGetActive(void);
const char* openavbProfileGetActiveName(void);

// Profile validation and compatibility
bool openavbProfileValidateConfig(const char *section, const char *name, const char *value);
bool openavbProfileValidateStreamFormat(U8 subtype, const void *format_specific);
bool openavbProfileValidateQuality(const openavb_stream_quality_metrics_t *metrics);

// Profile-aware media queue extensions
openavb_media_q_profile_ext_t* openavbProfileCreateMediaQExt(media_q_t *base_media_q);
void openavbProfileDestroyMediaQExt(openavb_media_q_profile_ext_t *profile_ext);
bool openavbProfileUpdateQualityMetrics(openavb_media_q_profile_ext_t *profile_ext,
                                         const openavb_stream_quality_metrics_t *metrics);

// Integration with existing interface/mapping system
bool openavbProfileValidateIntfCompatibility(openavb_intf_cb_t *pIntfCB);
bool openavbProfileValidateMapCompatibility(openavb_map_cb_t *pMapCB);

// Configuration integration
typedef struct openavb_profile_config_ctx {
    openavb_profile_cfg_t *profile;
    void *user_data;
    bool profile_validation_enabled;
} openavb_profile_config_ctx_t;

// Profile-aware configuration callback
int openavbProfileConfigCallback(void *user, const char *section, 
                                 const char *name, const char *value);

/**
 * Core Profile Framework APIs
 */

// Framework initialization
bool openavb_profile_framework_init(void);
void openavb_profile_framework_cleanup(void);

// Profile registration and management
bool openavb_profile_register(openavb_profile_cfg_t *profile);
openavb_profile_cfg_t* openavb_profile_get_by_name(const char *name, const char *version);
openavb_profile_cfg_t* openavb_profile_get_by_version(openavb_spec_version_e version, 
                                                      openavb_spec_variant_e variant);
U32 openavb_profile_list_all(const openavb_profile_cfg_t** profiles, U32 max_count);

// Profile compatibility and selection
bool openavb_profile_is_compatible(const openavb_profile_cfg_t *profile1,
                                   const openavb_profile_cfg_t *profile2);
openavb_profile_cfg_t* openavb_profile_select_optimal(const void *requirements);

// Stream operations
bool openavb_stream_validate_config(const openavb_profile_cfg_t *profile, const void *config);
const openavb_capability_matrix_t* openavb_profile_get_capabilities(const openavb_profile_cfg_t *profile);

// Error handling
U32 openavb_stream_handle_error_by_capability(const openavb_profile_cfg_t *profile,
                                              U32 error_type, U32 error_count,
                                              const openavb_stream_quality_metrics_t *metrics);

/**
 * Built-in Profile Behavior Functions
 */

// AVB Profile behaviors
bool openavbAVBValidateStreamFormat(const openavb_profile_cfg_t *profile,
                                    U8 subtype, const void *format_specific);
U32 openavbAVBCalculatePresentationOffset(const openavb_profile_cfg_t *profile,
                                          U8 sr_class, U32 max_transit_time);
bool openavbAVBEnforceQuality(const openavb_profile_cfg_t *profile,
                              const openavb_stream_quality_metrics_t *metrics);
bool openavbAVBValidateConfig(const openavb_profile_cfg_t *profile,
                              const char *section, const char *name, const char *value);

// MILAN Profile behaviors
bool openavbMILANValidateStreamFormat(const openavb_profile_cfg_t *profile,
                                      U8 subtype, const void *format_specific);
U32 openavbMILANCalculatePresentationOffset(const openavb_profile_cfg_t *profile,
                                            U8 sr_class, U32 max_transit_time);
bool openavbMILANValidateAVDECCDescriptor(const openavb_profile_cfg_t *profile,
                                          U16 descriptor_type, const void *descriptor);
bool openavbMILANEnforceQuality(const openavb_profile_cfg_t *profile,
                                const openavb_stream_quality_metrics_t *metrics);
bool openavbMILANValidateConfig(const openavb_profile_cfg_t *profile,
                                const char *section, const char *name, const char *value);

// Automotive Profile behaviors
bool openavbAutomotiveValidateStreamFormat(const openavb_profile_cfg_t *profile,
                                           U8 subtype, const void *format_specific);
U32 openavbAutomotiveCalculatePresentationOffset(const openavb_profile_cfg_t *profile,
                                                 U8 sr_class, U32 max_transit_time);
bool openavbAutomotiveEnforceQuality(const openavb_profile_cfg_t *profile,
                                     const openavb_stream_quality_metrics_t *metrics);
bool openavbAutomotiveValidateConfig(const openavb_profile_cfg_t *profile,
                                     const char *section, const char *name, const char *value);

/**
 * Forward declaration for stream handle
 */
typedef struct openavb_stream_handle openavb_stream_handle_t;

/**
 * Configuration Parser APIs
 */

// Parse configuration file and select profile
bool openavb_profile_parse_config(const char *config_file, openavb_profile_cfg_t **selected_profile);

// Validate configuration file against profile
bool openavb_profile_validate_config_file(const char *config_file, const openavb_profile_cfg_t *profile);

/**
 * Stream Integration APIs
 */

// Stream lifecycle management
openavb_stream_handle_t* openavb_stream_create(void *stream_data);
void openavb_stream_destroy(openavb_stream_handle_t *stream);

// Profile management for streams
bool openavb_stream_set_profile(openavb_stream_handle_t *stream, const openavb_profile_cfg_t *profile);
const openavb_profile_cfg_t* openavb_stream_get_profile(openavb_stream_handle_t *stream);

// Stream configuration and validation
bool openavb_stream_validate_with_profile(openavb_stream_handle_t *stream, const void *config);
bool openavb_stream_configure_from_profile(openavb_stream_handle_t *stream);

// Stream activation and monitoring
bool openavb_stream_activate(openavb_stream_handle_t *stream);
void openavb_stream_deactivate(openavb_stream_handle_t *stream);

// Quality metrics and error handling
void openavb_stream_update_metrics(openavb_stream_handle_t *stream, 
                                  const openavb_stream_quality_metrics_t *metrics);
const openavb_stream_quality_metrics_t* openavb_stream_get_metrics(openavb_stream_handle_t *stream);
bool openavb_stream_handle_error(openavb_stream_handle_t *stream, U32 error_type, U32 error_count);

// Stream registry
openavb_stream_handle_t* openavb_stream_find_by_id(U32 stream_id);

/**
 * Core Profile Framework APIs
 */

// Framework initialization
bool openavb_profile_framework_init(void);
void openavb_profile_framework_cleanup(void);

// Profile registration and management
bool openavb_profile_register(openavb_profile_cfg_t *profile);
openavb_profile_cfg_t* openavb_profile_get_by_name(const char *name, const char *version);
openavb_profile_cfg_t* openavb_profile_get_by_version(openavb_spec_version_e version, 
                                                      openavb_spec_variant_e variant);
U32 openavb_profile_list_all(const openavb_profile_cfg_t** profiles, U32 max_count);

// Profile compatibility and selection
bool openavb_profile_is_compatible(const openavb_profile_cfg_t *profile1,
                                   const openavb_profile_cfg_t *profile2);
openavb_profile_cfg_t* openavb_profile_select_optimal(const void *requirements);

// Stream operations
bool openavb_stream_validate_config(const openavb_profile_cfg_t *profile, const void *config);
const openavb_capability_matrix_t* openavb_profile_get_capabilities(const openavb_profile_cfg_t *profile);

// Error handling
U32 openavb_stream_handle_error_by_capability(const openavb_profile_cfg_t *profile,
                                              U32 error_type, U32 error_count,
                                              const openavb_stream_quality_metrics_t *metrics);

/**
 * Built-in Profile Behavior Functions
 */

// AVB Profile behaviors
bool openavbAVBValidateStreamFormat(const openavb_profile_cfg_t *profile,
                                    U8 subtype, const void *format_specific);
U32 openavbAVBCalculatePresentationOffset(const openavb_profile_cfg_t *profile,
                                          U8 sr_class, U32 max_transit_time);
bool openavbAVBEnforceQuality(const openavb_profile_cfg_t *profile,
                              const openavb_stream_quality_metrics_t *metrics);
bool openavbAVBValidateConfig(const openavb_profile_cfg_t *profile,
                              const char *section, const char *name, const char *value);

// MILAN Profile behaviors
bool openavbMILANValidateStreamFormat(const openavb_profile_cfg_t *profile,
                                      U8 subtype, const void *format_specific);
U32 openavbMILANCalculatePresentationOffset(const openavb_profile_cfg_t *profile,
                                            U8 sr_class, U32 max_transit_time);
bool openavbMILANValidateAVDECCDescriptor(const openavb_profile_cfg_t *profile,
                                          U16 descriptor_type, const void *descriptor);
bool openavbMILANEnforceQuality(const openavb_profile_cfg_t *profile,
                                const openavb_stream_quality_metrics_t *metrics);
bool openavbMILANValidateConfig(const openavb_profile_cfg_t *profile,
                                const char *section, const char *name, const char *value);

// Automotive Profile behaviors
bool openavbAutomotiveValidateStreamFormat(const openavb_profile_cfg_t *profile,
                                           U8 subtype, const void *format_specific);
U32 openavbAutomotiveCalculatePresentationOffset(const openavb_profile_cfg_t *profile,
                                                 U8 sr_class, U32 max_transit_time);
bool openavbAutomotiveEnforceQuality(const openavb_profile_cfg_t *profile,
                                     const openavb_stream_quality_metrics_t *metrics);
bool openavbAutomotiveValidateConfig(const openavb_profile_cfg_t *profile,
                                     const char *section, const char *name, const char *value);

/**
 * Helper Macros for Profile Integration
 */

// Get active profile safely
#define OPENAVB_PROFILE_ACTIVE() \
    (g_openavb_profile_registry.active_profile ? \
     g_openavb_profile_registry.active_profile : \
     openavbProfileGetBuiltinAVB())

// Check if active profile supports a feature
#define OPENAVB_PROFILE_SUPPORTS(feature) \
    (OPENAVB_PROFILE_ACTIVE()->stream_formats.support_##feature)

// Get profile-specific timing parameter
#define OPENAVB_PROFILE_TIMING(param) \
    (OPENAVB_PROFILE_ACTIVE()->timing.param)

// Check profile compliance requirement
#define OPENAVB_PROFILE_REQUIRES(requirement) \
    (OPENAVB_PROFILE_ACTIVE()->compliance.require_##requirement || \
     OPENAVB_PROFILE_ACTIVE()->discovery.require_##requirement)

// Validate against active profile
#define OPENAVB_PROFILE_VALIDATE_CONFIG(section, name, value) \
    openavbProfileValidateConfig(section, name, value)

#define OPENAVB_PROFILE_VALIDATE_STREAM_FORMAT(subtype, format) \
    openavbProfileValidateStreamFormat(subtype, format)

#ifdef __cplusplus
}
#endif

#endif // OPENAVB_PROFILE_FRAMEWORK_H
