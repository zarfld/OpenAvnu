/**
 * OpenAvnu Unified AVDECC API
 * 
 * This provides a unified interface for both L-Acoustics AVDECC and legacy avdecc-lib,
 * allowing applications to use either implementation transparently based on configuration.
 * 
 * Phase 3 Implementation Features:
 * - Unified API for both modern and legacy AVDECC
 * - Configuration-driven implementation selection
 * - Automatic fallback mechanisms
 * - Cross-platform compatibility
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>  // For size_t

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct openavb_unified_avdecc_controller openavb_unified_avdecc_controller_t;
typedef struct openavb_unified_avdecc_entity openavb_unified_avdecc_entity_t;

/**
 * AVDECC Implementation Types
 */
typedef enum {
    OPENAVB_AVDECC_IMPL_AUTO = 0,      // Auto-select best available
    OPENAVB_AVDECC_IMPL_LA_AVDECC,     // L-Acoustics AVDECC (modern)
    OPENAVB_AVDECC_IMPL_LEGACY,        // Legacy avdecc-lib
    OPENAVB_AVDECC_IMPL_SIMULATION     // Simulation mode (for testing)
} openavb_avdecc_impl_type_t;

/**
 * AVDECC Configuration
 */
typedef struct {
    openavb_avdecc_impl_type_t preferred_implementation;
    bool milan_mode_required;
    bool fast_connect_required;
    bool network_redundancy_required;
    uint32_t discovery_timeout_ms;
    char entity_name[64];
    char interface_name[32];
    bool enable_logging;
    uint32_t max_entities;
} openavb_unified_avdecc_config_t;

/**
 * Entity Information
 */
typedef struct {
    uint64_t entity_id;
    char entity_name[64];
    bool is_milan_device;
    bool supports_fast_connect;
    bool is_talker;
    bool is_listener;
    bool is_controller;
    uint32_t talker_stream_count;
    uint32_t listener_stream_count;
    bool is_online;
} openavb_unified_avdecc_entity_info_t;

/**
 * Discovery Callback Function Type
 */
typedef void (*openavb_avdecc_entity_discovery_callback_t)(
    openavb_unified_avdecc_controller_t *controller,
    const openavb_unified_avdecc_entity_info_t *entity_info,
    bool is_online,
    void *user_data
);

/**
 * Stream Connection Information
 */
typedef struct {
    uint64_t talker_entity_id;
    uint16_t talker_stream_index;
    uint64_t listener_entity_id;
    uint16_t listener_stream_index;
    bool is_connected;
    bool fast_connect_mode;
} openavb_avdecc_stream_connection_t;

// =============================================================================
// Unified AVDECC API Functions
// =============================================================================

/**
 * Initialize the unified AVDECC system
 */
bool openavb_unified_avdecc_initialize(void);

/**
 * Cleanup the unified AVDECC system
 */
void openavb_unified_avdecc_cleanup(void);

/**
 * Get available AVDECC implementations
 */
uint32_t openavb_unified_avdecc_get_available_implementations(
    openavb_avdecc_impl_type_t *implementations,
    uint32_t max_implementations
);

/**
 * Check if a specific implementation is available
 */
bool openavb_unified_avdecc_is_implementation_available(openavb_avdecc_impl_type_t impl_type);

/**
 * Auto-select best AVDECC implementation based on requirements
 */
openavb_avdecc_impl_type_t openavb_unified_avdecc_auto_select_implementation(
    const openavb_unified_avdecc_config_t *config
);

// =============================================================================
// Controller Management
// =============================================================================

/**
 * Create unified AVDECC controller
 */
openavb_unified_avdecc_controller_t* openavb_unified_avdecc_controller_create(
    const openavb_unified_avdecc_config_t *config
);

/**
 * Destroy unified AVDECC controller
 */
void openavb_unified_avdecc_controller_destroy(openavb_unified_avdecc_controller_t *controller);

/**
 * Get the actual implementation type being used
 */
openavb_avdecc_impl_type_t openavb_unified_avdecc_controller_get_implementation(
    openavb_unified_avdecc_controller_t *controller
);

/**
 * Check if controller supports MILAN features
 */
bool openavb_unified_avdecc_controller_supports_milan(
    openavb_unified_avdecc_controller_t *controller
);

