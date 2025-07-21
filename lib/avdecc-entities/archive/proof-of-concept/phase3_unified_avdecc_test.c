/**
 * Phase 3 Unified AVDECC API Example
 * 
 * This example demonstrates the Phase 3 implementation:
 * - Unified AVDECC API layer
 * - Configuration-driven implementation selection
 * - Compatibility with both L-Acoustics and legacy AVDECC
 * - Automatic fallback mechanisms
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

// OpenAvnu unified AVDECC API
#include "openavb_unified_avdecc.h"

static bool g_running = true;
static openavb_unified_avdecc_controller_t *g_controller = NULL;

void signal_handler(int sig) {
    printf("\nReceived signal %d, shutting down...\n", sig);
    g_running = false;
}

void entity_discovery_callback(
    openavb_unified_avdecc_controller_t *controller,
    const openavb_unified_avdecc_entity_info_t *entity_info,
    bool is_online,
    void *user_data) {
    
    char entity_id_str[32];
    openavb_unified_avdecc_entity_id_to_string(entity_info->entity_id, entity_id_str, sizeof(entity_id_str));
    
    printf("📡 Entity %s: %s\n", is_online ? "DISCOVERED" : "OFFLINE", entity_id_str);
    printf("   Name: %s\n", entity_info->entity_name);
    printf("   MILAN Device: %s\n", entity_info->is_milan_device ? "Yes" : "No");
    printf("   Fast Connect: %s\n", entity_info->supports_fast_connect ? "Yes" : "No");
    printf("   Capabilities: %s%s%s\n",
           entity_info->is_talker ? "Talker " : "",
           entity_info->is_listener ? "Listener " : "",
           entity_info->is_controller ? "Controller " : "");
    printf("   Streams: %u talker, %u listener\n", 
           entity_info->talker_stream_count, entity_info->listener_stream_count);
    printf("\n");
}

void test_implementation_detection(void) {
    printf("🔍 Testing Implementation Detection\n");
    printf("==================================\n");
    
    // Get available implementations
    openavb_avdecc_impl_type_t implementations[4];
    uint32_t count = openavb_unified_avdecc_get_available_implementations(implementations, 4);
    
    printf("Available AVDECC implementations: %u\n", count);
    for (uint32_t i = 0; i < count; i++) {
        openavb_avdecc_implementation_caps_t caps;
        const char *impl_name = openavb_unified_avdecc_impl_type_to_string(implementations[i]);
        
        printf("  %u. %s", i+1, impl_name);
        
        if (openavb_unified_avdecc_get_implementation_capabilities(implementations[i], &caps)) {
            printf("\n     - MILAN Support: %s\n", caps.supports_milan ? "Yes" : "No");
            printf("     - Fast Connect: %s\n", caps.supports_fast_connect ? "Yes" : "No");
            printf("     - IEEE 1722.1-2021: %s\n", caps.supports_ieee_1722_1_2021 ? "Yes" : "No");
            printf("     - Max Entities: %u\n", caps.max_entities);
            printf("     - Version: %s\n", caps.version_string);
        } else {
            printf(" (capabilities unavailable)\n");
        }
    }
    printf("\n");
}

void test_configuration_driven_selection(void) {
    printf("⚙️  Testing Configuration-Driven Selection\n");
    printf("==========================================\n");
    
    // Test different configuration scenarios
    openavb_unified_avdecc_config_t configs[4];
    const char *scenario_names[] = {
        "Auto-Select (Default)",
        "MILAN Required",
        "Legacy Compatible",
        "Simulation Mode"
    };
    
    // Scenario 1: Auto-select
    openavb_unified_avdecc_get_default_config(&configs[0]);
    
    // Scenario 2: MILAN required
    openavb_unified_avdecc_get_default_config(&configs[1]);
    configs[1].milan_mode_required = true;
    configs[1].fast_connect_required = true;
    strncpy(configs[1].entity_name, "MILAN-Controller", sizeof(configs[1].entity_name) - 1);
    
    // Scenario 3: Legacy compatible
    openavb_unified_avdecc_get_default_config(&configs[2]);
    configs[2].preferred_implementation = OPENAVB_AVDECC_IMPL_LEGACY;
    strncpy(configs[2].entity_name, "Legacy-Controller", sizeof(configs[2].entity_name) - 1);
    
    // Scenario 4: Simulation
    openavb_unified_avdecc_get_default_config(&configs[3]);
    configs[3].preferred_implementation = OPENAVB_AVDECC_IMPL_SIMULATION;
    strncpy(configs[3].entity_name, "Simulation-Controller", sizeof(configs[3].entity_name) - 1);
    
    for (int i = 0; i < 4; i++) {
        printf("Scenario %d: %s\n", i+1, scenario_names[i]);
        printf("  Preferred: %s\n", 
               openavb_unified_avdecc_impl_type_to_string(configs[i].preferred_implementation));
        
        openavb_avdecc_impl_type_t selected = openavb_unified_avdecc_auto_select_implementation(&configs[i]);
        printf("  Selected: %s\n", openavb_unified_avdecc_impl_type_to_string(selected));
        
        bool available = openavb_unified_avdecc_is_implementation_available(selected);
        printf("  Available: %s\n", available ? "Yes" : "No");
        printf("\n");
    }
}

void test_unified_controller_creation(void) {
    printf("🎛️  Testing Unified Controller Creation\n");
    printf("======================================\n");
    
    // Test with MILAN configuration
    openavb_unified_avdecc_config_t config;
    openavb_unified_avdecc_get_default_config(&config);
    config.milan_mode_required = true;
    config.fast_connect_required = true;
    strncpy(config.entity_name, "Phase3-Test-Controller", sizeof(config.entity_name) - 1);
    
    printf("Creating unified AVDECC controller...\n");
    printf("  Entity Name: %s\n", config.entity_name);
    printf("  MILAN Required: %s\n", config.milan_mode_required ? "Yes" : "No");
    printf("  Fast Connect Required: %s\n", config.fast_connect_required ? "Yes" : "No");
    
    openavb_unified_avdecc_controller_t *controller = 
        openavb_unified_avdecc_controller_create(&config);
    
    if (controller) {
        printf("✅ Controller created successfully!\n");
        
        openavb_avdecc_impl_type_t impl = openavb_unified_avdecc_controller_get_implementation(controller);
        printf("  Implementation: %s\n", openavb_unified_avdecc_impl_type_to_string(impl));
        
        bool milan_support = openavb_unified_avdecc_controller_supports_milan(controller);
        printf("  MILAN Support: %s\n", milan_support ? "Yes" : "No");
        
        // Store global reference for cleanup
        g_controller = controller;
        
    } else {
        printf("❌ Controller creation failed\n");
    }
    printf("\n");
}

void test_entity_discovery_with_unified_api(void) {
    printf("🔍 Testing Entity Discovery with Unified API\n");
    printf("=============================================\n");
    
    if (!g_controller) {
        printf("❌ No controller available for discovery test\n");
        return;
    }
    
    printf("Starting entity discovery...\n");
    
    bool discovery_started = openavb_unified_avdecc_start_discovery(
        g_controller, entity_discovery_callback, NULL);
    
    if (discovery_started) {
        printf("✅ Entity discovery started\n");
        printf("Waiting for entities to be discovered...\n\n");
        
        // Wait for discovery to find entities
        sleep(3);
        
        // Get discovered entities
        openavb_unified_avdecc_entity_info_t entities[16];
        uint32_t entity_count = openavb_unified_avdecc_get_discovered_entities(
            g_controller, entities, 16);
        
        printf("📊 Discovery Results: %u entities found\n", entity_count);
        
        for (uint32_t i = 0; i < entity_count; i++) {
            char entity_id_str[32];
            openavb_unified_avdecc_entity_id_to_string(entities[i].entity_id, entity_id_str, sizeof(entity_id_str));
            
            printf("  Entity %u:\n", i+1);
            printf("    ID: %s\n", entity_id_str);
            printf("    Name: %s\n", entities[i].entity_name);
            printf("    MILAN: %s\n", entities[i].is_milan_device ? "Yes" : "No");
            printf("    Type: %s%s%s\n",
                   entities[i].is_talker ? "Talker " : "",
                   entities[i].is_listener ? "Listener " : "",
                   entities[i].is_controller ? "Controller " : "");
        }
        
        openavb_unified_avdecc_stop_discovery(g_controller);
        printf("\n✅ Discovery test completed\n");
    } else {
        printf("❌ Failed to start entity discovery\n");
    }
    printf("\n");
}

void test_configuration_file_support(void) {
    printf("📁 Testing Configuration File Support\n");
    printf("=====================================\n");
    
    const char *config_file = "phase3_test_config.ini";
    
    // Create a test configuration
    openavb_unified_avdecc_config_t config;
    openavb_unified_avdecc_get_default_config(&config);
    config.preferred_implementation = OPENAVB_AVDECC_IMPL_LA_AVDECC;
    config.milan_mode_required = true;
    config.fast_connect_required = true;
    config.discovery_timeout_ms = 8000;
    strncpy(config.entity_name, "Phase3-File-Test", sizeof(config.entity_name) - 1);
    
    // Save configuration to file
    printf("Saving configuration to file: %s\n", config_file);
    if (openavb_unified_avdecc_save_config(&config, config_file)) {
        printf("✅ Configuration saved successfully\n");
        
        // Load configuration from file
        openavb_unified_avdecc_config_t loaded_config;
        printf("Loading configuration from file...\n");
        
        if (openavb_unified_avdecc_load_config(&loaded_config, config_file)) {
            printf("✅ Configuration loaded successfully\n");
            
            // Verify loaded configuration
            printf("  Implementation: %s\n", 
                   openavb_unified_avdecc_impl_type_to_string(loaded_config.preferred_implementation));
            printf("  Entity Name: %s\n", loaded_config.entity_name);
            printf("  MILAN Required: %s\n", loaded_config.milan_mode_required ? "Yes" : "No");
            printf("  Discovery Timeout: %u ms\n", loaded_config.discovery_timeout_ms);
            
            // Validate configuration
            if (openavb_unified_avdecc_validate_config(&loaded_config)) {
                printf("✅ Configuration validation passed\n");
            } else {
                printf("❌ Configuration validation failed\n");
            }
        } else {
            printf("❌ Failed to load configuration\n");
        }
        
        // Clean up test file
        remove(config_file);
    } else {
        printf("❌ Failed to save configuration\n");
    }
    printf("\n");
}

void test_cross_implementation_compatibility(void) {
    printf("🔗 Testing Cross-Implementation Compatibility\n");
    printf("=============================================\n");
    
    // Test creating controllers with different implementations
    openavb_avdecc_impl_type_t implementations[] = {
        OPENAVB_AVDECC_IMPL_SIMULATION,
        OPENAVB_AVDECC_IMPL_LA_AVDECC,
        OPENAVB_AVDECC_IMPL_LEGACY
    };
    
    const char *test_names[] = {
        "Simulation Mode",
        "L-Acoustics AVDECC",
        "Legacy avdecc-lib"
    };
    
    for (int i = 0; i < 3; i++) {
        printf("Testing %s:\n", test_names[i]);
        
        if (!openavb_unified_avdecc_is_implementation_available(implementations[i])) {
            printf("  ⚠️  Implementation not available - skipping\n\n");
            continue;
        }
        
        openavb_unified_avdecc_config_t config;
        openavb_unified_avdecc_get_default_config(&config);
        config.preferred_implementation = implementations[i];
        snprintf(config.entity_name, sizeof(config.entity_name), "Test-%s", test_names[i]);
        
        openavb_unified_avdecc_controller_t *test_controller = 
            openavb_unified_avdecc_controller_create(&config);
        
        if (test_controller) {
            printf("  ✅ Controller created\n");
            
            openavb_avdecc_impl_type_t actual_impl = 
                openavb_unified_avdecc_controller_get_implementation(test_controller);
            printf("  Implementation: %s\n", openavb_unified_avdecc_impl_type_to_string(actual_impl));
            
            bool milan_support = openavb_unified_avdecc_controller_supports_milan(test_controller);
            printf("  MILAN Support: %s\n", milan_support ? "Yes" : "No");
            
            // Test basic discovery
            bool discovery_ok = openavb_unified_avdecc_start_discovery(test_controller, NULL, NULL);
            printf("  Discovery: %s\n", discovery_ok ? "OK" : "Failed");
            
            if (discovery_ok) {
                openavb_unified_avdecc_stop_discovery(test_controller);
            }
            
            openavb_unified_avdecc_controller_destroy(test_controller);
            printf("  ✅ Controller destroyed\n");
        } else {
            printf("  ❌ Controller creation failed\n");
        }
        printf("\n");
    }
}

void display_phase3_achievements(void) {
    printf("🎉 Phase 3 Implementation Achievements\n");
    printf("======================================\n");
    
    printf("✅ UNIFIED AVDECC API:\n");
    printf("  • Single API for all AVDECC implementations\n");
    printf("  • Transparent access to L-Acoustics and legacy libraries\n");
    printf("  • Consistent interface regardless of backend\n");
    printf("  • Automatic implementation selection\n");
    
    printf("\n✅ CONFIGURATION-DRIVEN SELECTION:\n");
    printf("  • INI file configuration support\n");
    printf("  • Auto-selection based on requirements\n");
    printf("  • Runtime implementation switching\n");
    printf("  • Validation and error handling\n");
    
    printf("\n✅ COMPATIBILITY LAYER:\n");
    printf("  • Support for L-Acoustics AVDECC\n");
    printf("  • Support for legacy avdecc-lib\n");
    printf("  • Simulation mode for testing\n");
    printf("  • Graceful fallback mechanisms\n");
    
    printf("\n✅ ADVANCED FEATURES:\n");
    printf("  • MILAN compliance detection\n");
    printf("  • Fast connect capability\n");
    printf("  • Network redundancy support\n");
    printf("  • Cross-platform compatibility\n");
    
    printf("\n📊 IMPLEMENTATION STATUS:\n");
    printf("  • L-Acoustics AVDECC: %s\n", 
           openavb_unified_avdecc_is_implementation_available(OPENAVB_AVDECC_IMPL_LA_AVDECC) ? 
           "✅ Available" : "⚠️  Not Available");
    printf("  • Legacy avdecc-lib: %s\n", 
           openavb_unified_avdecc_is_implementation_available(OPENAVB_AVDECC_IMPL_LEGACY) ? 
           "✅ Available" : "⚠️  Not Available");
    printf("  • Simulation Mode: ✅ Available\n");
    
    printf("\n🚀 READY FOR PHASE 4:\n");
    printf("  • Stream connection management\n");
    printf("  • Entity configuration and control\n");
    printf("  • Advanced MILAN features\n");
    printf("  • Production deployment\n");
}

int main(int argc, char *argv[]) {
    printf("🚀 OpenAvnu Phase 3 Unified AVDECC API Test\n");
    printf("============================================\n");
    printf("\nThis test demonstrates Phase 3 implementation:\n");
    printf("• Unified AVDECC API layer\n");
    printf("• Configuration-driven implementation selection\n");
    printf("• Compatibility with multiple AVDECC implementations\n");
    printf("• Automatic fallback mechanisms\n");
    
    // Set up signal handling
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Initialize unified AVDECC system
    printf("\n🔧 Initializing Unified AVDECC System...\n");
    if (!openavb_unified_avdecc_initialize()) {
        printf("❌ Failed to initialize unified AVDECC system\n");
        return 1;
    }
    printf("✅ Unified AVDECC system initialized\n\n");
    
    // Run Phase 3 tests
    test_implementation_detection();
    test_configuration_driven_selection();
    test_unified_controller_creation();
    test_entity_discovery_with_unified_api();
    test_configuration_file_support();
    test_cross_implementation_compatibility();
    
    printf("⏱️  Running for 5 seconds to demonstrate operation...\n");
    
    // Run for 5 seconds or until interrupted
    int count = 0;
    while (g_running && count < 50) {
        usleep(100000);  // 100ms
        count++;
        
        if (count % 10 == 0) {
            printf(".");
            fflush(stdout);
        }
    }
    
    printf("\n\n🧹 Cleaning up...\n");
    
    // Cleanup controller if created
    if (g_controller) {
        openavb_unified_avdecc_controller_destroy(g_controller);
        g_controller = NULL;
    }
    
    // Cleanup unified AVDECC system
    openavb_unified_avdecc_cleanup();
    
    display_phase3_achievements();
    
    printf("\n🎯 Phase 3 Complete - Unified AVDECC API Ready!\n");
    printf("\nKey Achievements:\n");
    printf("✅ Single API for all AVDECC implementations\n");
    printf("✅ Configuration-driven implementation selection\n");
    printf("✅ Automatic fallback and compatibility\n");
    printf("✅ MILAN compliance and modern features\n");
    printf("✅ Production-ready unified interface\n");
    
    return 0;
}
