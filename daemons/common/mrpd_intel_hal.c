/*******************************************************************************
Copyright (c) 2025, Intel Corporation
All rights reserved.

MRPD Daemon Intel HAL Integration Implementation
Enhanced Intel adapter support with hardware timestamping for MRP protocols
*******************************************************************************/

#include "mrpd_intel_hal.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#include <syslog.h>
#endif

// Global Intel HAL state for MRPD
static mrpd_intel_hal_t g_mrpd_hal = {
    .available = false,
    .initialized = false,
    .interface_name = {0},
    .mac_address = {0},
    .last_timestamp = 0,
    .capabilities = 0,
    .registration_timestamps = {0},
    .timestamp_index = 0,
    .min_interval_ns = UINT64_MAX,
    .max_interval_ns = 0,
#ifdef OPENAVNU_BUILD_INTEL_HAL
    .device = NULL
#endif
};

// Logging macros for MRPD
#ifdef _WIN32
#define MRPD_LOG_INFO(fmt, ...) printf("[MRPD Intel HAL] INFO: " fmt "\n", ##__VA_ARGS__)
#define MRPD_LOG_WARNING(fmt, ...) printf("[MRPD Intel HAL] WARNING: " fmt "\n", ##__VA_ARGS__)
#define MRPD_LOG_ERROR(fmt, ...) printf("[MRPD Intel HAL] ERROR: " fmt "\n", ##__VA_ARGS__)
#define MRPD_LOG_STATUS(fmt, ...) printf("[MRPD Intel HAL] STATUS: " fmt "\n", ##__VA_ARGS__)
#else
#define MRPD_LOG_INFO(fmt, ...) syslog(LOG_INFO, "[MRPD Intel HAL] " fmt, ##__VA_ARGS__)
#define MRPD_LOG_WARNING(fmt, ...) syslog(LOG_WARNING, "[MRPD Intel HAL] " fmt, ##__VA_ARGS__)
#define MRPD_LOG_ERROR(fmt, ...) syslog(LOG_ERR, "[MRPD Intel HAL] " fmt, ##__VA_ARGS__)
#define MRPD_LOG_STATUS(fmt, ...) syslog(LOG_NOTICE, "[MRPD Intel HAL] " fmt, ##__VA_ARGS__)
#endif

/**
 * Initialize Intel HAL for MRPD daemon
 */
int mrpd_intel_hal_init(const char *interface_name) {
    if (g_mrpd_hal.initialized) {
        MRPD_LOG_WARNING("Intel HAL already initialized");
        return 0;
    }

    if (!interface_name || strlen(interface_name) == 0) {
        MRPD_LOG_ERROR("Invalid interface name for Intel HAL initialization");
        return -1;
    }

    // Copy interface name
    strncpy(g_mrpd_hal.interface_name, interface_name, sizeof(g_mrpd_hal.interface_name) - 1);
    g_mrpd_hal.interface_name[sizeof(g_mrpd_hal.interface_name) - 1] = '\0';

#ifdef OPENAVNU_BUILD_INTEL_HAL
    MRPD_LOG_INFO("Initializing Intel HAL for interface: %s", interface_name);

    // Initialize Intel HAL
    intel_hal_result_t result = intel_hal_init();
    if (result != INTEL_HAL_SUCCESS) {
        MRPD_LOG_INFO("Intel HAL initialization failed, using fallback timing");
        g_mrpd_hal.available = false;
        g_mrpd_hal.initialized = true;
        return 0; // Not a fatal error - fallback available
    }

    // Try to open Intel device by interface name
    result = intel_hal_open_device_by_name(interface_name, &g_mrpd_hal.device);
    if (result != INTEL_HAL_SUCCESS) {
        MRPD_LOG_INFO("Intel HAL device open failed for %s, using fallback", interface_name);
        g_mrpd_hal.available = false;
        g_mrpd_hal.initialized = true;
        return 0; // Not fatal - fallback available
    }

    // Enable timestamping if supported
    result = intel_hal_enable_timestamping(g_mrpd_hal.device, true);
    if (result == INTEL_HAL_SUCCESS) {
        MRPD_LOG_STATUS("Intel HAL timestamping enabled for %s", interface_name);
        g_mrpd_hal.available = true;
    } else {
        MRPD_LOG_WARNING("Intel HAL timestamping enable failed, using fallback");
        g_mrpd_hal.available = false;
    }

    // Get device capabilities and MAC address
    intel_device_info_t device_info;
    if (intel_hal_get_device_info(g_mrpd_hal.device, &device_info) == INTEL_HAL_SUCCESS) {
        memcpy(g_mrpd_hal.mac_address, device_info.mac_address, 6);
        g_mrpd_hal.capabilities = device_info.capabilities;
        
        MRPD_LOG_INFO("Intel device capabilities: 0x%08X", g_mrpd_hal.capabilities);
        MRPD_LOG_INFO("Intel device MAC: %02X:%02X:%02X:%02X:%02X:%02X",
                     g_mrpd_hal.mac_address[0], g_mrpd_hal.mac_address[1],
                     g_mrpd_hal.mac_address[2], g_mrpd_hal.mac_address[3],
                     g_mrpd_hal.mac_address[4], g_mrpd_hal.mac_address[5]);
    }

    g_mrpd_hal.initialized = true;
    
    if (g_mrpd_hal.available) {
        MRPD_LOG_STATUS("Intel HAL successfully initialized for MRPD daemon");
    } else {
        MRPD_LOG_INFO("Intel HAL partial initialization - using software fallback");
    }

    return 0;

#else
    MRPD_LOG_INFO("Intel HAL not compiled in, using software timing");
    g_mrpd_hal.available = false;
    g_mrpd_hal.initialized = true;
    return 0;
#endif
}

