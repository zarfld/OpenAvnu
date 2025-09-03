/**
 * @file network_hal.c
 * @brief Generic Network HAL Core Implementation  
 * 
 * Core implementation of vendor-agnostic network hardware abstraction.
 * Provides device discovery, vendor adapter registration, and resource management.
 * 
 * @author OpenAvnu Development Team
 * @date September 3, 2025
 * @version 1.0.0
 * 
 * Architecture Notes:
 * - Vendor adapters register function pointers for device operations
 * - Intel adapter automatically registered when intel_avb library available
 * - Thread-safe initialization and cleanup with reference counting
 * - Graceful degradation when hardware features unavailable
 * 
 * @copyright Copyright (c) 2025, The OpenAvnu Contributors
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "network_hal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#ifdef _WIN32
#include <windows.h>
#include <winsock2.h>
#include <iphlpapi.h>
#else
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <net/if.h>
#endif

/* ============================================================================
 * INTERNAL DATA STRUCTURES
 * ============================================================================ */

/**
 * @brief Forward declaration of vendor adapter interface
 */
typedef struct network_hal_vendor_adapter network_hal_vendor_adapter_t;

/**
 * @brief Generic network device implementation
 * 
 * Contains vendor-specific context and function pointers for device operations.
 * Opaque to external users, accessed only through network_hal_device_t handle.
 */
struct network_hal_device {
    /** Device information from enumeration */
    network_hal_device_info_t device_info;
    
    /** Vendor adapter providing this device */
    const network_hal_vendor_adapter_t *vendor_adapter;
    
    /** Vendor-specific device context (opaque) */
    void *vendor_context;
    
    /** Reference count for cleanup tracking */
    int ref_count;
    
    /** Device initialization state */
    bool is_initialized;
};

/**
 * @brief Vendor adapter operation function pointers
 * 
 * Each vendor adapter implements these functions for hardware-specific operations.
 * Generic HAL delegates to appropriate vendor adapter based on device type.
 */
typedef struct network_hal_vendor_operations {
    /** Initialize vendor adapter (called once during HAL init) */
    network_hal_result_t (*init)(void);
    
    /** Cleanup vendor adapter (called once during HAL cleanup) */
    network_hal_result_t (*cleanup)(void);
    
    /** Enumerate devices provided by this vendor */
    network_hal_result_t (*enumerate_devices)(
        network_hal_device_info_t *device_list,
        uint32_t max_devices,
        uint32_t *device_count
    );
    
    /** Open/attach to vendor-specific device */
    network_hal_result_t (*device_open)(
        const network_hal_device_info_t *device_info,
        void **vendor_context
    );
    
    /** Close/detach from vendor-specific device */
    network_hal_result_t (*device_close)(void *vendor_context);
    
    /** Get hardware timestamp */
    network_hal_result_t (*get_time)(
        void *vendor_context,
        network_hal_timestamp_t *timestamp
    );
    
    /** Set hardware timestamp */
    network_hal_result_t (*set_time)(
        void *vendor_context,
        const network_hal_timestamp_t *timestamp
    );
    
    /** Adjust hardware frequency */
    network_hal_result_t (*adjust_frequency)(
        void *vendor_context,
        int32_t frequency_ppb
    );
    
    /** Configure Time-Aware Shaper */
    network_hal_result_t (*configure_time_aware_shaper)(
        void *vendor_context,
        const void *config
    );
    
    /** Configure Frame Preemption */
    network_hal_result_t (*configure_frame_preemption)(
        void *vendor_context,
        const void *config
    );
    
    /** Check device capability */
    bool (*has_capability)(
        void *vendor_context,
        network_hal_capability_t capability
    );
    
    /** Get current device information */
    network_hal_result_t (*get_device_info)(
        void *vendor_context,
        network_hal_device_info_t *device_info
    );
} network_hal_vendor_operations_t;

/**
 * @brief Vendor adapter registration structure
 * 
 * Each vendor adapter registers itself with these details during HAL initialization.
 */
struct network_hal_vendor_adapter {
    /** Vendor identification */
    network_hal_vendor_t vendor_type;
    
