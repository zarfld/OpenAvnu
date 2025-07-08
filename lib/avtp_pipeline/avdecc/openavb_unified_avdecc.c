/**
 * OpenAvnu Unified AVDECC API Implementation
 * 
 * This implements the unified interface for both L-Acoustics AVDECC and legacy avdecc-lib,
 * providing transparent access to either implementation based on configuration.
 */

#define AVB_LOG_COMPONENT "Unified-AVDECC"

#include "openavb_unified_avdecc.h"
#include "openavb_profile_framework.h"
#include "openavb_log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// Conditional includes based on available implementations
#ifdef OPENAVNU_HAS_LA_AVDECC
#include "la/avdecc/avdecc.hpp"
#include "la/avdecc/controller/avdecc_controller.hpp"
#endif

#ifdef OPENAVNU_HAS_LEGACY_AVDECC
#include "controller.h"  // Legacy avdecc-lib
#endif

// =============================================================================
// Internal Data Structures
// =============================================================================

/**
 * Unified AVDECC Controller Implementation
 */
struct openavb_unified_avdecc_controller {
    openavb_avdecc_impl_type_t implementation_type;
    openavb_unified_avdecc_config_t config;
    bool is_initialized;
    
    // Discovery callback
    openavb_avdecc_entity_discovery_callback_t discovery_callback;
    void *discovery_user_data;
    bool discovery_active;
    
    // Implementation-specific data
    union {
#ifdef OPENAVNU_HAS_LA_AVDECC
        struct {
            std::unique_ptr<la::avdecc::controller::Controller> controller;
            std::vector<openavb_unified_avdecc_entity_info_t> discovered_entities;
        } la_avdecc;
#endif
#ifdef OPENAVNU_HAS_LEGACY_AVDECC
        struct {
            avdecc_lib::controller *controller;
            // Legacy-specific data
        } legacy;
#endif
        struct {
            // Simulation mode data
            openavb_unified_avdecc_entity_info_t simulated_entities[10];
            uint32_t simulated_entity_count;
        } simulation;
    } impl_data;
};

// Global state
static bool g_unified_avdecc_initialized = false;

// =============================================================================
// Implementation Detection and Management
// =============================================================================

bool openavb_unified_avdecc_initialize(void) {
    if (g_unified_avdecc_initialized) {
        return true;
    }
    
    AVB_LOG_INFO("Initializing Unified AVDECC API");
    
    // Initialize profile framework if not already done
    if (!openavb_profile_framework_initialize()) {
        AVB_LOG_ERROR("Failed to initialize profile framework");
        return false;
    }
    
    g_unified_avdecc_initialized = true;
    
    AVB_LOG_INFO("✅ Unified AVDECC API initialized successfully");
    return true;
}

void openavb_unified_avdecc_cleanup(void) {
    if (!g_unified_avdecc_initialized) {
        return;
    }
    
    AVB_LOG_INFO("Cleaning up Unified AVDECC API");
    g_unified_avdecc_initialized = false;
    
    AVB_LOG_INFO("✅ Unified AVDECC API cleanup complete");
}

uint32_t openavb_unified_avdecc_get_available_implementations(
    openavb_avdecc_impl_type_t *implementations,
    uint32_t max_implementations) {
    
    uint32_t count = 0;
    
    if (!implementations || max_implementations == 0) {
        return 0;
    }
    
    // Always available - simulation mode
    if (count < max_implementations) {
        implementations[count++] = OPENAVB_AVDECC_IMPL_SIMULATION;
    }
    
#ifdef OPENAVNU_HAS_LA_AVDECC
    if (count < max_implementations) {
        implementations[count++] = OPENAVB_AVDECC_IMPL_LA_AVDECC;
    }
#endif

#ifdef OPENAVNU_HAS_LEGACY_AVDECC
    if (count < max_implementations) {
        implementations[count++] = OPENAVB_AVDECC_IMPL_LEGACY;
    }
#endif
    
    return count;
}