/**
 * Cleanup Intel HAL resources
 */
void mrpd_intel_hal_cleanup(void) {
    if (!g_mrpd_hal.initialized) {
        return;
    }

#ifdef OPENAVNU_BUILD_INTEL_HAL
    if (g_mrpd_hal.device) {
        intel_hal_close_device(g_mrpd_hal.device);
        g_mrpd_hal.device = NULL;
    }
    
    intel_hal_cleanup();
    MRPD_LOG_INFO("Intel HAL cleanup completed");
#endif

    memset(&g_mrpd_hal, 0, sizeof(g_mrpd_hal));
}

/**
 * Check if Intel HAL is available
 */
bool mrpd_intel_hal_is_available(void) {
    return g_mrpd_hal.initialized && g_mrpd_hal.available;
}

/**
 * Get high-precision timestamp from Intel HAL
 */
uint64_t mrpd_intel_hal_get_timestamp(void) {
    if (!mrpd_intel_hal_is_available()) {
        return mrpd_get_fallback_timestamp();
    }

#ifdef OPENAVNU_BUILD_INTEL_HAL
    intel_timestamp_t timestamp;
    intel_hal_result_t result = intel_hal_read_timestamp(g_mrpd_hal.device, &timestamp);
    
    if (result == INTEL_HAL_SUCCESS) {
        g_mrpd_hal.last_timestamp = timestamp.nanoseconds;
        return timestamp.nanoseconds;
    } else {
        MRPD_LOG_WARNING("Intel HAL timestamp read failed, using fallback");
        return mrpd_get_fallback_timestamp();
    }
#else
    return mrpd_get_fallback_timestamp();
#endif
}

/**
 * Record message timing for MRP protocols
 */
void mrpd_intel_hal_record_message_timing(uint64_t timestamp) {
    if (g_mrpd_hal.timestamp_index > 0) {
        uint64_t interval = timestamp - g_mrpd_hal.registration_timestamps[
            (g_mrpd_hal.timestamp_index - 1) % 64];
        
        if (interval < g_mrpd_hal.min_interval_ns) {
            g_mrpd_hal.min_interval_ns = interval;
        }
        if (interval > g_mrpd_hal.max_interval_ns) {
            g_mrpd_hal.max_interval_ns = interval;
        }
    }
    
    g_mrpd_hal.registration_timestamps[g_mrpd_hal.timestamp_index % 64] = timestamp;
    g_mrpd_hal.timestamp_index++;
}

/**
 * Get average interval between MRP messages
 */
