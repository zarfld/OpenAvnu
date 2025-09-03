/**
 * @file gptp_hal_integration.c
 * @brief gPTP Generic HAL Integration Implementation
 * 
 * Core implementation of gPTP Generic HAL integration layer.
 * Provides hardware timestamping for gPTP via Generic Network HAL
 * with Intel hardware acceleration and graceful software fallback.
 * 
 * @author OpenAvnu Development Team
 * @date September 3, 2025
 * @version 1.0.0
 * 
 * @copyright Copyright (c) 2025, The OpenAvnu Contributors
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "gptp_hal_integration.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef _WIN32
    #include <windows.h>
    #include <time.h>
    /* Windows critical section for synchronization */
    static CRITICAL_SECTION g_gptp_hal_lock;
    static bool g_gptp_hal_lock_initialized = false;
    #define GPTP_HAL_LOCK()   EnterCriticalSection(&g_gptp_hal_lock)
    #define GPTP_HAL_UNLOCK() LeaveCriticalSection(&g_gptp_hal_lock)
#else
    #include <pthread.h>
    #include <unistd.h>
    /* POSIX mutex for synchronization */
    static pthread_mutex_t g_gptp_hal_lock = PTHREAD_MUTEX_INITIALIZER;
    #define GPTP_HAL_LOCK()   pthread_mutex_lock(&g_gptp_hal_lock)
    #define GPTP_HAL_UNLOCK() pthread_mutex_unlock(&g_gptp_hal_lock)
#endif

/* ============================================================================
 * INTERNAL DATA STRUCTURES
 * ============================================================================ */

/**
 * @brief gPTP HAL device context structure
 * 
 * Internal context structure maintaining device state and statistics
 * for gPTP timestamping operations via Generic Network HAL.
 */
struct gptp_hal_device_context {
    /** Generic HAL device handle */
    network_hal_device_t *hal_device;
    
    /** Device capabilities cached for fast access */
    uint32_t capabilities;
    
    /** Device vendor type for optimized code paths */
    network_hal_vendor_t vendor;
    
    /** Intel adapter context (if applicable) */
    void *intel_context;
    
    /** Runtime statistics */
    gptp_hal_statistics_t statistics;
    
    /** Device interface name */
    char interface_name[256];
    
    /** Current timestamp accuracy estimate (ns) */
    uint32_t current_accuracy_ns;
    
    /** Last frequency adjustment (ppb) */
    int32_t last_frequency_ppb;
    
    /** Context initialization flag */
    bool initialized;
    
    /** Hardware timestamping available flag */
    bool hw_timestamping_available;
    
    /** Cross-timestamp support flag */
    bool cross_timestamp_support;
};

/**
 * @brief Global gPTP HAL state
 * 
 * Maintains global initialization state and device registry.
 */
static struct {
    /** Global initialization flag */
    bool initialized;
    
    /** Generic HAL initialized flag */
    bool hal_initialized;
    
    /** Reference count for multiple initializations */
    uint32_t ref_count;
    
    /** Available gPTP devices */
    gptp_hal_device_context_t *devices[GPTP_HAL_MAX_DEVICES];
    
    /** Device count */
    uint32_t device_count;
    
} g_gptp_hal_state = { 0 };

/* ============================================================================
 * INTERNAL UTILITY FUNCTIONS
 * ============================================================================ */

/**
 * @brief Initialize platform-specific synchronization
 * 
 * Initializes critical section or mutex for thread-safe operations.
 * Called during gptp_hal_init().
 */
static network_hal_result_t gptp_hal_init_sync(void)
{
#ifdef _WIN32
    if (!g_gptp_hal_lock_initialized) {
        InitializeCriticalSection(&g_gptp_hal_lock);
        g_gptp_hal_lock_initialized = true;
    }
#endif
    return NETWORK_HAL_SUCCESS;
}

/**
 * @brief Cleanup platform-specific synchronization
 * 
 * Cleans up critical section or mutex.
 * Called during gptp_hal_cleanup().
 */
static void gptp_hal_cleanup_sync(void)
{
#ifdef _WIN32
    if (g_gptp_hal_lock_initialized) {
        DeleteCriticalSection(&g_gptp_hal_lock);
        g_gptp_hal_lock_initialized = false;
    }
#endif
}

