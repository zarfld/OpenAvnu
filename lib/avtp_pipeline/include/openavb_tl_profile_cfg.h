/*************************************************************************************************************
Copyright (c) 2012-2015, Symphony Teleca Corporation, a Harman International Industries, Incorporated company
Copyright (c) 2016-2017, Harman International Industries, Incorporated
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS LISTED "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS LISTED BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*************************************************************************************************************/

/*
* HEADER SUMMARY : Profile-Aware Talker Listener Configuration Extension
* 
* Extends the standard TL configuration with AVTP profile support for Milan,
* Automotive, and ProAV compliance. Provides automatic parameter validation,
* constraint enforcement, and runtime profile monitoring.
*/

#ifndef OPENAVB_TL_PROFILE_CFG_H
#define OPENAVB_TL_PROFILE_CFG_H 1

// Standard types for the integration
#include <stdbool.h>
#include <stdint.h>

// Basic OpenAvnu types
typedef uint8_t U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;

// Forward declarations
typedef void *tl_handle_t;

// Profile types from our AVTP Profile System
typedef enum {
    OPENAVB_PROFILE_MILAN = 0,
    OPENAVB_PROFILE_AUTOMOTIVE = 1,
    OPENAVB_PROFILE_PROAV = 2,
    OPENAVB_PROFILE_CUSTOM = 3
} openavb_tl_profile_type_t;

// Simplified configuration base structure
typedef struct {
    // Essential configuration fields for profile integration
    uint32_t max_transit_usec;
    uint16_t stream_uid;
    uint8_t sr_class;
    uint16_t vlan_id;
    char ifname[32];
    char friendly_name[64];
    bool start_paused;
} openavb_tl_base_cfg_t;

/** \file
 * Profile-Aware Talker Listener Configuration Extension.
 * 
 * This module extends the standard OpenAvnu TL configuration with comprehensive
 * AVTP profile support, enabling automatic compliance validation and optimization
 * for industry-standard profiles including Milan, Automotive, and ProAV.
 */

/// Maximum length for profile configuration file path
#define OPENAVB_PROFILE_CONFIG_PATH_MAX 256

/// Maximum number of profile-specific validation rules
#define OPENAVB_PROFILE_MAX_VALIDATION_RULES 32

/// Profile validation severity levels
typedef enum {
    OPENAVB_PROFILE_VALIDATION_INFO,     ///< Informational message
    OPENAVB_PROFILE_VALIDATION_WARNING,  ///< Warning - stream may not be optimal
    OPENAVB_PROFILE_VALIDATION_ERROR,    ///< Error - stream will not comply with profile
    OPENAVB_PROFILE_VALIDATION_CRITICAL  ///< Critical - stream cannot be created
} openavb_profile_validation_severity_t;

/// Profile validation result structure
typedef struct {
    openavb_profile_validation_severity_t severity;
    char rule_name[64];
    char message[256];
    U32 error_code;
    bool auto_correctable;
} openavb_profile_validation_issue_t;

/// Comprehensive profile validation results
typedef struct {
    bool is_compliant;
    bool has_warnings;
    bool has_auto_corrections;
    U32 issue_count;
    openavb_profile_validation_issue_t issues[OPENAVB_PROFILE_MAX_VALIDATION_RULES];
    
    // Compliance metrics
    U32 latency_compliance_usec;
    bool format_compliance;
    bool timing_compliance;
    bool bandwidth_compliance;
    
    // Auto-correction summary
    U32 corrections_applied;
    char correction_summary[512];
} openavb_stream_validation_result_t;

/// Profile compliance monitoring status
typedef enum {
    OPENAVB_PROFILE_COMPLIANCE_UNKNOWN,     ///< Compliance status not yet determined
    OPENAVB_PROFILE_COMPLIANCE_COMPLIANT,   ///< Stream is fully compliant with profile
    OPENAVB_PROFILE_COMPLIANCE_WARNING,     ///< Stream has minor compliance issues
    OPENAVB_PROFILE_COMPLIANCE_NON_COMPLIANT, ///< Stream violates profile requirements
    OPENAVB_PROFILE_COMPLIANCE_MONITORING   ///< Continuous monitoring active
} openavb_profile_compliance_status_t;

/// Profile-specific latency constraints
typedef struct {
    U32 max_presentation_time_usec;    ///< Maximum presentation time offset
    U32 max_transit_time_usec;         ///< Maximum network transit time
    U32 max_processing_time_usec;      ///< Maximum processing latency
    U32 buffer_target_usec;            ///< Target buffer size
    bool strict_timing_required;       ///< Whether timing must be exact
} openavb_latency_constraints_t;