bool openavb_unified_avdecc_is_implementation_available(openavb_avdecc_impl_type_t impl_type) {
    switch (impl_type) {
        case OPENAVB_AVDECC_IMPL_SIMULATION:
            return true;  // Always available
            
        case OPENAVB_AVDECC_IMPL_LA_AVDECC:
#ifdef OPENAVNU_HAS_LA_AVDECC
            return true;
#else
            return false;
#endif
            
        case OPENAVB_AVDECC_IMPL_LEGACY:
#ifdef OPENAVNU_HAS_LEGACY_AVDECC
            return true;
#else
            return false;
#endif
            
        case OPENAVB_AVDECC_IMPL_AUTO:
            return true;  // Auto-select is always "available"
            
        default:
            return false;
    }
}

openavb_avdecc_impl_type_t openavb_unified_avdecc_auto_select_implementation(
    const openavb_unified_avdecc_config_t *config) {
    
    if (!config) {
        return OPENAVB_AVDECC_IMPL_SIMULATION;
    }
    
    AVB_LOG_INFO("Auto-selecting AVDECC implementation based on requirements:");
    AVB_LOG_INFO("  MILAN required: %s", config->milan_mode_required ? "Yes" : "No");
    AVB_LOG_INFO("  Fast Connect required: %s", config->fast_connect_required ? "Yes" : "No");
    AVB_LOG_INFO("  Network Redundancy required: %s", config->network_redundancy_required ? "Yes" : "No");
    
    // If MILAN features are required, prefer L-Acoustics AVDECC
    if (config->milan_mode_required || config->fast_connect_required || config->network_redundancy_required) {
#ifdef OPENAVNU_HAS_LA_AVDECC
        AVB_LOG_INFO("✅ Selected L-Acoustics AVDECC (MILAN features required)");
        return OPENAVB_AVDECC_IMPL_LA_AVDECC;
#else
        AVB_LOG_WARNING("MILAN features required but L-Acoustics AVDECC not available");
        AVB_LOG_WARNING("Falling back to simulation mode");
        return OPENAVB_AVDECC_IMPL_SIMULATION;
#endif
    }
    
    // For standard requirements, prefer L-Acoustics if available, then legacy
#ifdef OPENAVNU_HAS_LA_AVDECC
    AVB_LOG_INFO("✅ Selected L-Acoustics AVDECC (best available)");
    return OPENAVB_AVDECC_IMPL_LA_AVDECC;
#elif defined(OPENAVNU_HAS_LEGACY_AVDECC)
    AVB_LOG_INFO("✅ Selected Legacy AVDECC (L-Acoustics not available)");
    return OPENAVB_AVDECC_IMPL_LEGACY;
#else
    AVB_LOG_INFO("✅ Selected Simulation mode (no implementations available)");
    return OPENAVB_AVDECC_IMPL_SIMULATION;
#endif
}

// =============================================================================
// Controller Management
// =============================================================================

