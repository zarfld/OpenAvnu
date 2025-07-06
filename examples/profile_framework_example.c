/**
 * OpenAvnu Profile Framework - Example Application
 * 
 * This example demonstrates how to use the profile framework in a
 * real OpenAvnu application with capability-based configuration.
 */

#include "openavb_profile_framework.h"
#include "openavb_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define AVB_LOG_COMPONENT "ProfileExample"

/**
 * Example stream configuration structure
 * (This would be your actual stream config in a real application)
 */
typedef struct example_stream_config {
    char dest_addr[18];
    U32 presentation_offset_ns;
    bool use_avtp_timestamps;
    bool security_enabled;
    U8 sr_class;
    U32 max_frame_size;
} example_stream_config_t;

/**
 * Print profile information
 */
void print_profile_info(const openavb_profile_cfg_t *profile) {
    printf("\n=== Profile Information ===\n");
    printf("Name: %s\n", profile->profile_name);
    printf("Version: %s\n", profile->version_string);
    
    printf("\nSecurity Capabilities:\n");
    printf("  Authentication Required: %s\n", 
           profile->capabilities.security.authentication_required ? "Yes" : "No");
    printf("  Encryption Required: %s\n", 
           profile->capabilities.security.encryption_required ? "Yes" : "No");
    
    printf("\nTiming Capabilities:\n");
    printf("  Min Presentation Offset: %u ns\n", 
           profile->capabilities.timing.min_presentation_offset_ns);
    printf("  Max Presentation Offset: %u ns\n", 
           profile->capabilities.timing.max_presentation_offset_ns);
    printf("  Sync Tolerance: %u ns\n", 
           profile->capabilities.timing.sync_uncertainty_tolerance_ns);
    
    printf("\nTransport Capabilities:\n");
    printf("  Fast Connect: %s\n", 
           profile->capabilities.transport.fast_connect_supported ? "Yes" : "No");
    printf("  Max Streams: %u\n", 
           profile->capabilities.transport.max_streams_per_entity);
    
    printf("\nQoS Capabilities:\n");
    printf("  Frame Preemption: %s\n", 
           profile->capabilities.qos.frame_preemption_supported ? "Yes" : "No");
    printf("  Max Frame Size: %u bytes\n", 
           profile->capabilities.qos.max_frame_size);
}

/**
 * Demonstrate profile-based stream configuration
 */
bool configure_stream_example(openavb_stream_handle_t *stream, 
                             const example_stream_config_t *config) {
    const openavb_profile_cfg_t *profile = openavb_stream_get_profile(stream);
    if (!profile) {
        printf("ERROR: No profile set for stream\n");
        return false;
    }
    
    printf("\n=== Configuring Stream with Profile-Based Logic ===\n");
    
    // Security configuration - pure capability-based logic
    if (profile->capabilities.security.authentication_required) {
        printf("Profile requires authentication - enabling security\n");
        
        U32 supported_auth = profile->capabilities.security.supported_auth_methods;
        if (supported_auth & AUTH_METHOD_CERTIFICATE) {
            printf("  Using certificate authentication\n");
        } else if (supported_auth & AUTH_METHOD_PSK) {
            printf("  Using PSK authentication\n");
        }
        
        U32 supported_ciphers = profile->capabilities.security.supported_cipher_suites;
        if (supported_ciphers & CIPHER_AES_256_GCM) {
            printf("  Using AES-256-GCM encryption\n");
        } else if (supported_ciphers & CIPHER_AES_128_GCM) {
            printf("  Using AES-128-GCM encryption\n");
        }
    } else {
        printf("Profile allows optional security\n");
        if (config->security_enabled) {
            printf("  Security enabled by configuration\n");
        } else {
            printf("  Security disabled by configuration\n");
        }
    }
    
    // Timing configuration - graduated based on actual values
    U32 tolerance = profile->capabilities.timing.sync_uncertainty_tolerance_ns;
    printf("Configuring timing based on tolerance: %u ns\n", tolerance);
    
    if (tolerance < 100000) {
        printf("  Using ultra-precision timing mode\n");
    } else if (tolerance < 500000) {
        printf("  Using high-precision timing mode\n");
    } else if (tolerance < 1000000) {
        printf("  Using standard timing mode\n");
    } else {
        printf("  Using relaxed timing mode\n");
    }
    
    // Validate presentation offset
    if (config->presentation_offset_ns < profile->capabilities.timing.min_presentation_offset_ns ||
        config->presentation_offset_ns > profile->capabilities.timing.max_presentation_offset_ns) {
        printf("ERROR: Presentation offset %u ns violates profile limits (%u - %u ns)\n",
               config->presentation_offset_ns,
               profile->capabilities.timing.min_presentation_offset_ns,
               profile->capabilities.timing.max_presentation_offset_ns);
        return false;
    }
    
    printf("  Presentation offset: %u ns (within profile limits)\n", 
           config->presentation_offset_ns);
    
    // QoS configuration
    if (profile->capabilities.qos.frame_preemption_supported) {
        printf("Enabling frame preemption (supported by profile)\n");
    }
    
    if (profile->capabilities.qos.credit_based_shaping_required) {
        printf("Enabling credit-based shaping (required by profile)\n");
    }
    
    return true;
}

