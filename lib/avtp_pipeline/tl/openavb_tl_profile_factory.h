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
* HEADER SUMMARY : Profile-Aware Stream Factory for OpenAvnu AVTP Pipeline
* 
* Provides factory functions for creating and configuring AVTP streams with
* automatic profile compliance validation and optimization. Integrates the
* AVTP Profile System with the existing OpenAvnu TL (Talker/Listener) framework.
*/

#ifndef OPENAVB_TL_PROFILE_FACTORY_H
#define OPENAVB_TL_PROFILE_FACTORY_H 1

#include "../include/openavb_tl_profile_cfg.h"

/** \file
 * Profile-Aware Stream Factory for OpenAvnu AVTP Pipeline.
 * 
 * This module provides enhanced stream creation and configuration functions
 * that automatically apply profile-specific optimizations and validate
 * compliance with industry-standard AVTP profiles.
 */

// =============================================================================
// Profile Factory Configuration
// =============================================================================

/// Profile factory configuration options
typedef struct {
    bool enable_auto_correction;      ///< Enable automatic parameter correction
    bool strict_validation;           ///< Enforce strict profile compliance
    bool allow_profile_mixing;        ///< Allow multiple profiles in same application
    U32  validation_timeout_msec;     ///< Timeout for validation operations
    char default_profile_config[256]; ///< Default profile configuration file
} openavb_profile_factory_cfg_t;

/// Profile stream creation options
typedef struct {
    openavb_tl_profile_type_t profile_type;    ///< Requested profile type
    bool auto_detect_profile;                  ///< Auto-detect from configuration
    bool enable_monitoring;                    ///< Enable runtime monitoring
    bool allow_fallback;                       ///< Allow fallback to basic stream
    char custom_profile_config[256];           ///< Custom profile configuration
} openavb_profile_stream_options_t;

// =============================================================================
// Core Profile Factory Functions
// =============================================================================

/** Initialize the profile factory system.
 * 
 * Initializes the profile-aware stream factory system with the specified
 * configuration. Must be called before any other profile factory functions.
 * 
 * \param pFactoryCfg Pointer to factory configuration structure
 * \return TRUE on success or FALSE on failure
 */
bool openavbProfileFactoryInitialize(const openavb_profile_factory_cfg_t *pFactoryCfg);

/** Shutdown the profile factory system.
 * 
 * Shuts down the profile factory system and cleans up all resources.
 * Should be called when the application is terminating.
 */
void openavbProfileFactoryShutdown(void);

/** Open a talker or listener with profile support.
 * 
 * Creates a new talker or listener with automatic profile configuration
 * and validation. This is the profile-aware version of openavbTLOpen().
 * 
 * \param profile_type Profile type to configure for
 * \return handle of the talker/listener. NULL if creation failed
 */
tl_handle_t openavbTLOpenWithProfile(openavb_tl_profile_type_t profile_type);

/** Open a stream with advanced profile options.
 * 
 * Creates a new stream with detailed profile configuration options including
 * auto-detection, monitoring, and fallback capabilities.
 * 
 * \param pStreamOptions Pointer to stream creation options
 * \return handle of the talker/listener. NULL if creation failed
 */
tl_handle_t openavbTLOpenWithProfileOptions(const openavb_profile_stream_options_t *pStreamOptions);

/** Configure talker/listener with profile validation.
 * 
 * Configures a talker or listener with automatic profile compliance validation
 * and optimization. This is the profile-aware version of openavbTLConfigure().
 * 
 * \param handle Handle of talker/listener
 * \param pProfileCfg Pointer to profile configuration structure
 * \param pNVCfg Pointer to name-value pair configuration structure
 * \return TRUE on success or FALSE on failure
 */
bool openavbTLConfigureWithProfile(tl_handle_t handle, 
                                   openavb_tl_profile_cfg_t *pProfileCfg,
                                   void *pNVCfg);

/** Validate stream configuration against profile requirements.
 * 
 * Performs comprehensive validation of the stream configuration against
 * the specified profile requirements. Can be called before or after
 * stream configuration.
 * 
 * \param handle Handle of talker/listener
 * \return TRUE if stream complies with profile, FALSE otherwise
 */
bool openavbTLValidateProfileCompliance(tl_handle_t handle);

/** Run stream with profile monitoring.
 * 
 * Starts the stream with continuous profile compliance monitoring.
 * This is the profile-aware version of openavbTLRun().
 * 
 * \param handle Handle of talker/listener
 * \return TRUE on success or FALSE on failure
 */
bool openavbTLRunWithProfileMonitoring(tl_handle_t handle);

// =============================================================================
// Profile Detection and Auto-Configuration
// =============================================================================

/** Detect profile type from configuration file.
 * 
 * Automatically detects the most appropriate profile type based on
 * the configuration parameters in an INI file.
 * 
 * \param config_file_path Path to configuration file
 * \return Detected profile type, or OPENAVB_PROFILE_CUSTOM if detection fails
 */
openavb_tl_profile_type_t openavbTLDetectProfileFromConfig(const char *config_file_path);

/** Auto-configure stream for detected profile.
 * 
 * Automatically configures stream parameters based on the detected
 * or specified profile type, applying optimal settings.
 * 
 * \param handle Handle of talker/listener
 * \param profile_type Profile type to configure for
 * \return TRUE on success or FALSE on failure
 */
bool openavbTLAutoConfigureProfile(tl_handle_t handle, openavb_tl_profile_type_t profile_type);

