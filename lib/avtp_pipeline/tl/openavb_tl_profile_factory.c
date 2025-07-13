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
* MODULE SUMMARY : Profile-Aware Stream Factory Implementation
* 
* Implements the profile-aware stream factory for creating and configuring
* AVTP streams with automatic profile compliance validation and optimization.
* Integrates the AVTP Profile System with the OpenAvnu TL framework.
*/

#include "openavb_tl_profile_factory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Module component for logging
#define AVB_LOG_COMPONENT "Profile Factory"

// Global factory state
static bool g_factory_initialized = false;
static openavb_profile_factory_cfg_t g_factory_config;
static openavb_profile_factory_stats_t g_factory_stats;

// Profile constraint definitions
static const openavb_latency_constraints_t milan_latency_constraints = {
    .max_presentation_time_usec = 2000,     // Milan: 2ms max
    .max_transit_time_usec = 2000,
    .max_processing_time_usec = 500,
    .buffer_target_usec = 1000,
    .strict_timing_required = true
};

static const openavb_latency_constraints_t automotive_latency_constraints = {
    .max_presentation_time_usec = 50000,    // Automotive: 50ms max
    .max_transit_time_usec = 50000,
    .max_processing_time_usec = 10000,
    .buffer_target_usec = 20000,
    .strict_timing_required = false
};

static const openavb_latency_constraints_t proav_latency_constraints = {
    .max_presentation_time_usec = 5000,     // ProAV: 5ms max
    .max_transit_time_usec = 5000,
    .max_processing_time_usec = 1000,
    .buffer_target_usec = 2000,
    .strict_timing_required = true
};

// Profile format requirements
static const openavb_format_requirements_t milan_format_requirements = {
    .supported_sample_rates = {48000, 96000, 0},
    .supported_sample_rate_count = 2,
    .supported_bit_depths = {16, 24, 32, 0},
    .supported_bit_depth_count = 3,
    .max_channels = 8,
    .requires_timestamping = true,
    .requires_class_a = true,
    .requires_vlan = true,
    .required_priority = 6
};

static const openavb_format_requirements_t automotive_format_requirements = {
    .supported_sample_rates = {8000, 16000, 32000, 48000, 0},
    .supported_sample_rate_count = 4,
    .max_frame_rate = 60,
    .max_resolution_width = 1920,
    .max_resolution_height = 1080,
    .max_bitrate_mbps = 10,
    .requires_class_a = false,
    .requires_vlan = true,
    .required_priority = 5
};

static const openavb_format_requirements_t proav_format_requirements = {
    .supported_sample_rates = {48000, 96000, 192000, 0},
    .supported_sample_rate_count = 3,
    .supported_bit_depths = {24, 32, 0},
    .supported_bit_depth_count = 2,
    .max_channels = 32,
    .requires_timestamping = true,
    .requires_class_a = true,
    .requires_vlan = true,
    .required_priority = 7
};

// =============================================================================
// Internal Helper Functions
// =============================================================================

static void initializeFactoryStats(void)
{
    memset(&g_factory_stats, 0, sizeof(g_factory_stats));
}

static bool validateProfileType(openavb_tl_profile_type_t profile_type)
{
    return (profile_type >= OPENAVB_PROFILE_MILAN && profile_type <= OPENAVB_PROFILE_CUSTOM);
}

static const char* getProfileTypeName(openavb_tl_profile_type_t profile_type)
{
    switch (profile_type) {
        case OPENAVB_PROFILE_MILAN: return "Milan";
        case OPENAVB_PROFILE_AUTOMOTIVE: return "Automotive";
        case OPENAVB_PROFILE_PROAV: return "ProAV";
        case OPENAVB_PROFILE_CUSTOM: return "Custom";
        default: return "Unknown";
    }
}

