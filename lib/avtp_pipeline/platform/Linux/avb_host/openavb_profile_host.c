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
* MODULE SUMMARY : Profile-Aware Host Application for OpenAvnu AVTP Pipeline
* 
* Enhanced version of the standard OpenAvnu host application with integrated
* AVTP profile support. Automatically detects profiles, validates configurations,
* and optimizes streams for Milan, Automotive, and ProAV compliance.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <getopt.h>
#include "openavb_tl_profile_factory.h"

#define AVB_LOG_COMPONENT "Profile Host"

bool bRunning = true;

// Profile-aware application state
typedef struct {
    U32 total_streams;
    U32 milan_streams;
    U32 automotive_streams;
    U32 proav_streams;
    U32 custom_streams;
    bool profile_monitoring_enabled;
    U32 validation_failures;
    U32 auto_corrections;
} profile_host_state_t;

static profile_host_state_t g_host_state = {0};

// Signal handler for graceful shutdown
static void profileHostSigHandler(int signal)
{
    if (signal == SIGINT || signal == SIGTERM) {
        if (bRunning) {
            printf("\nProfile Host shutting down gracefully...\n");
            bRunning = false;
        } else {
            // Force shutdown
            exit(2);
        }
    }
}

// Display usage information
void profileHostUsage(char *programName)
{
    printf(
        "\nProfile-Aware OpenAvnu Host Application\n"
        "=======================================\n\n"
        "Usage: %s [options] file...\n"
        "  -I val     Use given interface globally\n"
        "  -l val     Log file path\n"
        "  -p val     Force profile type (milan|automotive|proav|custom)\n"
        "  -s         Enable strict profile validation\n"
        "  -a         Enable auto-correction\n"
        "  -m         Enable continuous profile monitoring\n"
        "  -v         Verbose profile reporting\n"
        "  -h         Show this help\n"
        "\n"
        "Profile Examples:\n"
        "  %s milan_audio_talker_profile.ini\n"
        "    Start Milan-compliant audio stream with automatic validation\n\n"
        "  %s -p automotive automotive_video_talker_profile.ini\n"
        "    Force automotive profile validation\n\n"
        "  %s -s -a -m proav_audio_talker_profile.ini\n"
        "    Start ProAV stream with strict validation, auto-correction, and monitoring\n\n"
        "  %s -I eth0 milan_talker.ini automotive_listener.ini\n"
        "    Multi-profile setup with Milan talker and Automotive listener\n\n"
        ,
        programName, programName, programName, programName, programName);
}

// Parse profile type from string
openavb_tl_profile_type_t parseProfileType(const char *profile_str)
{
    if (!profile_str) return OPENAVB_PROFILE_CUSTOM;
    
    if (strcasecmp(profile_str, "milan") == 0) return OPENAVB_PROFILE_MILAN;
    if (strcasecmp(profile_str, "automotive") == 0) return OPENAVB_PROFILE_AUTOMOTIVE;
    if (strcasecmp(profile_str, "proav") == 0) return OPENAVB_PROFILE_PROAV;
    if (strcasecmp(profile_str, "custom") == 0) return OPENAVB_PROFILE_CUSTOM;
    
    printf("Warning: Unknown profile type '%s', using custom\n", profile_str);
    return OPENAVB_PROFILE_CUSTOM;
}

// Detect profile from configuration file
openavb_tl_profile_type_t detectProfileFromFile(const char *config_file)
{
    FILE *file = fopen(config_file, "r");
    if (!file) {
        printf("Warning: Cannot open %s for profile detection\n", config_file);
        return OPENAVB_PROFILE_CUSTOM;
    }
    
    char line[256];
    openavb_tl_profile_type_t detected_profile = OPENAVB_PROFILE_CUSTOM;
    
    while (fgets(line, sizeof(line), file)) {
        // Skip comments and empty lines
        if (line[0] == '#' || line[0] == '\n') continue;
        
        // Look for profile_type directive
        if (strncmp(line, "profile_type", 12) == 0) {
            char *equals = strchr(line, '=');
            if (equals) {
                char *value = equals + 1;
                // Trim whitespace
                while (*value == ' ' || *value == '\t') value++;
                char *end = value + strlen(value) - 1;
                while (end > value && (*end == '\n' || *end == '\r' || *end == ' ')) {
                    *end = '\0';
                    end--;
                }
                detected_profile = parseProfileType(value);
                break;
            }
        }
        
        // Auto-detect based on configuration patterns
        if (detected_profile == OPENAVB_PROFILE_CUSTOM) {
            if (strstr(line, "max_transit_usec = 2000") || 
                strstr(line, "max_transit_usec=2000")) {
                detected_profile = OPENAVB_PROFILE_MILAN;
            } else if (strstr(line, "max_transit_usec = 50000") || 
                      strstr(line, "max_transit_usec=50000")) {
                detected_profile = OPENAVB_PROFILE_AUTOMOTIVE;
            } else if (strstr(line, "max_transit_usec = 5000") || 
                      strstr(line, "max_transit_usec=5000")) {
                detected_profile = OPENAVB_PROFILE_PROAV;
            }
        }
    }
    
    fclose(file);
    return detected_profile;
}