    /** Human-readable vendor name */
    const char *vendor_name;
    
    /** Vendor adapter version */
    uint32_t version;
    
    /** Vendor-specific operations */
    network_hal_vendor_operations_t operations;
    
    /** Adapter initialization state */
    bool is_initialized;
    
    /** Next adapter in registration list */
    struct network_hal_vendor_adapter *next;
};

/**
 * @brief Global HAL state
 * 
 * Maintains HAL initialization state and registered vendor adapters.
 * Protected by mutex for thread-safe operation.
 */
typedef struct {
    /** HAL initialization reference count */
    int init_ref_count;
    
    /** HAL initialization state */
    bool is_initialized;
    
    /** Registered vendor adapters (linked list) */
    network_hal_vendor_adapter_t *vendor_adapters;
    
    /** Enumerated devices cache */
    network_hal_device_info_t device_cache[NETWORK_HAL_MAX_DEVICES];
    uint32_t cached_device_count;
    bool device_cache_valid;
    
#ifdef _WIN32
    /** Windows critical section for thread safety */
    CRITICAL_SECTION hal_lock;
#else
    /** POSIX mutex for thread safety */
    pthread_mutex_t hal_lock;
#endif
} network_hal_global_state_t;

/* ============================================================================
 * GLOBAL VARIABLES
 * ============================================================================ */

/**
 * @brief Global HAL state instance
 * 
 * Single global instance maintaining HAL state across all operations.
 * Protected by platform-specific synchronization primitives.
 */
static network_hal_global_state_t g_hal_state = {
    .init_ref_count = 0,
    .is_initialized = false,
    .vendor_adapters = NULL,
    .cached_device_count = 0,
    .device_cache_valid = false
};

/* ============================================================================
 * PLATFORM-SPECIFIC SYNCHRONIZATION
 * ============================================================================ */

/**
 * @brief Initialize HAL synchronization primitives
 * 
 * Creates platform-specific mutex/critical section for thread-safe operation.
 * Called once during first HAL initialization.
 * 
 * @return NETWORK_HAL_SUCCESS on success, error code on failure
 */
static network_hal_result_t hal_sync_init(void)
{
#ifdef _WIN32
    InitializeCriticalSection(&g_hal_state.hal_lock);
    return NETWORK_HAL_SUCCESS;
#else
    int result = pthread_mutex_init(&g_hal_state.hal_lock, NULL);
    return (result == 0) ? NETWORK_HAL_SUCCESS : NETWORK_HAL_ERROR_GENERAL;
#endif
}

/**
 * @brief Cleanup HAL synchronization primitives
 * 
 * Destroys platform-specific mutex/critical section.
 * Called once during final HAL cleanup.
 */
static void hal_sync_cleanup(void)
{
#ifdef _WIN32
    DeleteCriticalSection(&g_hal_state.hal_lock);
#else
    pthread_mutex_destroy(&g_hal_state.hal_lock);
#endif
}

/**
 * @brief Acquire HAL lock for thread-safe operation
 * 
 * Must be called before accessing global HAL state.
 * Paired with hal_lock_release().
 */
static void hal_lock_acquire(void)
{
#ifdef _WIN32
    EnterCriticalSection(&g_hal_state.hal_lock);
#else
    pthread_mutex_lock(&g_hal_state.hal_lock);
#endif
}

/**
 * @brief Release HAL lock after thread-safe operation
 * 
 * Must be called after hal_lock_acquire() and state manipulation.
 */
static void hal_lock_release(void)
{
#ifdef _WIN32
    LeaveCriticalSection(&g_hal_state.hal_lock);
#else
    pthread_mutex_unlock(&g_hal_state.hal_lock);
#endif
}

/* ============================================================================
 * VENDOR ADAPTER REGISTRATION
 * ============================================================================ */

/**
 * @brief Register vendor adapter with HAL
 * 
 * Adds vendor adapter to global registration list.
 * Called by vendor-specific initialization code.
 * 
 * @param[in] adapter           Vendor adapter to register
 * 
 * @return NETWORK_HAL_SUCCESS on success, error code on failure
 * 
 * Implementation Notes:
 * - Validates adapter structure completeness
 * - Prevents duplicate vendor registrations  
 * - Thread-safe operation with HAL lock
 */