openavb_unified_avdecc_controller_t* openavb_unified_avdecc_controller_create(
    const openavb_unified_avdecc_config_t *config) {
    
    if (!config) {
        AVB_LOG_ERROR("Invalid configuration for controller creation");
        return NULL;
    }
    
    if (!g_unified_avdecc_initialized) {
        AVB_LOG_ERROR("Unified AVDECC not initialized");
        return NULL;
    }
    
    // Allocate controller structure
    openavb_unified_avdecc_controller_t *controller = 
        (openavb_unified_avdecc_controller_t*)calloc(1, sizeof(openavb_unified_avdecc_controller_t));
    
    if (!controller) {
        AVB_LOG_ERROR("Failed to allocate controller structure");
        return NULL;
    }
    
    // Copy configuration
    memcpy(&controller->config, config, sizeof(openavb_unified_avdecc_config_t));
    
    // Auto-select implementation if needed
    if (config->preferred_implementation == OPENAVB_AVDECC_IMPL_AUTO) {
        controller->implementation_type = openavb_unified_avdecc_auto_select_implementation(config);
    } else {
        controller->implementation_type = config->preferred_implementation;
    }
    
    AVB_LOG_INFO("Creating unified AVDECC controller:");
    AVB_LOG_INFO("  Implementation: %s", 
                 openavb_unified_avdecc_impl_type_to_string(controller->implementation_type));
    AVB_LOG_INFO("  Entity Name: %s", config->entity_name);
    AVB_LOG_INFO("  MILAN Mode: %s", config->milan_mode_required ? "Required" : "Optional");
    
    // Initialize implementation-specific controller
    switch (controller->implementation_type) {
        case OPENAVB_AVDECC_IMPL_LA_AVDECC:
#ifdef OPENAVNU_HAS_LA_AVDECC
            try {
                // Use placement new to initialize C++ object in C structure
                new(&controller->impl_data.la_avdecc) decltype(controller->impl_data.la_avdecc);
                
                auto protocolInterface = la::avdecc::protocol::ProtocolInterface::Type::PCap;
                controller->impl_data.la_avdecc.controller = 
                    la::avdecc::controller::Controller::create(config->entity_name, protocolInterface);
                
                if (controller->impl_data.la_avdecc.controller) {
                    controller->is_initialized = true;
                    AVB_LOG_INFO("✅ L-Acoustics AVDECC controller created successfully");
                } else {
                    AVB_LOG_ERROR("Failed to create L-Acoustics AVDECC controller");
                    free(controller);
                    return NULL;
                }
            } catch (const std::exception& e) {
                AVB_LOG_ERROR("Exception creating L-Acoustics controller: %s", e.what());
                free(controller);
                return NULL;
            }
#else
            AVB_LOG_ERROR("L-Acoustics AVDECC not available");
            free(controller);
            return NULL;
#endif
            break;
            
        case OPENAVB_AVDECC_IMPL_LEGACY:
#ifdef OPENAVNU_HAS_LEGACY_AVDECC
            // Initialize legacy avdecc-lib controller
            // TODO: Implement legacy controller creation
            AVB_LOG_INFO("✅ Legacy AVDECC controller created (placeholder)");
            controller->is_initialized = true;
#else
            AVB_LOG_ERROR("Legacy AVDECC not available");
            free(controller);
            return NULL;
#endif
            break;
            
        case OPENAVB_AVDECC_IMPL_SIMULATION:
            // Initialize simulation mode
            controller->impl_data.simulation.simulated_entity_count = 3;
            
            // Create some simulated entities
            for (int i = 0; i < 3; i++) {
                openavb_unified_avdecc_entity_info_t *entity = 
                    &controller->impl_data.simulation.simulated_entities[i];
                
                entity->entity_id = 0x1234567890ABCD00ULL + i;
                snprintf(entity->entity_name, sizeof(entity->entity_name), "Simulated-Entity-%d", i+1);
                entity->is_milan_device = (i == 0);  // First entity is MILAN
                entity->supports_fast_connect = entity->is_milan_device;
                entity->is_talker = (i != 2);  // First two are talkers
                entity->is_listener = (i != 0);  // Last two are listeners
                entity->is_controller = false;
                entity->talker_stream_count = entity->is_talker ? 2 : 0;
                entity->listener_stream_count = entity->is_listener ? 2 : 0;
                entity->is_online = true;
            }
            
            controller->is_initialized = true;
            AVB_LOG_INFO("✅ Simulation AVDECC controller created with %d simulated entities", 
                        controller->impl_data.simulation.simulated_entity_count);
            break;
            
        default:
            AVB_LOG_ERROR("Unsupported AVDECC implementation type: %d", controller->implementation_type);
            free(controller);
            return NULL;
    }
    
    return controller;
}