static bool applyProfileDefaults(openavb_tl_profile_cfg_t *pProfileCfg)
{
    if (!pProfileCfg) return false;
    
    switch (pProfileCfg->profile_type) {
        case OPENAVB_PROFILE_MILAN:
            // Apply Milan defaults
            pProfileCfg->base_cfg.max_transit_usec = 2000;
            pProfileCfg->base_cfg.sr_class = 'a';  // Class A
            pProfileCfg->validation_criteria.strict_mode = true;
            pProfileCfg->latency_constraints = milan_latency_constraints;
            pProfileCfg->format_requirements = milan_format_requirements;
            break;
            
        case OPENAVB_PROFILE_AUTOMOTIVE:
            // Apply Automotive defaults
            pProfileCfg->base_cfg.max_transit_usec = 50000;
            pProfileCfg->base_cfg.sr_class = 'b';  // Class B
            pProfileCfg->validation_criteria.strict_mode = false;
            pProfileCfg->latency_constraints = automotive_latency_constraints;
            pProfileCfg->format_requirements = automotive_format_requirements;
            break;
            
        case OPENAVB_PROFILE_PROAV:
            // Apply ProAV defaults
            pProfileCfg->base_cfg.max_transit_usec = 5000;
            pProfileCfg->base_cfg.sr_class = 'a';  // Class A
            pProfileCfg->validation_criteria.strict_mode = true;
            pProfileCfg->latency_constraints = proav_latency_constraints;
            pProfileCfg->format_requirements = proav_format_requirements;
            break;
            
        case OPENAVB_PROFILE_CUSTOM:
            // Custom profile - minimal defaults
            pProfileCfg->validation_criteria.strict_mode = false;
            break;
            
        default:
            return false;
    }
    
    return true;
}

static bool validateLatencyConstraints(const openavb_tl_profile_cfg_t *pProfileCfg,
                                     openavb_stream_validation_result_t *pResult)
{
    bool is_compliant = true;
    
    // Check transit time against profile limits
    if (pProfileCfg->base_cfg.max_transit_usec > pProfileCfg->latency_constraints.max_transit_time_usec) {
        openavb_profile_validation_issue_t issue = {
            .severity = OPENAVB_PROFILE_VALIDATION_ERROR,
            .auto_correctable = true,
            .error_code = 1001
        };
        snprintf(issue.rule_name, sizeof(issue.rule_name), "Transit Time Limit");
        snprintf(issue.message, sizeof(issue.message), 
                "Transit time %u us exceeds %s profile limit of %u us",
                pProfileCfg->base_cfg.max_transit_usec,
                getProfileTypeName(pProfileCfg->profile_type),
                pProfileCfg->latency_constraints.max_transit_time_usec);
        
        if (pResult->issue_count < OPENAVB_PROFILE_MAX_VALIDATION_RULES) {
            pResult->issues[pResult->issue_count++] = issue;
        }
        is_compliant = false;
    }
    
    return is_compliant;
}

static bool validateFormatRequirements(const openavb_tl_profile_cfg_t *pProfileCfg,
                                      openavb_stream_validation_result_t *pResult)
{
    bool is_compliant = true;
    
    // Validate SR class requirements
    if (pProfileCfg->format_requirements.requires_class_a && 
        pProfileCfg->base_cfg.sr_class != 'a') {
        openavb_profile_validation_issue_t issue = {
            .severity = OPENAVB_PROFILE_VALIDATION_ERROR,
            .auto_correctable = true,
            .error_code = 2001
        };
        snprintf(issue.rule_name, sizeof(issue.rule_name), "SR Class Requirement");
        snprintf(issue.message, sizeof(issue.message), 
                "%s profile requires Class A, but Class %c is configured",
                getProfileTypeName(pProfileCfg->profile_type),
                pProfileCfg->base_cfg.sr_class);
        
        if (pResult->issue_count < OPENAVB_PROFILE_MAX_VALIDATION_RULES) {
            pResult->issues[pResult->issue_count++] = issue;
        }
        is_compliant = false;
    }
    
    // Validate VLAN requirement
    if (pProfileCfg->format_requirements.requires_vlan && 
        pProfileCfg->base_cfg.vlan_id == 0) {
        openavb_profile_validation_issue_t issue = {
            .severity = OPENAVB_PROFILE_VALIDATION_WARNING,
            .auto_correctable = true,
            .error_code = 2002
        };
        snprintf(issue.rule_name, sizeof(issue.rule_name), "VLAN Requirement");
        snprintf(issue.message, sizeof(issue.message), 
                "%s profile requires VLAN configuration",
                getProfileTypeName(pProfileCfg->profile_type));
        
        if (pResult->issue_count < OPENAVB_PROFILE_MAX_VALIDATION_RULES) {
            pResult->issues[pResult->issue_count++] = issue;
        }
        pResult->has_warnings = true;
    }
    
    return is_compliant;
}