static network_hal_result_t hal_register_vendor_adapter(
    network_hal_vendor_adapter_t *adapter
)
{
    if (!adapter || !adapter->vendor_name) {
        return NETWORK_HAL_ERROR_INVALID_PARAM;
    }
    
    hal_lock_acquire();
    
    /* Check for duplicate vendor registration */
    network_hal_vendor_adapter_t *current = g_hal_state.vendor_adapters;
    while (current) {
        if (current->vendor_type == adapter->vendor_type) {
            hal_lock_release();
            return NETWORK_HAL_ERROR_GENERAL;  /* Already registered */
        }
        current = current->next;
    }
    
    /* Add to head of vendor adapter list */
    adapter->next = g_hal_state.vendor_adapters;
    g_hal_state.vendor_adapters = adapter;
    
    hal_lock_release();
    return NETWORK_HAL_SUCCESS;
}

/**
 * @brief Find registered vendor adapter by type
 * 
 * Locates vendor adapter in registration list by vendor type.
 * Used for device-to-adapter mapping during operations.
 * 
 * @param[in] vendor_type       Vendor type to locate
 * 
 * @return Pointer to vendor adapter, or NULL if not found
 * 
 * Implementation Notes:
 * - Thread-safe operation (read-only access)
 * - Returns NULL for unknown/unregistered vendors
 */
static network_hal_vendor_adapter_t* hal_find_vendor_adapter(
    network_hal_vendor_t vendor_type
)
{
    network_hal_vendor_adapter_t *current = g_hal_state.vendor_adapters;
    
    while (current) {
        if (current->vendor_type == vendor_type) {
            return current;
        }
        current = current->next;
    }
    
    return NULL;
}

/* ============================================================================
 * INTEL ADAPTER INTEGRATION
 * ============================================================================ */

/* Forward declarations for Intel adapter */
static network_hal_result_t intel_adapter_init(void);
static network_hal_result_t intel_adapter_cleanup(void);
static network_hal_result_t intel_adapter_enumerate_devices(
    network_hal_device_info_t *device_list,
    uint32_t max_devices,
    uint32_t *device_count
);

/**
 * @brief Intel vendor adapter registration
 * 
 * Intel adapter implementation using intel_avb library.
 * Registered automatically when intel_avb library is available.
 */
static network_hal_vendor_adapter_t g_intel_adapter = {
    .vendor_type = NETWORK_HAL_VENDOR_INTEL,
    .vendor_name = "Intel Corporation",
    .version = 0x00010000,  /* Version 1.0.0 */
    .operations = {
        .init = intel_adapter_init,
        .cleanup = intel_adapter_cleanup,
        .enumerate_devices = intel_adapter_enumerate_devices,
        /* Additional operations initialized in intel_adapter_init() */
    },
    .is_initialized = false,
    .next = NULL
};

/* ============================================================================
 * CORE HAL API IMPLEMENTATION
 * ============================================================================ */

/**
 * @brief Initialize the Generic Network HAL
 * 
 * Initializes HAL framework, synchronization primitives, and vendor adapters.
 * Reference counted for multiple init/cleanup cycles.
 * 
 * @return NETWORK_HAL_SUCCESS on success, error code on failure
 */