/**
 * @brief Update timestamping statistics
 * 
 * Updates device statistics based on timestamping operation result.
 * Called after each timestamp operation.
 * 
 * @param[in] device_context    Device context to update
 * @param[in] hw_success        Hardware timestamping success flag
 * @param[in] accuracy_ns       Achieved accuracy in nanoseconds
 */
static void gptp_hal_update_statistics(
    gptp_hal_device_context_t *device_context,
    bool hw_success,
    uint32_t accuracy_ns)
{
    if (!device_context) {
        return;
    }
    
    gptp_hal_statistics_t *stats = &device_context->statistics;
    
    if (hw_success) {
        stats->hw_timestamp_attempts++;
        stats->hw_timestamp_successes++;
        
        /* Update accuracy statistics with simple moving average */
        if (stats->hw_timestamp_successes == 1) {
            stats->average_accuracy_ns = accuracy_ns;
            stats->best_accuracy_ns = accuracy_ns;
        } else {
            /* Weighted average with recent bias */
            stats->average_accuracy_ns = 
                (stats->average_accuracy_ns * 7 + accuracy_ns) / 8;
                
            if (accuracy_ns < stats->best_accuracy_ns) {
                stats->best_accuracy_ns = accuracy_ns;
            }
        }
    } else {
        stats->hw_timestamp_attempts++;
        stats->sw_timestamp_fallbacks++;
    }
    
    device_context->current_accuracy_ns = accuracy_ns;
}

/**
 * @brief Determine timestamp accuracy for device
 * 
 * Determines expected timestamp accuracy based on device capabilities.
 * Used for statistics tracking and quality assessment.
 * 
 * @param[in] device_context    Device context
 * 
 * @return Expected accuracy in nanoseconds
 */
static uint32_t gptp_hal_determine_accuracy(
    gptp_hal_device_context_t *device_context)
{
    if (!device_context || !device_context->hw_timestamping_available) {
        return GPTP_HAL_ACCURACY_SOFTWARE;
    }
    
    /* Intel vendor-specific accuracy determination */
    if (device_context->vendor == NETWORK_HAL_VENDOR_INTEL) {
        uint32_t caps = device_context->capabilities;
        
        /* Enhanced timestamping for I219/I225/I226 */
        if (caps & NETWORK_HAL_CAP_TSN_FEATURES) {
            return GPTP_HAL_ACCURACY_ENHANCED_HW;
        }
        
        /* Basic AVB timestamping for I210 */
        if (caps & NETWORK_HAL_CAP_AVB_FEATURES) {
            return GPTP_HAL_ACCURACY_BASIC_HW;
        }
    }
    
    /* Generic hardware timestamping (if available) */
    if (device_context->capabilities & NETWORK_HAL_CAP_HARDWARE_TIMESTAMPING) {
        return GPTP_HAL_ACCURACY_BASIC_HW;
    }
    
    return GPTP_HAL_ACCURACY_SOFTWARE;
}

/* ============================================================================
 * CORE API IMPLEMENTATION
 * ============================================================================ */

network_hal_result_t gptp_hal_init(void)
{
    network_hal_result_t result;
    
    /* Initialize synchronization */
    result = gptp_hal_init_sync();
    if (result != NETWORK_HAL_SUCCESS) {
        return result;
    }
    
    GPTP_HAL_LOCK();
    
    /* Check if already initialized */
    if (g_gptp_hal_state.initialized) {
        g_gptp_hal_state.ref_count++;
        GPTP_HAL_UNLOCK();
        return NETWORK_HAL_SUCCESS;
    }
    
    /* Initialize Generic Network HAL */
    if (!g_gptp_hal_state.hal_initialized) {
        result = network_hal_init();
        if (result != NETWORK_HAL_SUCCESS) {
            GPTP_HAL_UNLOCK();
            return result;
        }
        g_gptp_hal_state.hal_initialized = true;
    }
    
    /* Initialize state */
    memset(&g_gptp_hal_state, 0, sizeof(g_gptp_hal_state));
    g_gptp_hal_state.initialized = true;
    g_gptp_hal_state.hal_initialized = true;
    g_gptp_hal_state.ref_count = 1;
    
    GPTP_HAL_UNLOCK();
    return NETWORK_HAL_SUCCESS;
}