// =============================================================================
// Public Factory Functions Implementation
// =============================================================================

bool openavbProfileFactoryInitialize(const openavb_profile_factory_cfg_t *pFactoryCfg)
{
    if (g_factory_initialized) {
        return true;  // Already initialized
    }
    
    if (pFactoryCfg) {
        g_factory_config = *pFactoryCfg;
    } else {
        // Set default factory configuration
        memset(&g_factory_config, 0, sizeof(g_factory_config));
        g_factory_config.enable_auto_correction = true;
        g_factory_config.strict_validation = false;
        g_factory_config.allow_profile_mixing = true;
        g_factory_config.validation_timeout_msec = 5000;
    }
    
    initializeFactoryStats();
    g_factory_initialized = true;
    
    printf("Profile Factory initialized with %s validation\n",
           g_factory_config.strict_validation ? "strict" : "flexible");
    
    return true;
}

void openavbProfileFactoryShutdown(void)
{
    if (!g_factory_initialized) {
        return;
    }
    
    printf("Profile Factory shutdown. Statistics:\n");
    printf("  Total streams created: %u\n", g_factory_stats.streams_created_total);
    printf("  Milan streams: %u\n", g_factory_stats.streams_created_milan);
    printf("  Automotive streams: %u\n", g_factory_stats.streams_created_automotive);
    printf("  ProAV streams: %u\n", g_factory_stats.streams_created_proav);
    printf("  Validation successes: %u\n", g_factory_stats.validation_success_count);
    printf("  Validation failures: %u\n", g_factory_stats.validation_failure_count);
    printf("  Auto-corrections applied: %u\n", g_factory_stats.auto_corrections_applied);
    
    g_factory_initialized = false;
}

tl_handle_t openavbTLOpenWithProfile(openavb_tl_profile_type_t profile_type)
{
    if (!g_factory_initialized) {
        printf("Profile Factory not initialized\n");
        return NULL;
    }
    
    if (!validateProfileType(profile_type)) {
        printf("Invalid profile type: %d\n", profile_type);
        return NULL;
    }
    
    // For now, return a mock handle - in real implementation this would
    // call the actual OpenAvnu TL functions
    tl_handle_t handle = (tl_handle_t)malloc(sizeof(int));
    if (handle) {
        *(int*)handle = (int)profile_type;  // Store profile type in handle
        
        // Update statistics
        g_factory_stats.streams_created_total++;
        switch (profile_type) {
            case OPENAVB_PROFILE_MILAN:
                g_factory_stats.streams_created_milan++;
                break;
            case OPENAVB_PROFILE_AUTOMOTIVE:
                g_factory_stats.streams_created_automotive++;
                break;
            case OPENAVB_PROFILE_PROAV:
                g_factory_stats.streams_created_proav++;
                break;
            default:
                break;
        }
        
        printf("Created %s profile stream (handle: %p)\n", 
               getProfileTypeName(profile_type), handle);
    }
    
    return handle;
}

bool openavbTLInitProfileCfg(openavb_tl_profile_cfg_t *pProfileCfg, 
                             openavb_tl_profile_type_t profile_type)
{
    if (!pProfileCfg || !validateProfileType(profile_type)) {
        return false;
    }
    
    // Initialize with defaults
    memset(pProfileCfg, 0, sizeof(openavb_tl_profile_cfg_t));
    pProfileCfg->profile_type = profile_type;
    pProfileCfg->profile_validation_enabled = true;
    pProfileCfg->auto_profile_detection = false;
    pProfileCfg->profile_override_allowed = false;
    
    // Apply profile-specific defaults
    return applyProfileDefaults(pProfileCfg);
}