network_hal_result_t network_hal_init(void)
{
    network_hal_result_t result = NETWORK_HAL_SUCCESS;
    
    /* First-time initialization */
    if (g_hal_state.init_ref_count == 0) {
        /* Initialize synchronization primitives */
        result = hal_sync_init();
        if (result != NETWORK_HAL_SUCCESS) {
            return result;
        }
        
        hal_lock_acquire();
        
        /* Initialize global state */
        g_hal_state.vendor_adapters = NULL;
        g_hal_state.device_cache_valid = false;
        g_hal_state.cached_device_count = 0;
        
        /* Register Intel adapter (if intel_avb available) */
        result = hal_register_vendor_adapter(&g_intel_adapter);
        if (result != NETWORK_HAL_SUCCESS) {
            printf("[NETWORK_HAL] Warning: Intel adapter registration failed: %s\n",
                   network_hal_result_string(result));
            /* Continue without Intel adapter (graceful degradation) */
            result = NETWORK_HAL_SUCCESS;
        }
        
        /* Initialize all registered vendor adapters */
        network_hal_vendor_adapter_t *current = g_hal_state.vendor_adapters;
        while (current && result == NETWORK_HAL_SUCCESS) {
            if (current->operations.init) {
                result = current->operations.init();
                if (result == NETWORK_HAL_SUCCESS) {
                    current->is_initialized = true;
                    printf("[NETWORK_HAL] Initialized %s adapter\n", current->vendor_name);
                } else {
                    printf("[NETWORK_HAL] Warning: Failed to initialize %s adapter: %s\n",
                           current->vendor_name, network_hal_result_string(result));
                    /* Continue with other adapters (graceful degradation) */
                    result = NETWORK_HAL_SUCCESS;
                }
            }
            current = current->next;
        }
        
        g_hal_state.is_initialized = true;
        hal_lock_release();
    } else {
        hal_lock_acquire();
        /* Already initialized, just increment reference count */
        hal_lock_release();
    }
    
    if (result == NETWORK_HAL_SUCCESS) {
        hal_lock_acquire();
        g_hal_state.init_ref_count++;
        hal_lock_release();
        printf("[NETWORK_HAL] HAL initialized successfully (ref_count: %d)\n", 
               g_hal_state.init_ref_count);
    }
    
    return result;
}

/**
 * @brief Cleanup the Generic Network HAL
 * 
 * Releases HAL resources and cleans up vendor adapters.
 * Reference counted to match init calls.
 * 
 * @return NETWORK_HAL_SUCCESS on success, error code on failure
 */
network_hal_result_t network_hal_cleanup(void)
{
    if (!g_hal_state.is_initialized || g_hal_state.init_ref_count <= 0) {
        return NETWORK_HAL_SUCCESS;  /* Nothing to cleanup */
    }
    
    hal_lock_acquire();
    g_hal_state.init_ref_count--;
    
    /* Final cleanup when reference count reaches zero */
    if (g_hal_state.init_ref_count == 0) {
        /* Cleanup all vendor adapters */
        network_hal_vendor_adapter_t *current = g_hal_state.vendor_adapters;
        while (current) {
            if (current->is_initialized && current->operations.cleanup) {
                network_hal_result_t result = current->operations.cleanup();
                if (result != NETWORK_HAL_SUCCESS) {
                    printf("[NETWORK_HAL] Warning: %s adapter cleanup failed: %s\n",
                           current->vendor_name, network_hal_result_string(result));
                }
                current->is_initialized = false;
            }
            current = current->next;
        }
        
        /* Clear global state */
        g_hal_state.vendor_adapters = NULL;
        g_hal_state.device_cache_valid = false;
        g_hal_state.cached_device_count = 0;
        g_hal_state.is_initialized = false;
        
        printf("[NETWORK_HAL] HAL cleanup completed\n");
    }
    
    hal_lock_release();
    
    /* Cleanup synchronization primitives on final cleanup */
    if (g_hal_state.init_ref_count == 0) {
        hal_sync_cleanup();
    }
    
    return NETWORK_HAL_SUCCESS;
}

/**
 * @brief Enumerate available network devices
 * 
 * Discovers devices from all registered vendor adapters.
 * Results cached for performance with explicit refresh support.
 * 
 * @param[out] device_list      Array to receive device information
 * @param[in]  max_devices      Maximum devices to enumerate  
 * @param[out] device_count     Actual number of devices found
 * 
 * @return NETWORK_HAL_SUCCESS on success, error code on failure
 */