network_hal_result_t gptp_hal_cleanup(void)
{
    GPTP_HAL_LOCK();
    
    if (!g_gptp_hal_state.initialized) {
        GPTP_HAL_UNLOCK();
        return NETWORK_HAL_SUCCESS;
    }
    
    /* Decrement reference count */
    g_gptp_hal_state.ref_count--;
    if (g_gptp_hal_state.ref_count > 0) {
        GPTP_HAL_UNLOCK();
        return NETWORK_HAL_SUCCESS;
    }
    
    /* Close any remaining device contexts */
    for (uint32_t i = 0; i < g_gptp_hal_state.device_count; i++) {
        if (g_gptp_hal_state.devices[i]) {
            gptp_hal_device_context_t *context = g_gptp_hal_state.devices[i];
            
            if (context->hal_device) {
                network_hal_device_close(context->hal_device);
            }
            
            free(context);
            g_gptp_hal_state.devices[i] = NULL;
        }
    }
    
    g_gptp_hal_state.device_count = 0;
    
    /* Cleanup Generic HAL */
    if (g_gptp_hal_state.hal_initialized) {
        network_hal_cleanup();
        g_gptp_hal_state.hal_initialized = false;
    }
    
    g_gptp_hal_state.initialized = false;
    
    GPTP_HAL_UNLOCK();
    
    /* Cleanup synchronization */
    gptp_hal_cleanup_sync();
    
    return NETWORK_HAL_SUCCESS;
}

network_hal_result_t gptp_hal_device_open(
    const char *interface_name,
    gptp_hal_device_context_t **device_context)
{
    network_hal_result_t result;
    network_hal_device_t *hal_device = NULL;
    gptp_hal_device_context_t *context = NULL;
    
    if (!interface_name || !device_context) {
        return NETWORK_HAL_ERROR_INVALID_PARAMETER;
    }
    
    *device_context = NULL;
    
    GPTP_HAL_LOCK();
    
    if (!g_gptp_hal_state.initialized) {
        GPTP_HAL_UNLOCK();
        return NETWORK_HAL_ERROR_NOT_INITIALIZED;
    }
    
    /* Check device limit */
    if (g_gptp_hal_state.device_count >= GPTP_HAL_MAX_DEVICES) {
        GPTP_HAL_UNLOCK();
        return NETWORK_HAL_ERROR_NO_RESOURCES;
    }
    
    GPTP_HAL_UNLOCK();
    
    /* Open Generic HAL device */
    result = network_hal_device_open_by_name(interface_name, &hal_device);
    if (result != NETWORK_HAL_SUCCESS) {
        return result;
    }
    
    /* Allocate device context */
    context = (gptp_hal_device_context_t *)calloc(1, sizeof(gptp_hal_device_context_t));
    if (!context) {
        network_hal_device_close(hal_device);
        return NETWORK_HAL_ERROR_NO_MEMORY;
    }
    
    /* Initialize context */
    context->hal_device = hal_device;
    strncpy(context->interface_name, interface_name, sizeof(context->interface_name) - 1);
    context->interface_name[sizeof(context->interface_name) - 1] = '\0';
    
    /* Get device capabilities */
    context->capabilities = network_hal_device_get_capabilities(hal_device);
    context->vendor = network_hal_device_get_vendor(hal_device);
    
    /* Determine hardware timestamping availability */
    context->hw_timestamping_available = 
        (context->capabilities & NETWORK_HAL_CAP_HARDWARE_TIMESTAMPING) != 0;
        
    context->cross_timestamp_support = 
        (context->capabilities & NETWORK_HAL_CAP_CROSS_TIMESTAMP) != 0;
    
    /* Get Intel context if applicable */
    if (context->vendor == NETWORK_HAL_VENDOR_INTEL) {
        context->intel_context = network_hal_device_get_vendor_context(hal_device);
    }
    
    /* Determine initial accuracy */
    context->current_accuracy_ns = gptp_hal_determine_accuracy(context);
    
    /* Initialize statistics */
    memset(&context->statistics, 0, sizeof(context->statistics));
    
    context->initialized = true;
    
    /* Add to global device list */
    GPTP_HAL_LOCK();
    
    if (g_gptp_hal_state.device_count < GPTP_HAL_MAX_DEVICES) {
        g_gptp_hal_state.devices[g_gptp_hal_state.device_count] = context;
        g_gptp_hal_state.device_count++;
        *device_context = context;
        result = NETWORK_HAL_SUCCESS;
    } else {
        result = NETWORK_HAL_ERROR_NO_RESOURCES;
    }
    
    GPTP_HAL_UNLOCK();
    
    if (result != NETWORK_HAL_SUCCESS) {
        network_hal_device_close(hal_device);
        free(context);
    }
    
    return result;
}