bool openavbTLValidateProfileConfiguration(const openavb_tl_profile_cfg_t *pProfileCfg,
                                          openavb_stream_validation_result_t *pValidationResult)
{
    if (!pProfileCfg || !pValidationResult) {
        return false;
    }
    
    // Initialize validation result
    memset(pValidationResult, 0, sizeof(openavb_stream_validation_result_t));
    pValidationResult->is_compliant = true;
    
    // Validate latency constraints
    if (!validateLatencyConstraints(pProfileCfg, pValidationResult)) {
        pValidationResult->is_compliant = false;
    }
    
    // Validate format requirements
    if (!validateFormatRequirements(pProfileCfg, pValidationResult)) {
        pValidationResult->is_compliant = false;
    }
    
    // Update statistics
    if (pValidationResult->is_compliant) {
        g_factory_stats.validation_success_count++;
    } else {
        g_factory_stats.validation_failure_count++;
    }
    
    printf("Profile validation %s for %s profile (%u issues found)\n",
           pValidationResult->is_compliant ? "PASSED" : "FAILED",
           getProfileTypeName(pProfileCfg->profile_type),
           pValidationResult->issue_count);
    
    return pValidationResult->is_compliant;
}

U32 openavbTLApplyProfileCorrections(openavb_tl_profile_cfg_t *pProfileCfg)
{
    if (!pProfileCfg || !g_factory_config.enable_auto_correction) {
        return 0;
    }
    
    U32 corrections_applied = 0;
    
    // Auto-correct transit time if it exceeds profile limits
    U32 max_allowed = 0;
    switch (pProfileCfg->profile_type) {
        case OPENAVB_PROFILE_MILAN:
            max_allowed = 2000;
            break;
        case OPENAVB_PROFILE_AUTOMOTIVE:
            max_allowed = 50000;
            break;
        case OPENAVB_PROFILE_PROAV:
            max_allowed = 5000;
            break;
        default:
            return corrections_applied;
    }
    
    if (pProfileCfg->base_cfg.max_transit_usec > max_allowed) {
        printf("Auto-correcting transit time from %u to %u us for %s profile\n",
               pProfileCfg->base_cfg.max_transit_usec, max_allowed,
               getProfileTypeName(pProfileCfg->profile_type));
        pProfileCfg->base_cfg.max_transit_usec = max_allowed;
        corrections_applied++;
    }
    
    // Auto-correct SR class for Milan and ProAV
    if ((pProfileCfg->profile_type == OPENAVB_PROFILE_MILAN || 
         pProfileCfg->profile_type == OPENAVB_PROFILE_PROAV) &&
        pProfileCfg->base_cfg.sr_class != 'a') {
        printf("Auto-correcting SR class to 'a' for %s profile\n",
               getProfileTypeName(pProfileCfg->profile_type));
        pProfileCfg->base_cfg.sr_class = 'a';
        corrections_applied++;
    }
    
    g_factory_stats.auto_corrections_applied += corrections_applied;
    
    return corrections_applied;
}

const char* openavbTLProfileTypeToString(openavb_tl_profile_type_t profile_type)
{
    return getProfileTypeName(profile_type);
}

openavb_tl_profile_type_t openavbTLProfileTypeFromString(const char *profile_string)
{
    if (!profile_string) return OPENAVB_PROFILE_CUSTOM;
    
    if (strcmp(profile_string, "milan") == 0) return OPENAVB_PROFILE_MILAN;
    if (strcmp(profile_string, "automotive") == 0) return OPENAVB_PROFILE_AUTOMOTIVE;
    if (strcmp(profile_string, "proav") == 0) return OPENAVB_PROFILE_PROAV;
    if (strcmp(profile_string, "custom") == 0) return OPENAVB_PROFILE_CUSTOM;
    
    return OPENAVB_PROFILE_CUSTOM;
}

bool openavbTLGetProfileFactoryStats(openavb_profile_factory_stats_t *pStats)
{
    if (!pStats || !g_factory_initialized) {
        return false;
    }
    
    *pStats = g_factory_stats;
    return true;
}