network_hal_result_t network_hal_enumerate_devices(
    network_hal_device_info_t *device_list,
    uint32_t max_devices,
    uint32_t *device_count
)
{
    if (!device_list || !device_count || max_devices == 0) {
        return NETWORK_HAL_ERROR_INVALID_PARAM;
    }
    
    if (!g_hal_state.is_initialized) {
        return NETWORK_HAL_ERROR_GENERAL;
    }
    
    hal_lock_acquire();
    
    /* Use cached results if available and valid */
    if (g_hal_state.device_cache_valid) {
        uint32_t copy_count = (g_hal_state.cached_device_count < max_devices) ?
                              g_hal_state.cached_device_count : max_devices;
        
        memcpy(device_list, g_hal_state.device_cache, 
               copy_count * sizeof(network_hal_device_info_t));
        *device_count = copy_count;
        
        hal_lock_release();
        return NETWORK_HAL_SUCCESS;
    }
    
    /* Enumerate devices from all vendor adapters */
    uint32_t total_devices = 0;
    network_hal_result_t result = NETWORK_HAL_SUCCESS;
    
    network_hal_vendor_adapter_t *current = g_hal_state.vendor_adapters;
    while (current && total_devices < max_devices && result == NETWORK_HAL_SUCCESS) {
        if (current->is_initialized && current->operations.enumerate_devices) {
            uint32_t vendor_device_count = 0;
            
            result = current->operations.enumerate_devices(
                &device_list[total_devices],
                max_devices - total_devices,
                &vendor_device_count
            );
            
            if (result == NETWORK_HAL_SUCCESS) {
                total_devices += vendor_device_count;
                printf("[NETWORK_HAL] %s adapter: found %u devices\n", 
                       current->vendor_name, vendor_device_count);
            } else {
                printf("[NETWORK_HAL] Warning: %s device enumeration failed: %s\n",
                       current->vendor_name, network_hal_result_string(result));
                /* Continue with other adapters (graceful degradation) */
                result = NETWORK_HAL_SUCCESS;
            }
        }
        current = current->next;
    }
    
    *device_count = total_devices;
    
    /* Cache results for future use */
    if (result == NETWORK_HAL_SUCCESS && total_devices <= NETWORK_HAL_MAX_DEVICES) {
        memcpy(g_hal_state.device_cache, device_list,
               total_devices * sizeof(network_hal_device_info_t));
        g_hal_state.cached_device_count = total_devices;
        g_hal_state.device_cache_valid = true;
    }
    
    hal_lock_release();
    
    printf("[NETWORK_HAL] Device enumeration completed: %u devices found\n", total_devices);
    return result;
}

/**
 * @brief Open and attach to specific network device
 * 
 * Creates device handle and establishes vendor-specific connection.
 * Device must have been previously enumerated.
 * 
 * @param[in]  device_info      Device information from enumeration
 * @param[out] device_handle    Handle for subsequent operations
 * 
 * @return NETWORK_HAL_SUCCESS on success, error code on failure
 */
network_hal_result_t network_hal_device_open(
    const network_hal_device_info_t *device_info,
    network_hal_device_t **device_handle
)
{
    if (!device_info || !device_handle) {
        return NETWORK_HAL_ERROR_INVALID_PARAM;
    }
    
    if (!g_hal_state.is_initialized) {
        return NETWORK_HAL_ERROR_GENERAL;
    }
    
    /* Find appropriate vendor adapter */
    network_hal_vendor_adapter_t *adapter = hal_find_vendor_adapter(device_info->vendor);
    if (!adapter || !adapter->is_initialized || !adapter->operations.device_open) {
        return NETWORK_HAL_ERROR_NOT_SUPPORTED;
    }
    
    /* Allocate device handle */
    network_hal_device_t *device = (network_hal_device_t*)malloc(sizeof(network_hal_device_t));
    if (!device) {
        return NETWORK_HAL_ERROR_NO_MEMORY;
    }
    
    /* Initialize device handle */
    memcpy(&device->device_info, device_info, sizeof(network_hal_device_info_t));
    device->vendor_adapter = adapter;
    device->vendor_context = NULL;
    device->ref_count = 1;
    device->is_initialized = false;
    
    /* Open vendor-specific device */
    network_hal_result_t result = adapter->operations.device_open(
        device_info, &device->vendor_context
    );
    
    if (result == NETWORK_HAL_SUCCESS) {
        device->is_initialized = true;
        *device_handle = device;
        printf("[NETWORK_HAL] Opened device: %s (%s)\n", 
               device_info->name, adapter->vendor_name);
    } else {
        free(device);
        *device_handle = NULL;
        printf("[NETWORK_HAL] Failed to open device %s: %s\n", 
               device_info->name, network_hal_result_string(result));
    }
    
    return result;
}

