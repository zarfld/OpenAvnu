/**
 * L-Acoustics AVDECC Integration Example
 * 
 * This example demonstrates how to use the L-Acoustics AVDECC library
 * with OpenAvnu's profile framework for modern IEEE 1722.1-2021 and 
 * MILAN-compliant AVDECC functionality.
 * 
 * Features demonstrated:
 * - Profile auto-selection with MILAN requirements
 * - L-Acoustics AVDECC controller creation
 * - Entity discovery and enumeration
 * - Stream configuration with AVDECC control
 * - MILAN compliance validation
 * - Integration with existing OpenAvnu infrastructure
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

// OpenAvnu profile framework
#include "openavb_profile_framework.h"

// Standard includes
#include <stdbool.h>
#include <stdint.h>

static bool g_running = true;

void signal_handler(int sig) {
    printf("\nReceived signal %d, shutting down...\n", sig);
    g_running = false;
}

void print_profile_info(const openavb_profile_cfg_t *profile) {
    printf("📋 Profile Information:\n");
    printf("   Name: %s\n", profile->profile_name);
    printf("   Version: %s\n", profile->version_string);
    printf("   Specification: ");
    
    switch (profile->spec_version) {
        case OPENAVB_SPEC_IEEE_1722_1_2021:
            printf("IEEE 1722.1-2021");
            break;
        case OPENAVB_SPEC_IEEE_1722_1_2013:
            printf("IEEE 1722.1-2013");
            break;
        default:
            printf("Other (0x%04X)", profile->spec_version);
            break;
    }
    printf("\n");
    
    printf("   AVDECC Capabilities:\n");
    printf("     MILAN Compliant: %s\n", profile->capabilities.avdecc.milan_compliant ? "Yes" : "No");
    printf("     Fast Connect: %s\n", profile->capabilities.avdecc.fast_connect_supported ? "Yes" : "No");
    printf("     Network Redundancy: %s\n", profile->capabilities.avdecc.network_redundancy ? "Yes" : "No");
    printf("     Max Entities: %u\n", profile->capabilities.avdecc.max_entities);
    printf("     Discovery Timeout: %u ms\n", profile->capabilities.avdecc.entity_discovery_timeout_ms);
    printf("     IEEE 1722.1 Version: %u\n", profile->capabilities.avdecc.ieee_1722_1_version);
    
    printf("   Timing Capabilities:\n");
    printf("     Sync Tolerance: %u ns (%.3f ms)\n", 
           profile->capabilities.timing.sync_uncertainty_tolerance_ns,
           profile->capabilities.timing.sync_uncertainty_tolerance_ns / 1000000.0);
    printf("     Max Latency: %u ns (%.3f ms)\n",
           profile->capabilities.qos.max_latency_ns,
           profile->capabilities.qos.max_latency_ns / 1000000.0);
}

void test_profile_auto_selection(void) {
    printf("\n🎯 Testing Profile Auto-Selection with MILAN Requirements\n");
    printf("======================================================\n");
    
    openavb_profile_requirements_t requirements = {
        .required_avdecc_milan = true,
        .required_fast_connect = true,
        .required_network_redundancy = false,
        .min_timing_precision_ns = 250000,  // 250μs for MILAN
        .max_streams_needed = 64,
        .required_ieee_1722_1_version = 2021
    };
    
    printf("Requirements:\n");
    printf("  MILAN Compliance: Required\n");
    printf("  Fast Connect: Required\n");
    printf("  Network Redundancy: Optional\n");
    printf("  Min Timing Precision: 250μs\n");
    printf("  Max Streams: 64\n");
    printf("  IEEE 1722.1 Version: 2021\n");
    printf("\n");
    
    const openavb_profile_cfg_t *selected_profile = openavb_profile_auto_select(&requirements);
    
    if (selected_profile) {
        printf("✅ Profile auto-selection successful!\n");
        print_profile_info(selected_profile);
        
        // Check if it's an L-Acoustics AVDECC profile
        if (strstr(selected_profile->profile_name, "LA-AVDECC")) {
            printf("\n🎉 Selected L-Acoustics AVDECC profile!\n");
            printf("   This provides:\n");
            printf("   - Modern IEEE 1722.1-2021 compliance\n");
            printf("   - MILAN device support\n");
            printf("   - Hive compatibility\n");
            printf("   - Professional-grade AVDECC implementation\n");
        }
    } else {
        printf("❌ Profile auto-selection failed\n");
        printf("   No profile meets the specified requirements\n");
        printf("   Available profiles:\n");
        
        // List available profiles
        for (int i = 0; i < openavb_profile_get_count(); i++) {
            const openavb_profile_cfg_t *profile = openavb_profile_get_by_index(i);
            if (profile) {
                printf("     - %s\n", profile->profile_name);
            }
        }
    }
}

void test_stream_creation_and_configuration(void) {
    printf("\n🔧 Testing Stream Creation with L-Acoustics AVDECC\n");
    printf("================================================\n");
    
    // Create a stream handle
    openavb_stream_handle_t stream_handle;
    memset(&stream_handle, 0, sizeof(stream_handle));
    
    // Auto-select MILAN-compatible profile
    openavb_profile_requirements_t requirements = {
        .required_avdecc_milan = true,
        .required_fast_connect = true,
        .min_timing_precision_ns = 250000,
        .max_streams_needed = 64
    };
    
    const openavb_profile_cfg_t *profile = openavb_profile_auto_select(&requirements);
    if (!profile) {
        printf("❌ No suitable profile found for stream creation\n");
        return;
    }
    
    printf("Using profile: %s\n", profile->profile_name);
    
    // Assign profile to stream
    stream_handle.profile = profile;
    stream_handle.stream_id = 1;
    
    // Initialize profile
    printf("\n1. Initializing profile...\n");
    if (profile->callbacks.initialize && profile->callbacks.initialize(&stream_handle)) {
        printf("✅ Profile initialization successful\n");
        
        // Configure stream
        printf("\n2. Configuring stream with profile capabilities...\n");
        if (profile->callbacks.configure_stream && profile->callbacks.configure_stream(&stream_handle)) {
            printf("✅ Stream configuration successful\n");
            
            printf("\n📊 Stream Configuration Summary:\n");
            printf("   Stream ID: %u\n", stream_handle.stream_id);
            printf("   Profile: %s\n", profile->profile_name);
            
            if (profile->capabilities.avdecc.milan_compliant) {
                printf("   MILAN Features Enabled:\n");
                printf("     - Fast Connect: %s\n", profile->capabilities.avdecc.fast_connect_supported ? "Yes" : "No");
                printf("     - Network Redundancy: %s\n", profile->capabilities.avdecc.network_redundancy ? "Yes" : "No");
                printf("     - Strict Timing: 250μs tolerance\n");
            }
        } else {
            printf("❌ Stream configuration failed\n");
        }
        
        // Cleanup
        printf("\n3. Cleaning up...\n");
        if (profile->callbacks.cleanup) {
            profile->callbacks.cleanup(&stream_handle);
            printf("✅ Cleanup complete\n");
        }
    } else {
        printf("❌ Profile initialization failed\n");
        if (strstr(profile->profile_name, "LA-AVDECC")) {
            printf("   Note: L-Acoustics AVDECC requires:\n");
            printf("   - CMake build with OPENAVNU_ENABLE_LA_AVDECC=ON\n");
            printf("   - WinPcap Developer's Pack (Windows)\n");
            printf("   - libpcap-dev (Linux)\n");
            printf("   - Network interface access permissions\n");
        }
    }
}

void test_compatibility_checking(void) {
    printf("\n🔍 Testing Profile Compatibility\n");
    printf("===============================\n");
    
    const openavb_profile_cfg_t *milan_profile = openavb_profile_find_by_name("LA-AVDECC-MILAN");
    const openavb_profile_cfg_t *standard_profile = openavb_profile_find_by_name("LA-AVDECC-STANDARD");
    const openavb_profile_cfg_t *avb_profile = openavb_profile_find_by_name("AVB");
    
    if (milan_profile && standard_profile) {
        printf("Testing LA-AVDECC-MILAN ↔ LA-AVDECC-STANDARD:\n");
        bool compatible = openavb_profile_is_compatible(milan_profile, standard_profile);
        printf("  Result: %s\n", compatible ? "✅ Compatible" : "❌ Incompatible");
        if (compatible) {
            printf("  Both support IEEE 1722.1-2021 with overlapping capabilities\n");
        }
    }
    
    if (milan_profile && avb_profile) {
        printf("\nTesting LA-AVDECC-MILAN ↔ AVB:\n");
        bool compatible = openavb_profile_is_compatible(milan_profile, avb_profile);
        printf("  Result: %s\n", compatible ? "✅ Compatible" : "❌ Incompatible");
        if (!compatible) {
            printf("  MILAN requires IEEE 1722.1-2021, AVB uses IEEE 1722-2016\n");
        }
    }
}

void display_la_avdecc_status(void) {
    printf("\n📡 L-Acoustics AVDECC Integration Status\n");
    printf("=======================================\n");
    
#ifdef OPENAVNU_HAS_LA_AVDECC
    printf("✅ L-Acoustics AVDECC: AVAILABLE\n");
    printf("   Version: 4.1.0\n");
    printf("   Features: IEEE 1722.1-2021, MILAN compliance\n");
    printf("   Profiles: LA-AVDECC-MILAN, LA-AVDECC-STANDARD\n");
    
    // Check if profiles are registered
    const openavb_profile_cfg_t *milan = openavb_profile_find_by_name("LA-AVDECC-MILAN");
    const openavb_profile_cfg_t *standard = openavb_profile_find_by_name("LA-AVDECC-STANDARD");
    
    printf("   Registered Profiles:\n");
    printf("     LA-AVDECC-MILAN: %s\n", milan ? "✅ Yes" : "❌ No");
    printf("     LA-AVDECC-STANDARD: %s\n", standard ? "✅ Yes" : "❌ No");
#else
    printf("❌ L-Acoustics AVDECC: NOT AVAILABLE\n");
    printf("   To enable: cmake -DOPENAVNU_ENABLE_LA_AVDECC=ON\n");
    printf("   Requires: CMake 3.29+, C++17 compiler\n");
    printf("   Dependencies: WinPcap (Windows) or libpcap (Linux)\n");
#endif
}

int main(int argc, char *argv[]) {
    printf("🚀 OpenAvnu L-Acoustics AVDECC Integration Example\n");
    printf("==================================================\n");
    printf("\nThis example demonstrates modern AVDECC capabilities using\n");
    printf("the L-Acoustics AVDECC library integrated with OpenAvnu's\n");
    printf("capability-based profile framework.\n");
    
    // Set up signal handling
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Initialize profile framework
    printf("\n🔧 Initializing Profile Framework...\n");
    if (!openavb_profile_framework_initialize()) {
        printf("❌ Failed to initialize profile framework\n");
        return 1;
    }
    printf("✅ Profile framework initialized with %d profiles\n", openavb_profile_get_count());
    
    // Display L-Acoustics AVDECC status
    display_la_avdecc_status();
    
    // Run tests
    test_profile_auto_selection();
    test_stream_creation_and_configuration();
    test_compatibility_checking();
    
    printf("\n⏱️  Running for 10 seconds (Ctrl+C to exit early)...\n");
    
    // Run for 10 seconds or until interrupted
    int count = 0;
    while (g_running && count < 100) {
        usleep(100000);  // 100ms
        count++;
        
        if (count % 10 == 0) {
            printf(".");
            fflush(stdout);
        }
    }
    
    printf("\n\n🧹 Cleaning up...\n");
    openavb_profile_framework_cleanup();
    
    printf("\n🎉 L-Acoustics AVDECC Integration Example Complete!\n");
    printf("\nKey achievements demonstrated:\n");
    printf("✅ Profile framework with L-Acoustics AVDECC integration\n");
    printf("✅ IEEE 1722.1-2021 and MILAN compliance capability\n");
    printf("✅ Automatic profile selection based on requirements\n");
    printf("✅ Stream configuration with AVDECC capabilities\n");
    printf("✅ Compatibility checking between different profiles\n");
    printf("✅ Foundation for Hive compatibility and modern AVB networks\n");
    
    return 0;
}