// =============================================================================
// Entity Discovery
// =============================================================================

/**
 * Start entity discovery
 */
bool openavb_unified_avdecc_start_discovery(
    openavb_unified_avdecc_controller_t *controller,
    openavb_avdecc_entity_discovery_callback_t callback,
    void *user_data
);

/**
 * Stop entity discovery
 */
void openavb_unified_avdecc_stop_discovery(openavb_unified_avdecc_controller_t *controller);

/**
 * Get discovered entities
 */
uint32_t openavb_unified_avdecc_get_discovered_entities(
    openavb_unified_avdecc_controller_t *controller,
    openavb_unified_avdecc_entity_info_t *entities,
    uint32_t max_entities
);

/**
 * Get entity by ID
 */
bool openavb_unified_avdecc_get_entity_info(
    openavb_unified_avdecc_controller_t *controller,
    uint64_t entity_id,
    openavb_unified_avdecc_entity_info_t *entity_info
);

// =============================================================================
// Stream Management
// =============================================================================

/**
 * Connect stream (talker to listener)
 */
bool openavb_unified_avdecc_connect_stream(
    openavb_unified_avdecc_controller_t *controller,
    uint64_t talker_entity_id,
    uint16_t talker_stream_index,
    uint64_t listener_entity_id,
    uint16_t listener_stream_index,
    bool use_fast_connect
);

/**
 * Disconnect stream
 */
bool openavb_unified_avdecc_disconnect_stream(
    openavb_unified_avdecc_controller_t *controller,
    uint64_t talker_entity_id,
    uint16_t talker_stream_index,
    uint64_t listener_entity_id,
    uint16_t listener_stream_index
);

/**
 * Get stream connections
 */
uint32_t openavb_unified_avdecc_get_stream_connections(
    openavb_unified_avdecc_controller_t *controller,
    openavb_avdecc_stream_connection_t *connections,
    uint32_t max_connections
);

// =============================================================================
// Configuration and Status
// =============================================================================

/**
 * Load configuration from file
 */
bool openavb_unified_avdecc_load_config(
    openavb_unified_avdecc_config_t *config,
    const char *config_file_path
);

/**
 * Save configuration to file
 */
bool openavb_unified_avdecc_save_config(
    const openavb_unified_avdecc_config_t *config,
    const char *config_file_path
);

/**
 * Get default configuration
 */
void openavb_unified_avdecc_get_default_config(openavb_unified_avdecc_config_t *config);

/**
 * Validate configuration
 */
bool openavb_unified_avdecc_validate_config(const openavb_unified_avdecc_config_t *config);

/**
 * Get system status and capabilities
 */
typedef struct {
    openavb_avdecc_impl_type_t active_implementation;
    bool milan_mode_active;
    bool fast_connect_available;
    bool network_redundancy_available;
    uint32_t discovered_entities_count;
    uint32_t active_stream_connections;
    bool is_discovery_active;
    char implementation_version[32];
} openavb_unified_avdecc_status_t;

bool openavb_unified_avdecc_get_status(
    openavb_unified_avdecc_controller_t *controller,
    openavb_unified_avdecc_status_t *status
);

// =============================================================================
// Utility Functions
// =============================================================================

/**
 * Convert implementation type to string
 */
const char* openavb_unified_avdecc_impl_type_to_string(openavb_avdecc_impl_type_t impl_type);

/**
 * Convert entity ID to string
 */
void openavb_unified_avdecc_entity_id_to_string(uint64_t entity_id, char *buffer, size_t buffer_size);

/**
 * Check MILAN compliance of entity
 */
bool openavb_unified_avdecc_is_milan_entity(
    openavb_unified_avdecc_controller_t *controller,
    uint64_t entity_id
);

/**
 * Get implementation capabilities
 */
typedef struct {
    bool supports_milan;
    bool supports_fast_connect;
    bool supports_network_redundancy;
    bool supports_ieee_1722_1_2021;
    bool supports_ieee_1722_1_2013;
    uint32_t max_entities;
    char version_string[64];
} openavb_avdecc_implementation_caps_t;

bool openavb_unified_avdecc_get_implementation_capabilities(
    openavb_avdecc_impl_type_t impl_type,
    openavb_avdecc_implementation_caps_t *capabilities
);

#ifdef __cplusplus
}
#endif