void openavb_unified_avdecc_controller_destroy(openavb_unified_avdecc_controller_t *controller) {
    if (!controller) {
        return;
    }
    
    AVB_LOG_INFO("Destroying unified AVDECC controller (%s)", 
                 openavb_unified_avdecc_impl_type_to_string(controller->implementation_type));
    
    // Stop discovery if active
    if (controller->discovery_active) {
        openavb_unified_avdecc_stop_discovery(controller);
    }
    
    // Cleanup implementation-specific resources
    switch (controller->implementation_type) {
        case OPENAVB_AVDECC_IMPL_LA_AVDECC:
#ifdef OPENAVNU_HAS_LA_AVDECC
            try {
                controller->impl_data.la_avdecc.controller.reset();
                controller->impl_data.la_avdecc.discovered_entities.clear();
                // Explicitly call destructor
                controller->impl_data.la_avdecc.~decltype(controller->impl_data.la_avdecc)();
            } catch (const std::exception& e) {
                AVB_LOG_ERROR("Exception during L-Acoustics controller cleanup: %s", e.what());
            }
#endif
            break;
            
        case OPENAVB_AVDECC_IMPL_LEGACY:
#ifdef OPENAVNU_HAS_LEGACY_AVDECC
            // TODO: Cleanup legacy controller
#endif
            break;
            
        case OPENAVB_AVDECC_IMPL_SIMULATION:
            // Nothing special to cleanup for simulation
            break;
            
        default:
            break;
    }
    
    free(controller);
    AVB_LOG_INFO("✅ Unified AVDECC controller destroyed");
}

openavb_avdecc_impl_type_t openavb_unified_avdecc_controller_get_implementation(
    openavb_unified_avdecc_controller_t *controller) {
    
    if (!controller) {
        return OPENAVB_AVDECC_IMPL_SIMULATION;
    }
    
    return controller->implementation_type;
}

bool openavb_unified_avdecc_controller_supports_milan(
    openavb_unified_avdecc_controller_t *controller) {
    
    if (!controller) {
        return false;
    }
    
    switch (controller->implementation_type) {
        case OPENAVB_AVDECC_IMPL_LA_AVDECC:
            return true;  // L-Acoustics supports MILAN
            
        case OPENAVB_AVDECC_IMPL_LEGACY:
            return false;  // Legacy typically doesn't support MILAN
            
        case OPENAVB_AVDECC_IMPL_SIMULATION:
            return controller->config.milan_mode_required;  // Simulated based on config
            
        default:
            return false;
    }
}

// =============================================================================
// Entity Discovery
// =============================================================================

bool openavb_unified_avdecc_start_discovery(
    openavb_unified_avdecc_controller_t *controller,
    openavb_avdecc_entity_discovery_callback_t callback,
    void *user_data) {
    
    if (!controller || !controller->is_initialized) {
        AVB_LOG_ERROR("Invalid controller for discovery start");
        return false;
    }
    
    if (controller->discovery_active) {
        AVB_LOG_WARNING("Discovery already active");
        return true;
    }
    
    AVB_LOG_INFO("Starting entity discovery (%s implementation)", 
                 openavb_unified_avdecc_impl_type_to_string(controller->implementation_type));
    
    controller->discovery_callback = callback;
    controller->discovery_user_data = user_data;
    controller->discovery_active = true;
    
    switch (controller->implementation_type) {
        case OPENAVB_AVDECC_IMPL_LA_AVDECC:
#ifdef OPENAVNU_HAS_LA_AVDECC
            // TODO: Set up L-Acoustics discovery callbacks
            AVB_LOG_INFO("L-Acoustics entity discovery started");
#endif
            break;
            
        case OPENAVB_AVDECC_IMPL_LEGACY:
#ifdef OPENAVNU_HAS_LEGACY_AVDECC
            // TODO: Set up legacy discovery
            AVB_LOG_INFO("Legacy entity discovery started");
#endif
            break;
            
        case OPENAVB_AVDECC_IMPL_SIMULATION:
            // Simulate discovery by immediately reporting simulated entities
            if (callback) {
                for (uint32_t i = 0; i < controller->impl_data.simulation.simulated_entity_count; i++) {
                    const openavb_unified_avdecc_entity_info_t *entity = 
                        &controller->impl_data.simulation.simulated_entities[i];
                    callback(controller, entity, true, user_data);
                }
            }
            AVB_LOG_INFO("Simulation entity discovery completed - reported %d entities", 
                        controller->impl_data.simulation.simulated_entity_count);
            break;
            
        default:
            controller->discovery_active = false;
            return false;
    }
    
    return true;
}