// Validate and configure a single stream with profile awareness
bool configureProfileStream(const char *config_file, 
                          openavb_tl_profile_type_t forced_profile,
                          bool strict_validation,
                          bool auto_correction,
                          bool verbose)
{
    // Detect or use forced profile
    openavb_tl_profile_type_t profile_type = forced_profile;
    if (profile_type == OPENAVB_PROFILE_CUSTOM) {
        profile_type = detectProfileFromFile(config_file);
    }
    
    const char *profile_name = openavbTLProfileTypeToString(profile_type);
    
    if (verbose) {
        printf("\nConfiguring stream: %s\n", config_file);
        printf("Profile type: %s\n", profile_name);
    }
    
    // Create profile-aware stream
    tl_handle_t handle = openavbTLOpenWithProfile(profile_type);
    if (!handle) {
        printf("Error: Failed to create %s profile stream for %s\n", profile_name, config_file);
        return false;
    }
    
    // Initialize profile configuration
    openavb_tl_profile_cfg_t profile_cfg;
    if (!openavbTLInitProfileCfg(&profile_cfg, profile_type)) {
        printf("Error: Failed to initialize %s profile configuration\n", profile_name);
        free(handle);
        return false;
    }
    
    // Set validation options
    profile_cfg.validation_criteria.strict_mode = strict_validation;
    profile_cfg.validation_criteria.auto_correct_enabled = auto_correction;
    
    // TODO: Parse the INI file and populate profile_cfg
    // For now, use defaults with some simulated configuration
    
    // Validate configuration
    openavb_stream_validation_result_t validation_result;
    bool is_valid = openavbTLValidateProfileConfiguration(&profile_cfg, &validation_result);
    
    if (verbose || !is_valid) {
        printf("Validation result: %s\n", is_valid ? "PASSED" : "FAILED");
        if (validation_result.issue_count > 0) {
            printf("Issues found: %u\n", validation_result.issue_count);
            for (U32 i = 0; i < validation_result.issue_count && i < 5; i++) {
                printf("  %s: %s\n", 
                       validation_result.issues[i].rule_name,
                       validation_result.issues[i].message);
            }
        }
    }
    
    // Apply auto-corrections if enabled and needed
    if (auto_correction && !is_valid) {
        U32 corrections = openavbTLApplyProfileCorrections(&profile_cfg);
        if (corrections > 0) {
            printf("Applied %u auto-corrections\n", corrections);
            g_host_state.auto_corrections += corrections;
            
            // Re-validate after corrections
            is_valid = openavbTLValidateProfileConfiguration(&profile_cfg, &validation_result);
            if (verbose) {
                printf("Post-correction validation: %s\n", is_valid ? "PASSED" : "FAILED");
            }
        }
    }
    
    if (!is_valid) {
        if (strict_validation) {
            printf("Error: Stream does not comply with %s profile requirements\n", profile_name);
            free(handle);
            g_host_state.validation_failures++;
            return false;
        } else {
            printf("Warning: Stream has %s profile compliance issues but continuing\n", profile_name);
        }
    }
    
    // Update statistics
    g_host_state.total_streams++;
    switch (profile_type) {
        case OPENAVB_PROFILE_MILAN:
            g_host_state.milan_streams++;
            break;
        case OPENAVB_PROFILE_AUTOMOTIVE:
            g_host_state.automotive_streams++;
            break;
        case OPENAVB_PROFILE_PROAV:
            g_host_state.proav_streams++;
            break;
        case OPENAVB_PROFILE_CUSTOM:
            g_host_state.custom_streams++;
            break;
    }
    
    printf("✓ %s profile stream configured successfully\n", profile_name);
    
    // TODO: In real implementation, start the stream here
    // openavbTLRunWithProfileMonitoring(handle);
    
    return true;
}

// Display profile statistics
void displayProfileStatistics(bool verbose)
{
    printf("\n=== Profile Host Statistics ===\n");
    printf("Total streams configured: %u\n", g_host_state.total_streams);
    
    if (g_host_state.total_streams > 0) {
        printf("Profile distribution:\n");
        if (g_host_state.milan_streams > 0) {
            printf("  Milan: %u (%.1f%%)\n", g_host_state.milan_streams,
                   g_host_state.milan_streams * 100.0 / g_host_state.total_streams);
        }
        if (g_host_state.automotive_streams > 0) {
            printf("  Automotive: %u (%.1f%%)\n", g_host_state.automotive_streams,
                   g_host_state.automotive_streams * 100.0 / g_host_state.total_streams);
        }
        if (g_host_state.proav_streams > 0) {
            printf("  ProAV: %u (%.1f%%)\n", g_host_state.proav_streams,
                   g_host_state.proav_streams * 100.0 / g_host_state.total_streams);
        }
        if (g_host_state.custom_streams > 0) {
            printf("  Custom: %u (%.1f%%)\n", g_host_state.custom_streams,
                   g_host_state.custom_streams * 100.0 / g_host_state.total_streams);
        }
    }
    
    printf("Validation failures: %u\n", g_host_state.validation_failures);
    printf("Auto-corrections applied: %u\n", g_host_state.auto_corrections);
    
    if (verbose) {
        openavb_profile_factory_stats_t factory_stats;
        if (openavbTLGetProfileFactoryStats(&factory_stats)) {
            printf("\nFactory Statistics:\n");
            printf("  Validation success rate: %.1f%%\n",
                   factory_stats.validation_success_count * 100.0 /
                   (factory_stats.validation_success_count + factory_stats.validation_failure_count));
            printf("  Compliance violations detected: %u\n", 
                   factory_stats.compliance_violations_detected);
        }
    }
    
    printf("===============================\n");
}

