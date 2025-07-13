/*******************************************************************************
Copyright (c) 2025, Intel Corporation
All rights reserved.

MAAP Daemon Intel HAL Integration Implementation
Enhanced Intel adapter support with hardware timestamping for MAAP protocol
*******************************************************************************/

#ifdef _WIN32
// Fix WinSock header inclusion order for Windows builds
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/time.h>
#endif

#include "maap_intel_hal.h"
#include "maap_log.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

// Use Intel HAL specific log component name
#undef MAAP_LOG_COMPONENT
#define MAAP_LOG_COMPONENT "Intel_HAL"

// Global Intel HAL state for MAAP
static maap_intel_hal_t g_maap_hal = {
    .available = false,
    .initialized = false,
    .interface_name = {0},
    .mac_address = {0},
    .last_timestamp = 0,
    .capabilities = 0,
#ifdef OPENAVNU_BUILD_INTEL_HAL
    .device = NULL
#endif
};

/**
 * Initialize Intel HAL for MAAP daemon
 */
int maap_intel_hal_init(const char *interface_name) {
    if (g_maap_hal.initialized) {
        MAAP_LOG_WARNING("Intel HAL already initialized");
        return 0;
    }

    if (!interface_name || strlen(interface_name) == 0) {
        MAAP_LOG_ERROR("Invalid interface name for Intel HAL initialization");
        return -1;
    }

    // Copy interface name
    strncpy(g_maap_hal.interface_name, interface_name, sizeof(g_maap_hal.interface_name) - 1);
    g_maap_hal.interface_name[sizeof(g_maap_hal.interface_name) - 1] = '\0';

#ifdef OPENAVNU_BUILD_INTEL_HAL
    MAAP_LOG_INFO("Initializing Intel HAL for interface: %s", interface_name);

    // Initialize Intel HAL
    intel_hal_result_t result = intel_hal_init();
    if (result != INTEL_HAL_SUCCESS) {
        MAAP_LOG_INFO("Intel HAL initialization failed, using fallback timing");
        g_maap_hal.available = false;
        g_maap_hal.initialized = true;
        return 0; // Not a fatal error - fallback available
    }

    // Try to open Intel device by interface name
    result = intel_hal_open_device(interface_name, &g_maap_hal.device);
    if (result != INTEL_HAL_SUCCESS) {
        MAAP_LOG_INFO("Intel HAL device open failed for %s, using fallback", interface_name);
        g_maap_hal.available = false;
        g_maap_hal.initialized = true;
        return 0; // Not fatal - fallback available
    }

    // Enable timestamping if supported
    result = intel_hal_enable_timestamping(g_maap_hal.device, true);
    if (result == INTEL_HAL_SUCCESS) {
        MAAP_LOG_STATUS("Intel HAL timestamping enabled for %s", interface_name);
        g_maap_hal.available = true;
    } else {
        MAAP_LOG_WARNING("Intel HAL timestamping enable failed, using fallback");
        g_maap_hal.available = false;
    }

    // Get device capabilities and MAC address
    intel_device_info_t device_info;
    intel_interface_info_t interface_info;
    
    if (intel_hal_get_device_info(&g_maap_hal.device, &device_info) == INTEL_HAL_SUCCESS) {
        g_maap_hal.capabilities = device_info.capabilities;
        MAAP_LOG_INFO("Intel device capabilities: 0x%08X", g_maap_hal.capabilities);
    }
    
    if (intel_hal_get_interface_info(&g_maap_hal.device, &interface_info) == INTEL_HAL_SUCCESS) {
        memcpy(g_maap_hal.mac_address, interface_info.mac_address, 6);
        MAAP_LOG_INFO("Intel device MAC: %02X:%02X:%02X:%02X:%02X:%02X",
                     g_maap_hal.mac_address[0], g_maap_hal.mac_address[1],
                     g_maap_hal.mac_address[2], g_maap_hal.mac_address[3],
                     g_maap_hal.mac_address[4], g_maap_hal.mac_address[5]);
    }

    g_maap_hal.initialized = true;
    
    if (g_maap_hal.available) {
        MAAP_LOG_STATUS("Intel HAL successfully initialized for MAAP daemon");
    } else {
        MAAP_LOG_INFO("Intel HAL partial initialization - using software fallback");
    }

    return 0;

#else
    MAAP_LOG_INFO("Intel HAL not compiled in, using software timing");
    g_maap_hal.available = false;
    g_maap_hal.initialized = true;
    return 0;
#endif
}