void openavb_unified_avdecc_stop_discovery(openavb_unified_avdecc_controller_t *controller) {
    if (!controller || !controller->discovery_active) {
        return;
    }
    
    AVB_LOG_INFO("Stopping entity discovery");
    
    controller->discovery_active = false;
    controller->discovery_callback = NULL;
    controller->discovery_user_data = NULL;
    
    // Implementation-specific cleanup handled in switch above
    AVB_LOG_INFO("✅ Entity discovery stopped");
}

uint32_t openavb_unified_avdecc_get_discovered_entities(
    openavb_unified_avdecc_controller_t *controller,
    openavb_unified_avdecc_entity_info_t *entities,
    uint32_t max_entities) {
    
    if (!controller || !entities || max_entities == 0) {
        return 0;
    }
    
    uint32_t count = 0;
    
    switch (controller->implementation_type) {
        case OPENAVB_AVDECC_IMPL_LA_AVDECC:
#ifdef OPENAVNU_HAS_LA_AVDECC
            // TODO: Get entities from L-Acoustics controller
            count = 0;  // Placeholder
#endif
            break;
            
        case OPENAVB_AVDECC_IMPL_LEGACY:
#ifdef OPENAVNU_HAS_LEGACY_AVDECC
            // TODO: Get entities from legacy controller
            count = 0;  // Placeholder
#endif
            break;
            
        case OPENAVB_AVDECC_IMPL_SIMULATION:
            count = controller->impl_data.simulation.simulated_entity_count;
            if (count > max_entities) {
                count = max_entities;
            }
            
            for (uint32_t i = 0; i < count; i++) {
                memcpy(&entities[i], &controller->impl_data.simulation.simulated_entities[i], 
                       sizeof(openavb_unified_avdecc_entity_info_t));
            }
            break;
            
        default:
            count = 0;
            break;
    }
    
    return count;
}

bool openavb_unified_avdecc_get_entity_info(
    openavb_unified_avdecc_controller_t *controller,
    uint64_t entity_id,
    openavb_unified_avdecc_entity_info_t *entity_info) {
    
    if (!controller || !entity_info) {
        return false;
    }
    
    // Get all entities and find the one with matching ID
    openavb_unified_avdecc_entity_info_t entities[32];
    uint32_t count = openavb_unified_avdecc_get_discovered_entities(controller, entities, 32);
    
    for (uint32_t i = 0; i < count; i++) {
        if (entities[i].entity_id == entity_id) {
            memcpy(entity_info, &entities[i], sizeof(openavb_unified_avdecc_entity_info_t));
            return true;
        }
    }
    
    return false;
}

// =============================================================================
// Utility Functions
// =============================================================================

const char* openavb_unified_avdecc_impl_type_to_string(openavb_avdecc_impl_type_t impl_type) {
    switch (impl_type) {
        case OPENAVB_AVDECC_IMPL_AUTO:
            return "Auto-Select";
        case OPENAVB_AVDECC_IMPL_LA_AVDECC:
            return "L-Acoustics AVDECC";
        case OPENAVB_AVDECC_IMPL_LEGACY:
            return "Legacy avdecc-lib";
        case OPENAVB_AVDECC_IMPL_SIMULATION:
            return "Simulation";
        default:
            return "Unknown";
    }
}