uint64_t mrpd_intel_hal_get_average_interval(void) {
    if (g_mrpd_hal.timestamp_index < 2) {
        return 0;
    }
    
    int count = (g_mrpd_hal.timestamp_index < 64) ? g_mrpd_hal.timestamp_index : 64;
    uint64_t total_interval = 0;
    int intervals = 0;
    
    for (int i = 1; i < count; i++) {
        uint64_t interval = g_mrpd_hal.registration_timestamps[i] - 
                           g_mrpd_hal.registration_timestamps[i-1];
        total_interval += interval;
        intervals++;
    }
    
    return intervals > 0 ? (total_interval / intervals) : 0;
}

/**
 * Get MRPD-specific registration timestamp
 */
uint64_t mrpd_intel_hal_get_registration_timestamp(void) {
    uint64_t timestamp = mrpd_intel_hal_get_timestamp();
    mrpd_intel_hal_record_message_timing(timestamp);
    return timestamp;
}

/**
 * Get MRPD-specific leave timestamp
 */
uint64_t mrpd_intel_hal_get_leave_timestamp(void) {
    return mrpd_intel_hal_get_timestamp();
}

/**
 * Protocol-specific timestamp functions
 */
uint64_t mrpd_intel_hal_get_mvrp_timestamp(void) {
    // MVRP (VLAN Registration Protocol)
    return mrpd_intel_hal_get_registration_timestamp();
}

uint64_t mrpd_intel_hal_get_mmrp_timestamp(void) {
    // MMRP (MAC Registration Protocol) 
    return mrpd_intel_hal_get_registration_timestamp();
}

uint64_t mrpd_intel_hal_get_msrp_timestamp(void) {
    // MSRP (Stream Reservation Protocol) - needs highest precision
    return mrpd_intel_hal_get_registration_timestamp();
}

/**
 * Check if Intel HAL supports precision timing
 */
bool mrpd_intel_hal_supports_precision_timing(void) {
    if (!mrpd_intel_hal_is_available()) {
        return false;
    }

#ifdef OPENAVNU_BUILD_INTEL_HAL
    // Check if device supports hardware timestamping
    return (g_mrpd_hal.capabilities & INTEL_CAP_BASIC_1588) != 0;
#else
    return false;
#endif
}

/**
 * Get timing precision in nanoseconds
 */
int mrpd_intel_hal_get_timing_precision_ns(void) {
    if (!mrpd_intel_hal_supports_precision_timing()) {
        return 1000000; // 1ms fallback precision
    }

#ifdef OPENAVNU_BUILD_INTEL_HAL
    // Intel I219: ~500ns, I225/I226: ~100ns, I210: ~1000ns
    if (g_mrpd_hal.capabilities & INTEL_CAP_ENHANCED_TS) {
        return 100; // I225/I226 enhanced timestamping
    } else if (g_mrpd_hal.capabilities & INTEL_CAP_BASIC_1588) {
        return 500; // I219/I210 basic IEEE 1588
    }
#endif
    
    return 1000000; // 1ms software fallback
}

/**
 * Get interface information from Intel HAL
 */
int mrpd_intel_hal_get_interface_info(uint8_t *mac_address, uint32_t *capabilities) {
    if (!g_mrpd_hal.initialized) {
        return -1;
    }

    if (mac_address) {
        memcpy(mac_address, g_mrpd_hal.mac_address, 6);
    }

    if (capabilities) {
        *capabilities = g_mrpd_hal.capabilities;
    }

    return 0;
}

/**
 * Get interface name
 */
const char *mrpd_intel_hal_get_interface_name(void) {
    if (!g_mrpd_hal.initialized) {
        return NULL;
    }
    return g_mrpd_hal.interface_name;
}

/**
 * Fallback timestamp function (OS-level timing)
 */
uint64_t mrpd_get_fallback_timestamp(void) {
#ifdef _WIN32
    // Windows high-precision timing
    LARGE_INTEGER frequency, counter;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&counter);
    
    // Convert to nanoseconds
    return (uint64_t)((counter.QuadPart * 1000000000LL) / frequency.QuadPart);
#else
    // Linux/Unix high-precision timing
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)(ts.tv_sec * 1000000000LL + ts.tv_nsec);
#endif
}