/**
 * Cleanup Intel HAL resources
 */
void maap_intel_hal_cleanup(void) {
    if (!g_maap_hal.initialized) {
        return;
    }

#ifdef OPENAVNU_BUILD_INTEL_HAL
    if (g_maap_hal.device) {
        intel_hal_close_device(g_maap_hal.device);
        g_maap_hal.device = NULL;
    }
    
    intel_hal_cleanup();
    MAAP_LOG_INFO("Intel HAL cleanup completed");
#endif

    memset(&g_maap_hal, 0, sizeof(g_maap_hal));
}

/**
 * Check if Intel HAL is available
 */
bool maap_intel_hal_is_available(void) {
    return g_maap_hal.initialized && g_maap_hal.available;
}

/**
 * Get high-precision timestamp from Intel HAL
 */
uint64_t maap_intel_hal_get_timestamp(void) {
    if (!maap_intel_hal_is_available()) {
        return maap_get_fallback_timestamp();
    }

#ifdef OPENAVNU_BUILD_INTEL_HAL
    intel_timestamp_t timestamp;
    intel_hal_result_t result = intel_hal_read_timestamp(g_maap_hal.device, &timestamp);
    
    if (result == INTEL_HAL_SUCCESS) {
        g_maap_hal.last_timestamp = timestamp.nanoseconds;
        return timestamp.nanoseconds;
    } else {
        MAAP_LOG_WARNING("Intel HAL timestamp read failed, using fallback");
        return maap_get_fallback_timestamp();
    }
#else
    return maap_get_fallback_timestamp();
#endif
}

/**
 * Get MAAP-specific allocation timestamp
 */
uint64_t maap_intel_hal_get_allocation_timestamp(void) {
    // For MAAP, we want precise timing for address allocation conflicts
    return maap_intel_hal_get_timestamp();
}

/**
 * Check if Intel HAL supports precision timing
 */
bool maap_intel_hal_supports_precision_timing(void) {
    if (!maap_intel_hal_is_available()) {
        return false;
    }

#ifdef OPENAVNU_BUILD_INTEL_HAL
    // Check if device supports hardware timestamping
    return (g_maap_hal.capabilities & INTEL_CAP_BASIC_1588) != 0;
#else
    return false;
#endif
}

/**
 * Get timing precision in nanoseconds
 */
int maap_intel_hal_get_timing_precision_ns(void) {
    if (!maap_intel_hal_supports_precision_timing()) {
        return 1000000; // 1ms fallback precision
    }

#ifdef OPENAVNU_BUILD_INTEL_HAL
    // Intel I219: ~500ns, I225/I226: ~100ns, I210: ~1000ns
    if (g_maap_hal.capabilities & INTEL_CAP_ENHANCED_TS) {
        return 100; // I225/I226 enhanced timestamping
    } else if (g_maap_hal.capabilities & INTEL_CAP_BASIC_1588) {
        return 500; // I219/I210 basic IEEE 1588
    }
#endif
    
    return 1000000; // 1ms software fallback
}

/**
 * Get interface information from Intel HAL
 */
int maap_intel_hal_get_interface_info(uint8_t *mac_address, uint32_t *capabilities) {
    if (!g_maap_hal.initialized) {
        return -1;
    }

    if (mac_address) {
        memcpy(mac_address, g_maap_hal.mac_address, 6);
    }

    if (capabilities) {
        *capabilities = g_maap_hal.capabilities;
    }

    return 0;
}

/**
 * Get interface name
 */
const char *maap_intel_hal_get_interface_name(void) {
    if (!g_maap_hal.initialized) {
        return NULL;
    }
    return g_maap_hal.interface_name;
}

/**
 * Fallback timestamp function (OS-level timing)
 */
uint64_t maap_get_fallback_timestamp(void) {
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