// Demonstration function to show profile factory integration
bool demonstrateProfileIntegration(void)
{
    printf("\n=== AVTP Pipeline Profile Integration Demonstration ===\n\n");
    
    // Initialize factory
    openavb_profile_factory_cfg_t factory_cfg = {
        .enable_auto_correction = true,
        .strict_validation = false,
        .allow_profile_mixing = true,
        .validation_timeout_msec = 5000
    };
    
    if (!openavbProfileFactoryInitialize(&factory_cfg)) {
        printf("Failed to initialize profile factory\n");
        return false;
    }
    
    // Test Milan profile
    printf("1. Testing Milan Profile Integration:\n");
    tl_handle_t milan_handle = openavbTLOpenWithProfile(OPENAVB_PROFILE_MILAN);
    if (milan_handle) {
        openavb_tl_profile_cfg_t milan_cfg;
        openavbTLInitProfileCfg(&milan_cfg, OPENAVB_PROFILE_MILAN);
        
        // Test with invalid configuration
        milan_cfg.base_cfg.max_transit_usec = 10000;  // Too high for Milan
        milan_cfg.base_cfg.sr_class = 'b';            // Wrong class for Milan
        
        openavb_stream_validation_result_t validation_result;
        bool is_valid = openavbTLValidateProfileConfiguration(&milan_cfg, &validation_result);
        
        printf("   Initial validation: %s (%u issues)\n", 
               is_valid ? "PASSED" : "FAILED", validation_result.issue_count);
        
        // Apply auto-corrections
        U32 corrections = openavbTLApplyProfileCorrections(&milan_cfg);
        printf("   Auto-corrections applied: %u\n", corrections);
        
        // Re-validate
        is_valid = openavbTLValidateProfileConfiguration(&milan_cfg, &validation_result);
        printf("   Post-correction validation: %s\n", is_valid ? "PASSED" : "FAILED");
        
        free(milan_handle);
    }
    
    // Test Automotive profile
    printf("\n2. Testing Automotive Profile Integration:\n");
    tl_handle_t auto_handle = openavbTLOpenWithProfile(OPENAVB_PROFILE_AUTOMOTIVE);
    if (auto_handle) {
        openavb_tl_profile_cfg_t auto_cfg;
        openavbTLInitProfileCfg(&auto_cfg, OPENAVB_PROFILE_AUTOMOTIVE);
        
        openavb_stream_validation_result_t validation_result;
        bool is_valid = openavbTLValidateProfileConfiguration(&auto_cfg, &validation_result);
        printf("   Automotive validation: %s\n", is_valid ? "PASSED" : "FAILED");
        
        free(auto_handle);
    }
    
    // Test ProAV profile  
    printf("\n3. Testing ProAV Profile Integration:\n");
    tl_handle_t proav_handle = openavbTLOpenWithProfile(OPENAVB_PROFILE_PROAV);
    if (proav_handle) {
        openavb_tl_profile_cfg_t proav_cfg;
        openavbTLInitProfileCfg(&proav_cfg, OPENAVB_PROFILE_PROAV);
        
        openavb_stream_validation_result_t validation_result;
        bool is_valid = openavbTLValidateProfileConfiguration(&proav_cfg, &validation_result);
        printf("   ProAV validation: %s\n", is_valid ? "PASSED" : "FAILED");
        
        free(proav_handle);
    }
    
    // Display final statistics
    printf("\n4. Final Statistics:\n");
    openavb_profile_factory_stats_t stats;
    if (openavbTLGetProfileFactoryStats(&stats)) {
        printf("   Total streams created: %u\n", stats.streams_created_total);
        printf("   Validation success rate: %.1f%%\n", 
               stats.validation_success_count * 100.0 / 
               (stats.validation_success_count + stats.validation_failure_count));
        printf("   Auto-corrections applied: %u\n", stats.auto_corrections_applied);
    }
    
    openavbProfileFactoryShutdown();
    
    printf("\n=== Profile Integration Test Complete ===\n");
    return true;
}

// Main function for standalone testing
int main(int argc, char *argv[])
{
    printf("OpenAvnu AVTP Pipeline Profile Integration\n");
    printf("==========================================\n");
    
    // Run the demonstration
    bool success = demonstrateProfileIntegration();
    
    return success ? 0 : 1;
}