/// Profile-specific format requirements
typedef struct {
    // Audio format constraints
    U32 supported_sample_rates[8];     ///< Allowed sample rates
    U32 supported_sample_rate_count;   ///< Number of supported rates
    U8  supported_bit_depths[4];       ///< Allowed bit depths
    U32 supported_bit_depth_count;     ///< Number of supported bit depths
    U8  max_channels;                  ///< Maximum number of channels
    bool requires_timestamping;        ///< Whether precise timestamping is required
    
    // Video format constraints  
    U32 max_frame_rate;                ///< Maximum video frame rate
    U32 max_resolution_width;          ///< Maximum video width
    U32 max_resolution_height;         ///< Maximum video height
    U32 max_bitrate_mbps;              ///< Maximum video bitrate (Mbps)
    
    // General constraints
    bool requires_class_a;             ///< Whether SR Class A is required
    bool requires_vlan;                ///< Whether VLAN tagging is required
    U8  required_priority;             ///< Required traffic priority
} openavb_format_requirements_t;

/// Profile validation criteria configuration
typedef struct {
    bool validate_latency;             ///< Enable latency validation
    bool validate_formats;             ///< Enable format validation  
    bool validate_bandwidth;           ///< Enable bandwidth validation
    bool validate_timing;              ///< Enable timing validation
    bool validate_interoperability;    ///< Enable interop validation
    bool strict_mode;                  ///< Whether to enforce strict compliance
    bool auto_correct_enabled;         ///< Allow automatic corrections
    
    // Custom validation thresholds
    U32 latency_tolerance_percent;     ///< Allowed latency variance (%)
    U32 jitter_tolerance_usec;         ///< Maximum allowed jitter
    U32 packet_loss_tolerance_ppm;     ///< Maximum packet loss (parts per million)
} openavb_validation_criteria_t;

/// Extended TL configuration with profile support
typedef struct {
    // Base OpenAvnu TL configuration
    openavb_tl_base_cfg_t base_cfg;
    
    // Profile system integration
    openavb_tl_profile_type_t profile_type;           ///< Active profile type
    void *profile_cfg;                                 ///< Profile configuration pointer
    bool auto_profile_detection;                       ///< Enable automatic profile detection
    bool profile_validation_enabled;                   ///< Enable profile validation
    bool profile_override_allowed;                     ///< Allow manual parameter overrides
    
    // Profile-specific constraints
    openavb_latency_constraints_t latency_constraints;     ///< Latency requirements
    openavb_format_requirements_t format_requirements;     ///< Format requirements  
    openavb_validation_criteria_t validation_criteria;     ///< Validation configuration
    
    // Runtime profile management
    char profile_config_file[OPENAVB_PROFILE_CONFIG_PATH_MAX]; ///< Profile config file
    bool dynamic_profile_switching;                ///< Allow runtime profile changes
    bool continuous_monitoring;                    ///< Enable continuous compliance monitoring
    U32  monitoring_interval_msec;                 ///< Monitoring check interval
    
    // Profile state tracking
    openavb_stream_validation_result_t last_validation_result;  ///< Last validation results
    openavb_profile_compliance_status_t compliance_status;      ///< Current compliance status
    U64 profile_validation_timestamp;              ///< Last validation timestamp
    U32 compliance_check_count;                    ///< Number of compliance checks performed
    
    // Error handling and recovery
    bool profile_error_recovery_enabled;           ///< Enable automatic error recovery
    U32  max_compliance_violations;                ///< Max violations before stream stop
    U32  current_violation_count;                  ///< Current violation count
    
} openavb_tl_profile_cfg_t;

/// Profile context for runtime stream management
typedef struct {
    openavb_tl_profile_type_t profile_type;           ///< Stream profile type
    openavb_stream_validation_result_t validation_result; ///< Validation results
    openavb_profile_compliance_status_t compliance_status; ///< Compliance status
    
    // Profile-specific runtime data
    void *profile_private_data;                    ///< Profile-specific data
    U32   profile_data_size;                       ///< Size of profile data
    
    // Performance metrics
    U32 average_latency_usec;                      ///< Measured average latency
    U32 max_latency_usec;                          ///< Measured maximum latency
    U32 jitter_usec;                               ///< Measured jitter
    U32 packet_loss_count;                         ///< Packet loss counter
    
    // Compliance monitoring
    U64 last_compliance_check;                     ///< Last compliance check timestamp
    U32 compliance_check_interval;                 ///< Check interval in milliseconds
    bool compliance_monitoring_active;             ///< Whether monitoring is active
    
} openavb_tl_profile_context_t;

// =============================================================================
// Profile-Aware TL Configuration Functions
// =============================================================================

/** Initialize profile-aware TL configuration with defaults.
 * 
 * Initializes the profile configuration structure with appropriate defaults
 * based on the specified profile type.
 * 
 * \param pProfileCfg Pointer to profile configuration structure
 * \param profile_type Profile type to configure for
 * \return TRUE on success or FALSE on failure
 */