/**
 * @brief Close and detach from network device
 * 
 * Releases device resources and invalidates device handle.
 * Safe to call with NULL handle (no-op).
 * 
 * @param[in] device_handle     Device handle from open operation
 * 
 * @return NETWORK_HAL_SUCCESS on success, error code on failure
 */
network_hal_result_t network_hal_device_close(
    network_hal_device_t *device_handle
)
{
    if (!device_handle) {
        return NETWORK_HAL_SUCCESS;  /* No-op for NULL handle */
    }
    
    network_hal_result_t result = NETWORK_HAL_SUCCESS;
    
    /* Close vendor-specific device */
    if (device_handle->is_initialized && 
        device_handle->vendor_adapter->operations.device_close) {
        
        result = device_handle->vendor_adapter->operations.device_close(
            device_handle->vendor_context
        );
        
        if (result != NETWORK_HAL_SUCCESS) {
            printf("[NETWORK_HAL] Warning: Device close failed: %s\n",
                   network_hal_result_string(result));
        }
    }
    
    printf("[NETWORK_HAL] Closed device: %s\n", device_handle->device_info.name);
    
    /* Release device handle */
    free(device_handle);
    return result;
}

/* ============================================================================
 * DEVICE OPERATION IMPLEMENTATIONS
 * ============================================================================ */

/**
 * @brief Get current hardware time
 * 
 * Delegates to vendor-specific timestamping implementation.
 * Critical path for gPTP synchronization.
 * 
 * @param[in]  device_handle    Device handle from open operation
 * @param[out] timestamp        Hardware timestamp structure
 * 
 * @return NETWORK_HAL_SUCCESS on success, error code on failure
 */
network_hal_result_t network_hal_get_time(
    network_hal_device_t *device_handle,
    network_hal_timestamp_t *timestamp
)
{
    if (!device_handle || !timestamp) {
        return NETWORK_HAL_ERROR_INVALID_PARAM;
    }
    
    if (!device_handle->is_initialized ||
        !device_handle->vendor_adapter->operations.get_time) {
        return NETWORK_HAL_ERROR_NOT_SUPPORTED;
    }
    
    return device_handle->vendor_adapter->operations.get_time(
        device_handle->vendor_context, timestamp
    );
}

/**
 * @brief Set hardware time
 * 
 * Delegates to vendor-specific time setting implementation.
 * Used for gPTP synchronization initialization.
 * 
 * @param[in] device_handle     Device handle from open operation
 * @param[in] timestamp         Target hardware timestamp
 * 
 * @return NETWORK_HAL_SUCCESS on success, error code on failure
 */
network_hal_result_t network_hal_set_time(
    network_hal_device_t *device_handle,
    const network_hal_timestamp_t *timestamp
)
{
    if (!device_handle || !timestamp) {
        return NETWORK_HAL_ERROR_INVALID_PARAM;
    }
    
    if (!device_handle->is_initialized ||
        !device_handle->vendor_adapter->operations.set_time) {
        return NETWORK_HAL_ERROR_NOT_SUPPORTED;
    }
    
    return device_handle->vendor_adapter->operations.set_time(
        device_handle->vendor_context, timestamp
    );
}

/**
 * @brief Adjust hardware time frequency
 * 
 * Delegates to vendor-specific frequency adjustment implementation.
 * Primary mechanism for gPTP clock discipline.
 * 
 * @param[in] device_handle     Device handle from open operation
 * @param[in] frequency_ppb     Frequency adjustment in parts per billion
 * 
 * @return NETWORK_HAL_SUCCESS on success, error code on failure
 */