/**
 * Demonstrate profile compatibility checking
 */
void demonstrate_profile_compatibility(void) {
    printf("\n=== Profile Compatibility Demonstration ===\n");
    
    openavb_profile_cfg_t *avb = openavb_profile_get_by_name("AVB", NULL);
    openavb_profile_cfg_t *milan = openavb_profile_get_by_name("MILAN", NULL);
    openavb_profile_cfg_t *automotive = openavb_profile_get_by_name("Automotive-A2B", NULL);
    
    if (avb && milan) {
        bool compatible = openavb_profile_is_compatible(milan, avb);
        printf("MILAN -> AVB compatibility: %s\n", compatible ? "Compatible" : "Incompatible");
        if (!compatible) {
            printf("  (MILAN requires security, AVB doesn't support it)\n");
        }
    }
    
    if (milan && automotive) {
        bool compatible = openavb_profile_is_compatible(milan, automotive);
        printf("MILAN -> Automotive compatibility: %s\n", compatible ? "Compatible" : "Incompatible");
    }
}

/**
 * Main example function
 */
int main(int argc, char *argv[]) {
    printf("OpenAvnu Profile Framework Example\n");
    printf("=================================\n");
    
    // Initialize the profile framework
    if (!openavb_profile_framework_init()) {
        printf("ERROR: Failed to initialize profile framework\n");
        return 1;
    }
    
    printf("Profile framework initialized successfully\n");
    
    // Parse configuration file if provided
    openavb_profile_cfg_t *selected_profile = NULL;
    if (argc > 1) {
        printf("\nParsing configuration file: %s\n", argv[1]);
        if (!openavb_profile_parse_config(argv[1], &selected_profile)) {
            printf("ERROR: Failed to parse configuration file\n");
            return 1;
        }
    } else {
        // Use MILAN profile as default for demo
        selected_profile = openavb_profile_get_by_name("MILAN", NULL);
        if (!selected_profile) {
            printf("ERROR: MILAN profile not found\n");
            return 1;
        }
        printf("\nUsing default MILAN profile for demonstration\n");
    }
    
    // Print selected profile information
    print_profile_info(selected_profile);
    
    // Create a stream and demonstrate configuration
    printf("\n=== Stream Configuration Example ===\n");
    openavb_stream_handle_t *stream = openavb_stream_create(NULL);
    if (!stream) {
        printf("ERROR: Failed to create stream\n");
        return 1;
    }
    
    // Set the profile for the stream
    if (!openavb_stream_set_profile(stream, selected_profile)) {
        printf("ERROR: Failed to set profile for stream\n");
        openavb_stream_destroy(stream);
        return 1;
    }
    
    // Configure stream with example settings
    example_stream_config_t config = {
        .dest_addr = "91:E0:F0:00:FE:01",
        .presentation_offset_ns = 1000000,  // 1ms
        .use_avtp_timestamps = true,
        .security_enabled = false,
        .sr_class = 0x02,  // Class A
        .max_frame_size = 1522
    };
    
    // Demonstrate capability-based configuration
    if (configure_stream_example(stream, &config)) {
        printf("Stream configuration successful\n");
        
        // Activate the stream
        if (openavb_stream_activate(stream)) {
            printf("Stream activated successfully\n");
        }
    } else {
        printf("Stream configuration failed\n");
    }
    
    // Demonstrate profile compatibility
    demonstrate_profile_compatibility();
    
    // Simulate quality metrics update
    printf("\n=== Quality Metrics Simulation ===\n");
    openavb_stream_quality_metrics_t metrics = {
        .packets_sent = 1000,
        .packets_lost = 2,
        .late_packets = 1,
        .timing_violations = 0,
        .jitter_ms = 0.1f,  // 100μs jitter
        .sync_accuracy_ppm = 10.0f,
        .buffer_underruns = 0,
        .buffer_overruns = 0
    };
    
    openavb_stream_update_metrics(stream, &metrics);
    printf("Updated stream quality metrics\n");
    
    // Test with higher jitter to trigger profile logic
    metrics.jitter_ms = 2.0f;  // 2ms jitter - should trigger warning for MILAN
    openavb_stream_update_metrics(stream, &metrics);
    
    // Cleanup
    openavb_stream_deactivate(stream);
    openavb_stream_destroy(stream);
    openavb_profile_framework_cleanup();
    
    printf("\nExample completed successfully\n");
    return 0;
}