bool openavbTLInitProfileCfg(openavb_tl_profile_cfg_t *pProfileCfg, 
                             openavb_tl_profile_type_t profile_type);

/** Load profile configuration from file.
 * 
 * Loads profile-specific configuration from an INI file, applying profile
 * defaults and validating parameter combinations.
 * 
 * \param pProfileCfg Pointer to profile configuration structure
 * \param config_file_path Path to profile configuration file
 * \return TRUE on success or FALSE on failure
 */
bool openavbTLLoadProfileConfig(openavb_tl_profile_cfg_t *pProfileCfg,
                               const char *config_file_path);

/** Validate stream configuration against profile requirements.
 * 
 * Performs comprehensive validation of stream configuration against the 
 * specified profile requirements, generating detailed compliance reports.
 * 
 * \param pProfileCfg Pointer to profile configuration structure
 * \param pValidationResult Pointer to validation result structure
 * \return TRUE if configuration is valid, FALSE otherwise
 */
bool openavbTLValidateProfileConfiguration(const openavb_tl_profile_cfg_t *pProfileCfg,
                                          openavb_stream_validation_result_t *pValidationResult);

/** Apply automatic profile-based parameter corrections.
 * 
 * Automatically corrects configuration parameters to ensure profile compliance
 * where possible, updating the configuration structure.
 * 
 * \param pProfileCfg Pointer to profile configuration structure  
 * \return Number of corrections applied
 */
U32 openavbTLApplyProfileCorrections(openavb_tl_profile_cfg_t *pProfileCfg);

/** Get profile-specific constraints for stream type.
 * 
 * Retrieves the latency, format, and validation constraints for the specified
 * profile type and stream configuration.
 * 
 * \param profile_type Profile type to get constraints for
 * \param pLatencyConstraints Pointer to latency constraints structure
 * \param pFormatRequirements Pointer to format requirements structure
 * \param pValidationCriteria Pointer to validation criteria structure
 * \return TRUE on success or FALSE on failure
 */
bool openavbTLGetProfileConstraints(openavb_tl_profile_type_t profile_type,
                                   openavb_latency_constraints_t *pLatencyConstraints,
                                   openavb_format_requirements_t *pFormatRequirements, 
                                   openavb_validation_criteria_t *pValidationCriteria);

/** Convert validation result to human-readable report.
 * 
 * Generates a detailed human-readable report of validation results including
 * compliance status, issues found, and recommended corrections.
 * 
 * \param pValidationResult Pointer to validation result structure
 * \param report_buffer Buffer to store the report
 * \param buffer_size Size of the report buffer
 * \return Number of characters written to buffer
 */
U32 openavbTLGenerateValidationReport(const openavb_stream_validation_result_t *pValidationResult,
                                     char *report_buffer, 
                                     U32 buffer_size);

// =============================================================================
// Profile Context Management Functions
// =============================================================================

/** Create profile context for stream runtime management.
 * 
 * Creates and initializes a profile context for runtime stream monitoring
 * and compliance tracking.
 * 
 * \param profile_type Profile type for the context
 * \param pProfileCfg Pointer to profile configuration
 * \return Pointer to created profile context, NULL on failure
 */
openavb_tl_profile_context_t* openavbTLCreateProfileContext(openavb_tl_profile_type_t profile_type,
                                                           const openavb_tl_profile_cfg_t *pProfileCfg);

/** Destroy profile context and clean up resources.
 * 
 * Destroys the profile context and frees all associated resources.
 * 
 * \param pContext Pointer to profile context to destroy
 */
void openavbTLDestroyProfileContext(openavb_tl_profile_context_t *pContext);

/** Update profile context with runtime metrics.
 * 
 * Updates the profile context with current stream performance metrics
 * for compliance monitoring.
 * 
 * \param pContext Pointer to profile context
 * \param latency_usec Current stream latency in microseconds
 * \param jitter_usec Current stream jitter in microseconds  
 * \param packet_loss_count Current packet loss count
 * \return TRUE on success or FALSE on failure
 */
bool openavbTLUpdateProfileMetrics(openavb_tl_profile_context_t *pContext,
                                  U32 latency_usec,
                                  U32 jitter_usec, 
                                  U32 packet_loss_count);

/** Check profile compliance with current metrics.
 * 
 * Evaluates current stream performance against profile requirements and
 * updates compliance status.
 * 
 * \param pContext Pointer to profile context
 * \return Current compliance status
 */
openavb_profile_compliance_status_t openavbTLCheckProfileCompliance(openavb_tl_profile_context_t *pContext);

#endif // OPENAVB_TL_PROFILE_CFG_H