network_hal_result_t network_hal_adjust_frequency(
    network_hal_device_t *device_handle,
    int32_t frequency_ppb
)
{
    if (!device_handle) {
        return NETWORK_HAL_ERROR_INVALID_PARAM;
    }
    
    if (!device_handle->is_initialized ||
        !device_handle->vendor_adapter->operations.adjust_frequency) {
        return NETWORK_HAL_ERROR_NOT_SUPPORTED;
    }
    
    return device_handle->vendor_adapter->operations.adjust_frequency(
        device_handle->vendor_context, frequency_ppb
    );
}

/**
 * @brief Configure Time-Aware Shaper (IEEE 802.1Qbv)
 * 
 * Delegates to vendor-specific TSN configuration.
 * Available only on Intel I225/I226 or equivalent hardware.
 * 
 * @param[in] device_handle     Device handle from open operation
 * @param[in] config           Time-aware shaper configuration
 * 
 * @return NETWORK_HAL_SUCCESS on success, error code on failure
 */
network_hal_result_t network_hal_configure_time_aware_shaper(
    network_hal_device_t *device_handle,
    const void *config
)
{
    if (!device_handle || !config) {
        return NETWORK_HAL_ERROR_INVALID_PARAM;
    }
    
    if (!device_handle->is_initialized ||
        !device_handle->vendor_adapter->operations.configure_time_aware_shaper) {
        return NETWORK_HAL_ERROR_NOT_SUPPORTED;
    }
    
    return device_handle->vendor_adapter->operations.configure_time_aware_shaper(
        device_handle->vendor_context, config
    );
}

/**
 * @brief Configure Frame Preemption (IEEE 802.1Qbu)
 * 
 * Delegates to vendor-specific TSN configuration.
 * Available only on Intel I225/I226 or equivalent hardware.
 * 
 * @param[in] device_handle     Device handle from open operation
 * @param[in] config           Frame preemption configuration
 * 
 * @return NETWORK_HAL_SUCCESS on success, error code on failure
 */
network_hal_result_t network_hal_configure_frame_preemption(
    network_hal_device_t *device_handle,
    const void *config
)
{
    if (!device_handle || !config) {
        return NETWORK_HAL_ERROR_INVALID_PARAM;
    }
    
    if (!device_handle->is_initialized ||
        !device_handle->vendor_adapter->operations.configure_frame_preemption) {
        return NETWORK_HAL_ERROR_NOT_SUPPORTED;
    }
    
    return device_handle->vendor_adapter->operations.configure_frame_preemption(
        device_handle->vendor_context, config
    );
}

/* ============================================================================
 * UTILITY FUNCTION IMPLEMENTATIONS
 * ============================================================================ */

/**
 * @brief Check device capability
 * 
 * Tests capability flags from device enumeration.
 * Fast operation using cached capability information.
 * 
 * @param[in] device_handle     Device handle from open operation
 * @param[in] capability        Capability flag to test
 * 
 * @return true if capability supported, false otherwise
 */
bool network_hal_has_capability(
    network_hal_device_t *device_handle,
    network_hal_capability_t capability
)
{
    if (!device_handle || !device_handle->is_initialized) {
        return false;
    }
    
    return (device_handle->device_info.capabilities & capability) != 0;
}

/**
 * @brief Get device information
 * 
 * Retrieves current device information and status.
 * May update dynamic information via vendor adapter.
 * 
 * @param[in]  device_handle    Device handle from open operation
 * @param[out] device_info      Current device information
 * 
 * @return NETWORK_HAL_SUCCESS on success, error code on failure
 */
network_hal_result_t network_hal_get_device_info(
    network_hal_device_t *device_handle,
    network_hal_device_info_t *device_info
)
{
    if (!device_handle || !device_info) {
        return NETWORK_HAL_ERROR_INVALID_PARAM;
    }
    
    if (!device_handle->is_initialized) {
        return NETWORK_HAL_ERROR_DEVICE_NOT_FOUND;
    }
    
    /* Get updated device information from vendor adapter if available */
    if (device_handle->vendor_adapter->operations.get_device_info) {
        return device_handle->vendor_adapter->operations.get_device_info(
            device_handle->vendor_context, device_info
        );
    }
    
    /* Return cached device information */
    memcpy(device_info, &device_handle->device_info, sizeof(network_hal_device_info_t));
    return NETWORK_HAL_SUCCESS;
}