/** Get recommended configuration for profile.
 * 
 * Returns recommended configuration parameters for the specified profile
 * type, which can be used as a starting point for customization.
 * 
 * \param profile_type Profile type to get recommendations for
 * \param pProfileCfg Pointer to configuration structure to fill
 * \return TRUE on success or FALSE on failure
 */
bool openavbTLGetRecommendedProfileConfig(openavb_tl_profile_type_t profile_type,
                                         openavb_tl_profile_cfg_t *pProfileCfg);

// =============================================================================
// Profile Compliance and Monitoring
// =============================================================================

/** Get current profile compliance status.
 * 
 * Retrieves the current profile compliance status for a running stream,
 * including any violations or warnings.
 * 
 * \param handle Handle of talker/listener
 * \param pComplianceStatus Pointer to compliance status structure
 * \return TRUE on success or FALSE on failure
 */
bool openavbTLGetProfileComplianceStatus(tl_handle_t handle, 
                                        openavb_profile_compliance_status_t *pComplianceStatus);

/** Get detailed validation report for stream.
 * 
 * Generates a detailed validation report showing all compliance checks,
 * issues, and recommendations for the stream.
 * 
 * \param handle Handle of talker/listener
 * \param report_buffer Buffer to store the report
 * \param buffer_size Size of the report buffer
 * \return Number of characters written to buffer
 */
U32 openavbTLGetDetailedValidationReport(tl_handle_t handle,
                                        char *report_buffer,
                                        U32 buffer_size);

/** Update stream profile at runtime.
 * 
 * Updates the active profile for a running stream, applying new constraints
 * and validation criteria. Stream may be briefly paused during the update.
 * 
 * \param handle Handle of talker/listener
 * \param new_profile_type New profile type to apply
 * \return TRUE on success or FALSE on failure
 */
bool openavbTLUpdateStreamProfile(tl_handle_t handle, 
                                 openavb_tl_profile_type_t new_profile_type);

// =============================================================================
// Profile-Specific Stream Creation Helpers
// =============================================================================

/** Create Milan-compliant audio stream.
 * 
 * Creates a stream specifically configured for Milan compliance with
 * 2ms latency, appropriate sample rates, and Class A priority.
 * 
 * \param is_talker TRUE for talker, FALSE for listener
 * \param sample_rate Audio sample rate (48000 or 96000)
 * \param channels Number of audio channels (1-8)
 * \param bit_depth Audio bit depth (16, 24, or 32)
 * \return handle of the stream. NULL if creation failed
 */
tl_handle_t openavbTLCreateMilanAudioStream(bool is_talker,
                                           U32 sample_rate,
                                           U8 channels,
                                           U8 bit_depth);

/** Create Automotive video stream.
 * 
 * Creates a stream specifically configured for Automotive compliance with
 * 50ms latency, H.264 compression, and appropriate bandwidth limits.
 * 
 * \param is_talker TRUE for talker, FALSE for listener
 * \param frame_rate Video frame rate (15, 30, or 60 fps)
 * \param width Video width in pixels
 * \param height Video height in pixels
 * \param bitrate_mbps Target bitrate in Mbps
 * \return handle of the stream. NULL if creation failed
 */
tl_handle_t openavbTLCreateAutomotiveVideoStream(bool is_talker,
                                                U32 frame_rate,
                                                U32 width,
                                                U32 height,
                                                U32 bitrate_mbps);

/** Create ProAV high-resolution audio stream.
 * 
 * Creates a stream specifically configured for ProAV applications with
 * 5ms latency, high sample rates, and professional audio requirements.
 * 
 * \param is_talker TRUE for talker, FALSE for listener
 * \param sample_rate Audio sample rate (up to 192000)
 * \param channels Number of audio channels (1-32)
 * \param bit_depth Audio bit depth (24 or 32)
 * \return handle of the stream. NULL if creation failed
 */
tl_handle_t openavbTLCreateProAVAudioStream(bool is_talker,
                                           U32 sample_rate,
                                           U8 channels,
                                           U8 bit_depth);

// =============================================================================
// Profile Factory Utilities
// =============================================================================

/** Convert profile type to string representation.
 * 
 * Returns a human-readable string representation of the profile type.
 * 
 * \param profile_type Profile type to convert
 * \return Pointer to string representation
 */
const char* openavbTLProfileTypeToString(openavb_tl_profile_type_t profile_type);

/** Parse profile type from string.
 * 
 * Parses a profile type from its string representation.
 * 
 * \param profile_string String representation of profile type
 * \return Parsed profile type, or OPENAVB_PROFILE_CUSTOM if parsing fails
 */
openavb_tl_profile_type_t openavbTLProfileTypeFromString(const char *profile_string);

/** Get profile factory statistics.
 * 
 * Retrieves statistics about profile factory usage including number of
 * streams created, validation success rate, and performance metrics.
 * 
 * \param pStats Pointer to statistics structure to fill
 * \return TRUE on success or FALSE on failure
 */
typedef struct {
    U32 streams_created_total;
    U32 streams_created_milan;
    U32 streams_created_automotive;
    U32 streams_created_proav;
    U32 validation_success_count;
    U32 validation_failure_count;
    U32 auto_corrections_applied;
    U32 compliance_violations_detected;
} openavb_profile_factory_stats_t;

bool openavbTLGetProfileFactoryStats(openavb_profile_factory_stats_t *pStats);

#endif // OPENAVB_TL_PROFILE_FACTORY_H