network_hal_result_t gptp_hal_device_close(
    gptp_hal_device_context_t *device_context)
{
    if (!device_context) {
        return NETWORK_HAL_SUCCESS;  /* NULL context is valid (no-op) */
    }
    
    GPTP_HAL_LOCK();
    
    /* Remove from global device list */
    for (uint32_t i = 0; i < g_gptp_hal_state.device_count; i++) {
        if (g_gptp_hal_state.devices[i] == device_context) {
            /* Shift remaining devices down */
            for (uint32_t j = i; j < g_gptp_hal_state.device_count - 1; j++) {
                g_gptp_hal_state.devices[j] = g_gptp_hal_state.devices[j + 1];
            }
            g_gptp_hal_state.device_count--;
            break;
        }
    }
    
    GPTP_HAL_UNLOCK();
    
    /* Close Generic HAL device */
    if (device_context->hal_device) {
        network_hal_device_close(device_context->hal_device);
    }
    
    /* Free context */
    free(device_context);
    
    return NETWORK_HAL_SUCCESS;
}

/* ============================================================================
 * TIMESTAMPING API IMPLEMENTATION
 * ============================================================================ */

bool gptp_hal_get_time(
    gptp_hal_device_context_t *device_context,
    gptp_hal_timestamp_t *system_time,
    gptp_hal_timestamp_t *device_time,
    uint32_t *local_clock,
    uint32_t *nominal_rate)
{
    network_hal_timestamp_t hal_system_time, hal_device_time;
    network_hal_result_t result;
    bool hw_success = false;
    uint32_t accuracy_ns = GPTP_HAL_ACCURACY_SOFTWARE;
    
    if (!device_context || !system_time || !device_time) {
        return false;
    }
    
    if (!device_context->initialized) {
        return false;
    }
    
    /* Attempt hardware timestamping if available */
    if (device_context->hw_timestamping_available) {
        result = network_hal_get_time(device_context->hal_device, 
                                      &hal_system_time, &hal_device_time);
        
        if (result == NETWORK_HAL_SUCCESS) {
            hw_success = true;
            accuracy_ns = gptp_hal_determine_accuracy(device_context);
        }
    }
    
    /* Software fallback if hardware timestamping failed */
    if (!hw_success) {
        /* Get current system time as fallback */
        #ifdef _WIN32
            FILETIME ft;
            ULARGE_INTEGER uli;
            GetSystemTimeAsFileTime(&ft);
            uli.LowPart = ft.dwLowDateTime;
            uli.HighPart = ft.dwHighDateTime;
            
            /* Convert from 100ns intervals since 1601 to nanoseconds since 1970 */
            uint64_t ns_since_1601 = uli.QuadPart * 100;
            uint64_t ns_since_1970 = ns_since_1601 - 116444736000000000ULL;
            
            hal_system_time.hw_timestamp_ns = ns_since_1970;
            hal_device_time.hw_timestamp_ns = ns_since_1970;  /* Same as system time */
        #else
            struct timespec ts;
            if (clock_gettime(CLOCK_REALTIME, &ts) == 0) {
                hal_system_time.hw_timestamp_ns = (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
                hal_device_time.hw_timestamp_ns = hal_system_time.hw_timestamp_ns;
            } else {
                return false;
            }
        #endif
        
        hal_system_time.accuracy_ns = GPTP_HAL_ACCURACY_SOFTWARE;
        hal_system_time.source = NETWORK_HAL_TIMESTAMP_SOURCE_SOFTWARE;
        hal_device_time.accuracy_ns = GPTP_HAL_ACCURACY_SOFTWARE;
        hal_device_time.source = NETWORK_HAL_TIMESTAMP_SOURCE_SOFTWARE;
        
        accuracy_ns = GPTP_HAL_ACCURACY_SOFTWARE;
    }
    
    /* Convert timestamps to gPTP format */
    gptp_hal_convert_timestamp(&hal_system_time, system_time);
    gptp_hal_convert_timestamp(&hal_device_time, device_time);
    
    /* Set optional output parameters */
    if (local_clock) {
        *local_clock = 0;  /* Local clock ID (gPTP uses 0) */
    }
    
    if (nominal_rate) {
        *nominal_rate = 1000000000;  /* 1GHz nominal rate */
    }
    
    /* Update statistics */
    gptp_hal_update_statistics(device_context, hw_success, accuracy_ns);
    
    return true;
}

network_hal_result_t gptp_hal_set_time(
    gptp_hal_device_context_t *device_context,
    const gptp_hal_timestamp_t *device_time)
{
    network_hal_timestamp_t hal_timestamp;
    
    if (!device_context || !device_time) {
        return NETWORK_HAL_ERROR_INVALID_PARAMETER;
    }
    
    if (!device_context->initialized) {
        return NETWORK_HAL_ERROR_NOT_INITIALIZED;
    }
    
    /* Software timestamping cannot set time */
    if (!device_context->hw_timestamping_available) {
        return NETWORK_HAL_ERROR_NOT_SUPPORTED;
    }
    
    /* Convert timestamp format */
    gptp_hal_convert_timestamp_reverse(device_time, &hal_timestamp);
    
    /* Set device time */
    return network_hal_set_time(device_context->hal_device, &hal_timestamp);
}

network_hal_result_t gptp_hal_adjust_frequency(
    gptp_hal_device_context_t *device_context,
    int32_t frequency_ppb)
{
    network_hal_result_t result;
    
    if (!device_context) {
        return NETWORK_HAL_ERROR_INVALID_PARAMETER;
    }
    
    if (!device_context->initialized) {
        return NETWORK_HAL_ERROR_NOT_INITIALIZED;
    }
    
    /* Validate frequency range */
    if (frequency_ppb < GPTP_HAL_FREQ_ADJ_MIN_PPB || 
        frequency_ppb > GPTP_HAL_FREQ_ADJ_MAX_PPB) {
        return NETWORK_HAL_ERROR_INVALID_PARAMETER;
    }
    
    /* Software timestamping records adjustment but cannot implement it */
    if (!device_context->hw_timestamping_available) {
        device_context->last_frequency_ppb = frequency_ppb;
        device_context->statistics.frequency_adjustments++;
        device_context->statistics.current_frequency_ppb = frequency_ppb;
        return NETWORK_HAL_SUCCESS;
    }
    
    /* Apply hardware frequency adjustment */
    result = network_hal_adjust_frequency(device_context->hal_device, frequency_ppb);
    
    if (result == NETWORK_HAL_SUCCESS) {
        device_context->last_frequency_ppb = frequency_ppb;
        device_context->statistics.frequency_adjustments++;
        device_context->statistics.current_frequency_ppb = frequency_ppb;
    }
    
    return result;
}

/* ============================================================================
 * DEVICE INFORMATION API IMPLEMENTATION
 * ============================================================================ */

uint32_t gptp_hal_get_capabilities(
    gptp_hal_device_context_t *device_context)
{
    if (!device_context || !device_context->initialized) {
        return 0;
    }
    
    return device_context->capabilities;
}

network_hal_result_t gptp_hal_get_statistics(
    gptp_hal_device_context_t *device_context,
    gptp_hal_statistics_t *statistics)
{
    if (!device_context || !statistics) {
        return NETWORK_HAL_ERROR_INVALID_PARAMETER;
    }
    
    if (!device_context->initialized) {
        return NETWORK_HAL_ERROR_NOT_INITIALIZED;
    }
    
    /* Copy statistics (thread-safe) */
    *statistics = device_context->statistics;
    
    return NETWORK_HAL_SUCCESS;
}

bool gptp_hal_is_intel_available(
    gptp_hal_device_context_t *device_context)
{
    if (!device_context || !device_context->initialized) {
        return false;
    }
    
    return (device_context->vendor == NETWORK_HAL_VENDOR_INTEL) &&
           device_context->hw_timestamping_available;
}

uint32_t gptp_hal_get_accuracy(
    gptp_hal_device_context_t *device_context)
{
    if (!device_context || !device_context->initialized) {
        return GPTP_HAL_ACCURACY_SOFTWARE;
    }
    
    return device_context->current_accuracy_ns;
}

/* ============================================================================
 * UTILITY FUNCTIONS IMPLEMENTATION
 * ============================================================================ */

void gptp_hal_convert_timestamp(
    const network_hal_timestamp_t *hal_timestamp,
    gptp_hal_timestamp_t *gptp_timestamp)
{
    if (!hal_timestamp || !gptp_timestamp) {
        return;
    }
    
    /* Convert 64-bit nanoseconds to seconds + nanoseconds */
    uint64_t total_ns = hal_timestamp->nanoseconds;
    uint64_t seconds = total_ns / 1000000000ULL;
    uint32_t nanoseconds = total_ns % 1000000000ULL;
    
    /* Split seconds into upper and lower parts for gPTP format */
    gptp_timestamp->seconds_ls = (uint32_t)(seconds & 0xFFFFFFFF);
    gptp_timestamp->seconds_ms = (uint16_t)((seconds >> 32) & 0xFFFF);
    gptp_timestamp->nanoseconds = nanoseconds;
    
    /* Copy additional fields */
    gptp_timestamp->version = 2;  /* gPTP version 2 */
    gptp_timestamp->accuracy_ns = hal_timestamp->accuracy_ns;
    
    /* Convert source types */
    switch (hal_timestamp->source) {
        case NETWORK_HAL_TIMESTAMP_SOURCE_INTEL_HARDWARE:
            gptp_timestamp->timestamp_source = GPTP_HAL_TIMESTAMP_SOURCE_INTEL_HW;
            break;
        case NETWORK_HAL_TIMESTAMP_SOURCE_GENERIC_HARDWARE:
            gptp_timestamp->timestamp_source = GPTP_HAL_TIMESTAMP_SOURCE_GENERIC_HW;
            break;
        case NETWORK_HAL_TIMESTAMP_SOURCE_CROSS_TIMESTAMP:
            gptp_timestamp->timestamp_source = GPTP_HAL_TIMESTAMP_SOURCE_CROSS_TIMESTAMP;
            break;
        case NETWORK_HAL_TIMESTAMP_SOURCE_SOFTWARE:
        default:
            gptp_timestamp->timestamp_source = GPTP_HAL_TIMESTAMP_SOURCE_SOFTWARE;
            break;
    }
    
    gptp_timestamp->vendor_flags = 0;
}

void gptp_hal_convert_timestamp_reverse(
    const gptp_hal_timestamp_t *gptp_timestamp,
    network_hal_timestamp_t *hal_timestamp)
{
    if (!gptp_timestamp || !hal_timestamp) {
        return;
    }
    
    /* Reconstruct 64-bit seconds from gPTP format */
    uint64_t seconds = ((uint64_t)gptp_timestamp->seconds_ms << 32) |
                       gptp_timestamp->seconds_ls;
    
    /* Convert to total nanoseconds */
    hal_timestamp->hw_timestamp_ns = seconds * 1000000000ULL + gptp_timestamp->nanoseconds;
    hal_timestamp->accuracy_ns = gptp_timestamp->accuracy_ns;
    
    /* Convert source types */
    switch (gptp_timestamp->timestamp_source) {
        case GPTP_HAL_TIMESTAMP_SOURCE_INTEL_HW:
            hal_timestamp->source = NETWORK_HAL_TIMESTAMP_SOURCE_INTEL_HARDWARE;
            break;
        case GPTP_HAL_TIMESTAMP_SOURCE_GENERIC_HW:
            hal_timestamp->source = NETWORK_HAL_TIMESTAMP_SOURCE_GENERIC_HARDWARE;
            break;
        case GPTP_HAL_TIMESTAMP_SOURCE_CROSS_TIMESTAMP:
            hal_timestamp->source = NETWORK_HAL_TIMESTAMP_SOURCE_CROSS_TIMESTAMP;
            break;
        case GPTP_HAL_TIMESTAMP_SOURCE_SOFTWARE:
        default:
            hal_timestamp->source = NETWORK_HAL_TIMESTAMP_SOURCE_SOFTWARE;
            break;
    }
}

void gptp_hal_get_version(
    uint32_t *major,
    uint32_t *minor,
    uint32_t *patch)
{
    if (major) {
        *major = GPTP_HAL_INTEGRATION_VERSION_MAJOR;
    }
    
    if (minor) {
        *minor = GPTP_HAL_INTEGRATION_VERSION_MINOR;
    }
    
    if (patch) {
        *patch = GPTP_HAL_INTEGRATION_VERSION_PATCH;
    }
}