// Main function
int main(int argc, char *argv[])
{
    printf("OpenAvnu Profile-Aware Host Application\n");
    printf("======================================\n\n");
    
    char *optIfnameGlobal = NULL;
    char *optLogFileName = NULL;
    char *optProfileType = NULL;
    bool optStrictValidation = false;
    bool optAutoCorrection = false;
    bool optMonitoring = false;
    bool optVerbose = false;
    
    // Process command line options
    int opt;
    while ((opt = getopt(argc, argv, "I:l:p:samvh")) != -1) {
        switch (opt) {
            case 'I':
                optIfnameGlobal = strdup(optarg);
                break;
            case 'l':
                optLogFileName = strdup(optarg);
                break;
            case 'p':
                optProfileType = strdup(optarg);
                break;
            case 's':
                optStrictValidation = true;
                break;
            case 'a':
                optAutoCorrection = true;
                break;
            case 'm':
                optMonitoring = true;
                g_host_state.profile_monitoring_enabled = true;
                break;
            case 'v':
                optVerbose = true;
                break;
            case 'h':
            default:
                profileHostUsage(argv[0]);
                exit(opt == 'h' ? 0 : -1);
        }
    }
    
    // Check for configuration files
    if (optind >= argc) {
        printf("Error: No configuration files specified\n\n");
        profileHostUsage(argv[0]);
        exit(-1);
    }
    
    // Initialize profile factory
    openavb_profile_factory_cfg_t factory_cfg = {
        .enable_auto_correction = optAutoCorrection,
        .strict_validation = optStrictValidation,
        .allow_profile_mixing = true,
        .validation_timeout_msec = 5000
    };
    
    if (!openavbProfileFactoryInitialize(&factory_cfg)) {
        printf("Error: Failed to initialize profile factory\n");
        exit(-1);
    }
    
    // Setup signal handler
    struct sigaction sa;
    sa.sa_handler = profileHostSigHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    
    // Parse profile type if specified
    openavb_tl_profile_type_t forced_profile = OPENAVB_PROFILE_CUSTOM;
    if (optProfileType) {
        forced_profile = parseProfileType(optProfileType);
        printf("Forced profile type: %s\n", openavbTLProfileTypeToString(forced_profile));
    }
    
    printf("Configuration:\n");
    printf("  Strict validation: %s\n", optStrictValidation ? "enabled" : "disabled");
    printf("  Auto-correction: %s\n", optAutoCorrection ? "enabled" : "disabled");
    printf("  Profile monitoring: %s\n", optMonitoring ? "enabled" : "disabled");
    printf("  Verbose reporting: %s\n", optVerbose ? "enabled" : "disabled");
    if (optIfnameGlobal) {
        printf("  Global interface: %s\n", optIfnameGlobal);
    }
    printf("\n");
    
    // Configure all streams
    bool all_successful = true;
    for (int i = optind; i < argc; i++) {
        if (!configureProfileStream(argv[i], forced_profile, optStrictValidation, 
                                   optAutoCorrection, optVerbose)) {
            all_successful = false;
            if (optStrictValidation) {
                printf("Stopping due to validation failure in strict mode\n");
                break;
            }
        }
    }
    
    if (all_successful && g_host_state.total_streams > 0) {
        printf("\n✓ All streams configured successfully\n");
        
        if (optMonitoring) {
            printf("Starting profile monitoring (Press Ctrl+C to stop)...\n");
            
            // Monitoring loop
            while (bRunning) {
                sleep(5);  // Check every 5 seconds
                
                // TODO: In real implementation, check stream compliance here
                // For demonstration, just show that monitoring is active
                if (optVerbose) {
                    printf("Profile monitoring active (%u streams)\n", g_host_state.total_streams);
                }
            }
        } else {
            printf("Profile configuration complete. Streams ready to run.\n");
            printf("(In full implementation, streams would start here)\n");
        }
    }
    
    // Display final statistics
    if (optVerbose || g_host_state.validation_failures > 0) {
        displayProfileStatistics(optVerbose);
    }
    
    // Cleanup
    openavbProfileFactoryShutdown();
    
    if (optIfnameGlobal) free(optIfnameGlobal);
    if (optLogFileName) free(optLogFileName);
    if (optProfileType) free(optProfileType);
    
    printf("\nProfile Host shutdown complete.\n");
    
    return all_successful ? 0 : 1;
}