/**
 * @brief Convert result code to human-readable string
 * 
 * Provides error descriptions for debugging and logging.
 * Returns constant strings for thread safety.
 * 
 * @param[in] result            Network HAL result code
 * 
 * @return Constant string describing the result code
 */
const char* network_hal_result_string(network_hal_result_t result)
{
    switch (result) {
        case NETWORK_HAL_SUCCESS:
            return "Success";
        case NETWORK_HAL_ERROR_GENERAL:
            return "General failure";
        case NETWORK_HAL_ERROR_INVALID_PARAM:
            return "Invalid parameter";
        case NETWORK_HAL_ERROR_DEVICE_NOT_FOUND:
            return "Device not found or not available";
        case NETWORK_HAL_ERROR_NOT_SUPPORTED:
            return "Feature not supported on this hardware";
        case NETWORK_HAL_ERROR_DEVICE_BUSY:
            return "Device busy or resource conflict";
        case NETWORK_HAL_ERROR_NO_MEMORY:
            return "Insufficient memory or resources";
        case NETWORK_HAL_ERROR_TIMEOUT:
            return "Hardware access timeout";
        case NETWORK_HAL_ERROR_IO:
            return "Hardware I/O error";
        default:
            return "Unknown error";
    }
}

/* ============================================================================
 * INTEL ADAPTER STUB IMPLEMENTATIONS
 * ============================================================================ */

/**
 * @brief Intel adapter initialization stub
 * 
 * Placeholder implementation for Intel adapter initialization.
 * Will be replaced with real intel_avb integration in Phase 2.
 * 
 * @return NETWORK_HAL_SUCCESS on success, error code on failure
 */
static network_hal_result_t intel_adapter_init(void)
{
    printf("[INTEL_ADAPTER] Initializing Intel vendor adapter...\n");
    
    /* TODO: Initialize intel_avb library
     * - Call intel_probe() for each potential device
     * - Build Intel device list for enumeration
     * - Initialize Intel adapter function pointers
     */
    
    printf("[INTEL_ADAPTER] Intel adapter initialization completed (stub)\n");
    return NETWORK_HAL_SUCCESS;
}

/**
 * @brief Intel adapter cleanup stub
 * 
 * Placeholder implementation for Intel adapter cleanup.
 * Will be replaced with real intel_avb integration in Phase 2.
 * 
 * @return NETWORK_HAL_SUCCESS on success, error code on failure
 */
static network_hal_result_t intel_adapter_cleanup(void)
{
    printf("[INTEL_ADAPTER] Cleaning up Intel vendor adapter...\n");
    
    /* TODO: Cleanup intel_avb library
     * - Close all open Intel devices
     * - Release Intel adapter resources
     * - Cleanup intel_avb library state
     */
    
    printf("[INTEL_ADAPTER] Intel adapter cleanup completed (stub)\n");
    return NETWORK_HAL_SUCCESS;
}

/**
 * @brief Intel adapter device enumeration stub
 * 
 * Placeholder implementation for Intel device enumeration.
 * Will be replaced with real intel_avb integration in Phase 2.
 * 
 * @param[out] device_list      Array to receive device information
 * @param[in]  max_devices      Maximum devices to enumerate
 * @param[out] device_count     Actual number of devices found
 * 
 * @return NETWORK_HAL_SUCCESS on success, error code on failure
 */
static network_hal_result_t intel_adapter_enumerate_devices(
    network_hal_device_info_t *device_list,
    uint32_t max_devices,
    uint32_t *device_count
)
{
    printf("[INTEL_ADAPTER] Enumerating Intel devices...\n");
    
    /* TODO: Enumerate Intel devices via intel_avb
     * - Call intel_enum_adapters() to discover devices
     * - Populate device_list with Intel device information
     * - Set capability flags based on Intel device type
     * - Map Intel device paths to Generic HAL format
     */
    
    /* Stub implementation: no devices found */
    *device_count = 0;
    
    printf("[INTEL_ADAPTER] Intel device enumeration completed: %u devices (stub)\n", 
           *device_count);
    return NETWORK_HAL_SUCCESS;
}