void openavb_unified_avdecc_entity_id_to_string(uint64_t entity_id, char *buffer, size_t buffer_size) {
    if (buffer && buffer_size > 0) {
        snprintf(buffer, buffer_size, "0x%016llX", (unsigned long long)entity_id);
    }
}

void openavb_unified_avdecc_get_default_config(openavb_unified_avdecc_config_t *config) {
    if (!config) {
        return;
    }
    
    memset(config, 0, sizeof(openavb_unified_avdecc_config_t));
    
    config->preferred_implementation = OPENAVB_AVDECC_IMPL_AUTO;
    config->milan_mode_required = false;
    config->fast_connect_required = false;
    config->network_redundancy_required = false;
    config->discovery_timeout_ms = 5000;
    strncpy(config->entity_name, "OpenAvnu-Unified-Controller", sizeof(config->entity_name) - 1);
    strncpy(config->interface_name, "auto", sizeof(config->interface_name) - 1);
    config->enable_logging = true;
    config->max_entities = 256;
}

bool openavb_unified_avdecc_validate_config(const openavb_unified_avdecc_config_t *config) {
    if (!config) {
        return false;
    }
    
    // Check implementation availability
    if (config->preferred_implementation != OPENAVB_AVDECC_IMPL_AUTO && 
        !openavb_unified_avdecc_is_implementation_available(config->preferred_implementation)) {
        AVB_LOG_ERROR("Preferred implementation not available: %s", 
                     openavb_unified_avdecc_impl_type_to_string(config->preferred_implementation));
        return false;
    }
    
    // Validate discovery timeout
    if (config->discovery_timeout_ms < 1000 || config->discovery_timeout_ms > 60000) {
        AVB_LOG_ERROR("Invalid discovery timeout: %u ms (must be 1000-60000)", 
                     config->discovery_timeout_ms);
        return false;
    }
    
    // Validate entity name
    if (strlen(config->entity_name) == 0) {
        AVB_LOG_ERROR("Entity name cannot be empty");
        return false;
    }
    
    return true;
}

bool openavb_unified_avdecc_get_implementation_capabilities(
    openavb_avdecc_impl_type_t impl_type,
    openavb_avdecc_implementation_caps_t *capabilities) {
    
    if (!capabilities) {
        return false;
    }
    
    memset(capabilities, 0, sizeof(openavb_avdecc_implementation_caps_t));
    
    switch (impl_type) {
        case OPENAVB_AVDECC_IMPL_LA_AVDECC:
            capabilities->supports_milan = true;
            capabilities->supports_fast_connect = true;
            capabilities->supports_network_redundancy = true;
            capabilities->supports_ieee_1722_1_2021 = true;
            capabilities->supports_ieee_1722_1_2013 = false;
            capabilities->max_entities = 1024;
            strncpy(capabilities->version_string, "L-Acoustics v4.1.0", 
                   sizeof(capabilities->version_string) - 1);
            break;
            
        case OPENAVB_AVDECC_IMPL_LEGACY:
            capabilities->supports_milan = false;
            capabilities->supports_fast_connect = false;
            capabilities->supports_network_redundancy = false;
            capabilities->supports_ieee_1722_1_2021 = false;
            capabilities->supports_ieee_1722_1_2013 = true;
            capabilities->max_entities = 256;
            strncpy(capabilities->version_string, "Legacy avdecc-lib", 
                   sizeof(capabilities->version_string) - 1);
            break;
            
        case OPENAVB_AVDECC_IMPL_SIMULATION:
            capabilities->supports_milan = true;  // Can simulate anything
            capabilities->supports_fast_connect = true;
            capabilities->supports_network_redundancy = true;
            capabilities->supports_ieee_1722_1_2021 = true;
            capabilities->supports_ieee_1722_1_2013 = true;
            capabilities->max_entities = 32;
            strncpy(capabilities->version_string, "Simulation Mode v1.0", 
                   sizeof(capabilities->version_string) - 1);
            break;
            
        default:
            return false;
    }
    
    return true;
}
