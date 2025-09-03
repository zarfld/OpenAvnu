/**
 * @file gptp_hal_integration.h
 * @brief gPTP Generic HAL Integration Interface
 * 
 * Provides integration layer between gPTP timestamping and Generic Network HAL.
 * Enables hardware timestamping via Intel I210/I219/I225/I226 adapters while
 * maintaining graceful fallback to software timestamping.
 * 
 * @author OpenAvnu Development Team
 * @date September 3, 2025
 * @version 1.0.0
 * 
 * Architecture Integration:
 * - Bridges gPTP EtherTimestamper interface with Generic HAL
 * - Preserves existing gPTP timestamping semantics
 * - Provides unified hardware detection across Windows/Linux
 * - Maintains backward compatibility with existing gPTP code
 * 
 * Hardware Support:
 * - Intel I226: Full TSN + PCIe PTM + 2.5G (±40ns accuracy)
 * - Intel I225: TSN + Frame Preemption + 2.5G (±40ns accuracy)  
 * - Intel I219: Enhanced timestamping (±40ns accuracy, corrected frequency)
 * - Intel I210: Basic AVB + hardware timestamping (±80ns accuracy)
 * - Other adapters: Software fallback (±1ms accuracy)
 * 
 * @copyright Copyright (c) 2025, The OpenAvnu Contributors
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __GPTP_HAL_INTEGRATION_H__
#define __GPTP_HAL_INTEGRATION_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

/* Include Generic Network HAL */
#include "../../../lib/common/hal/network_hal.h"

/* ============================================================================
 * CONSTANTS & DEFINITIONS
 * ============================================================================ */

/**
 * @brief gPTP HAL Integration Version Information
 * 
 * Used for version compatibility checking between gPTP and Generic HAL.
 */
#define GPTP_HAL_INTEGRATION_VERSION_MAJOR   1
#define GPTP_HAL_INTEGRATION_VERSION_MINOR   0
#define GPTP_HAL_INTEGRATION_VERSION_PATCH   0

/**
 * @brief Maximum number of network devices for gPTP use
 * 
 * Typically gPTP uses single network interface, but allow multiple for flexibility.
 */
#define GPTP_HAL_MAX_DEVICES                 4

/**
 * @brief gPTP timestamp accuracy targets
 * 
 * Based on IEEE 802.1AS requirements and Intel hardware capabilities.
 * Values in nanoseconds representing ±accuracy bounds.
 */
#define GPTP_HAL_ACCURACY_ENHANCED_HW        40    /**< Intel I219/I225/I226 enhanced timestamping */
#define GPTP_HAL_ACCURACY_BASIC_HW           80    /**< Intel I210 basic hardware timestamping */
#define GPTP_HAL_ACCURACY_SOFTWARE           1000  /**< Software timestamping fallback */

/**
 * @brief gPTP frequency adjustment limits
 * 
 * Frequency adjustment range in parts per billion (ppb).
 * Based on typical Intel hardware PLL capabilities.
 */
#define GPTP_HAL_FREQ_ADJ_MAX_PPB            100000  /**< ±100ppm typical Intel limit */
#define GPTP_HAL_FREQ_ADJ_MIN_PPB            -100000

/* ============================================================================
 * DATA STRUCTURES
 * ============================================================================ */

/**
 * @brief gPTP timestamp structure
 * 
 * Compatible with existing gPTP Timestamp format while providing
 * additional hardware timestamping information.
 */
typedef struct {
    /** Timestamp seconds (lower 32 bits) */
    uint32_t seconds_ls;
    
    /** Timestamp seconds (upper 16 bits) */
    uint16_t seconds_ms;
    
    /** Nanoseconds within second (0-999,999,999) */
    uint32_t nanoseconds;
    
    /** Timestamp version for compatibility tracking */
    uint16_t version;
    
    /** Hardware timestamp accuracy in nanoseconds */
    uint32_t accuracy_ns;
    
    /** Source of timestamp (hardware/software/cross-timestamp) */
    uint8_t timestamp_source;
    
    /** Vendor-specific flags */
    uint8_t vendor_flags;
} gptp_hal_timestamp_t;

/**
 * @brief gPTP HAL device context
 * 
 * Maintains gPTP-specific device state and Generic HAL integration.
 * Opaque structure for gPTP daemon use.
 */
typedef struct gptp_hal_device_context gptp_hal_device_context_t;

/**
 * @brief gPTP timestamping statistics
 * 
 * Runtime statistics for timestamping performance monitoring.
 * Used for quality assessment and debugging.
 */
typedef struct {
    /** Total hardware timestamp attempts */
    uint64_t hw_timestamp_attempts;
    
    /** Successful hardware timestamps */
    uint64_t hw_timestamp_successes;
    
    /** Software timestamp fallbacks */
    uint64_t sw_timestamp_fallbacks;
    
    /** Average timestamp accuracy (nanoseconds) */
    uint32_t average_accuracy_ns;
    
    /** Maximum timestamp accuracy achieved */
    uint32_t best_accuracy_ns;
    
    /** Timestamp accuracy standard deviation */
    uint32_t accuracy_std_dev_ns;
    
    /** Frequency adjustment count */
    uint32_t frequency_adjustments;
    
    /** Current frequency offset (ppb) */
    int32_t current_frequency_ppb;
    
} gptp_hal_statistics_t;

/**
 * @brief Timestamp source enumeration
 * 
 * Indicates the source of timestamp for accuracy assessment.
 */
typedef enum {
    GPTP_HAL_TIMESTAMP_SOURCE_UNKNOWN = 0,     /**< Unknown/uninitialized */
    GPTP_HAL_TIMESTAMP_SOURCE_INTEL_HW,        /**< Intel hardware timestamping */
    GPTP_HAL_TIMESTAMP_SOURCE_GENERIC_HW,      /**< Generic hardware timestamping */
    GPTP_HAL_TIMESTAMP_SOURCE_CROSS_TIMESTAMP, /**< Cross-timestamp correlation */
    GPTP_HAL_TIMESTAMP_SOURCE_SOFTWARE         /**< Software timestamping */
} gptp_hal_timestamp_source_t;

/* ============================================================================
 * CORE API FUNCTIONS
 * ============================================================================ */

/**
 * @brief Initialize gPTP HAL integration
 * 
 * Initializes Generic HAL and discovers gPTP-compatible network devices.
 * Must be called before any other gPTP HAL functions.
 * 
 * @return NETWORK_HAL_SUCCESS on success, error code on failure
 * 
 * Implementation Notes:
 * - Initializes Generic Network HAL if not already initialized
 * - Enumerates devices and filters for gPTP compatibility
 * - Sets up gPTP-specific device contexts
 * - Thread-safe initialization with reference counting
 * 
 * Hardware Context:
 * - Intel devices: Verified hardware timestamping capabilities
 * - Non-Intel devices: Marked for software timestamping fallback
 * - Device capabilities cached for fast runtime queries
 */
network_hal_result_t gptp_hal_init(void);

/**
 * @brief Cleanup gPTP HAL integration
 * 
 * Releases gPTP HAL resources and closes Generic HAL.
 * Should be called during gPTP daemon shutdown.
 * 
 * @return NETWORK_HAL_SUCCESS on success, error code on failure
 * 
 * Implementation Notes:
 * - Closes all open gPTP device contexts
 * - Releases Generic HAL resources (reference counted)
 * - Thread-safe cleanup matching init calls
 * - Safe to call multiple times or after init failure
 */
network_hal_result_t gptp_hal_cleanup(void);

/**
 * @brief Open gPTP device by interface name
 * 
 * Opens network device for gPTP timestamping operations.
 * Maps interface name to Generic HAL device and creates gPTP context.
 * 
 * @param[in]  interface_name   Network interface name (e.g., "Intel(R) I210...")
 * @param[out] device_context   gPTP device context for operations
 * 
 * @return NETWORK_HAL_SUCCESS on success, error code on failure
 * 
 * Implementation Notes:
 * - Matches interface name against enumerated Generic HAL devices
 * - Creates gPTP-specific context with timestamp conversion
 * - Initializes hardware timestamping if supported
 * - Falls back to software timestamping for unsupported hardware
 * 
 * Hardware Context:
 * - Intel devices: Hardware timestamping via intel_avb integration
 * - Other devices: Software timestamping with high-precision fallback
 * - Context contains device capabilities and conversion functions
 */
network_hal_result_t gptp_hal_device_open(
    const char *interface_name,
    gptp_hal_device_context_t **device_context
);

/**
 * @brief Close gPTP device context
 * 
 * Closes gPTP device context and releases associated resources.
 * Device context becomes invalid after successful close.
 * 
 * @param[in] device_context    gPTP device context from open
 * 
 * @return NETWORK_HAL_SUCCESS on success, error code on failure
 * 
 * Implementation Notes:
 * - Closes underlying Generic HAL device
 * - Releases gPTP-specific context memory
 * - Safe to call with NULL context (no-op)
 * - Thread-safe operation for context cleanup
 */
network_hal_result_t gptp_hal_device_close(
    gptp_hal_device_context_t *device_context
);

/* ============================================================================
 * TIMESTAMPING API
 * ============================================================================ */

/**
 * @brief Get gPTP system and device timestamps
 * 
 * Retrieves correlated system and device timestamps for gPTP synchronization.
 * Primary interface for gPTP HWTimestamper_gettime() integration.
 * 
 * @param[in]  device_context   gPTP device context
 * @param[out] system_time      System timestamp (CLOCK_REALTIME equivalent)
 * @param[out] device_time      Device hardware timestamp
 * @param[out] local_clock      Local clock identifier (optional, can be NULL)
 * @param[out] nominal_rate     Nominal clock rate (optional, can be NULL)
 * 
 * @return true on success, false on failure
 * 
 * Implementation Notes:
 * - Intel devices: Uses intel_gettime() for nanosecond-precision timestamps
 * - Cross-timestamp correlation provides system/device time relationship
 * - Software fallback maintains gPTP operation on any hardware
 * - Timestamp accuracy reported in accuracy_ns field
 * 
 * Hardware Context (Intel):
 * - Accesses SYSTIML/SYSTIMH registers for 64-bit nanosecond timestamp
 * - Provides hardware correlation with system CLOCK_REALTIME
 * - Enhanced timestamping delivers ±40ns accuracy on I219/I225/I226
 * - Basic 1588 timestamping provides ±80ns accuracy on I210
 * 
 * gPTP Integration:
 * - Direct replacement for WindowsEtherTimestamper::HWTimestamper_gettime()
 * - Compatible with existing gPTP Timestamp format
 * - Preserves gPTP error handling and retry semantics
 */
bool gptp_hal_get_time(
    gptp_hal_device_context_t *device_context,
    gptp_hal_timestamp_t *system_time,
    gptp_hal_timestamp_t *device_time,
    uint32_t *local_clock,
    uint32_t *nominal_rate
);

/**
 * @brief Set gPTP device time
 * 
 * Sets device hardware clock for gPTP master synchronization.
 * Used during gPTP initialization and phase adjustment.
 * 
 * @param[in] device_context    gPTP device context
 * @param[in] device_time       Target device timestamp
 * 
 * @return NETWORK_HAL_SUCCESS on success, error code on failure
 * 
 * Implementation Notes:
 * - Intel devices: Uses intel_set_systime() for direct register write
 * - Software timestamping: No-op (software clocks cannot be set)
 * - Atomic timestamp setting to avoid timestamp discontinuities
 * 
 * Hardware Context (Intel):
 * - Writes SYSTIML/SYSTIMH registers atomically
 * - May cause brief timestamp accuracy reduction during transition
 * - Used for gPTP master role initialization and major corrections
 */
network_hal_result_t gptp_hal_set_time(
    gptp_hal_device_context_t *device_context,
    const gptp_hal_timestamp_t *device_time
);

/**
 * @brief Adjust gPTP device frequency
 * 
 * Adjusts device hardware clock frequency for gPTP synchronization.
 * Primary mechanism for gPTP slave clock discipline.
 * 
 * @param[in] device_context    gPTP device context
 * @param[in] frequency_ppb     Frequency adjustment in parts per billion
 * 
 * @return NETWORK_HAL_SUCCESS on success, error code on failure
 * 
 * Implementation Notes:
 * - Intel devices: Uses intel_adjust_systime() for hardware PLL control
 * - Frequency range validation against hardware capabilities
 * - Software timestamping: Adjustment recorded for correction calculation
 * 
 * Hardware Context (Intel):
 * - Adjusts FREQOUT register for clock frequency compensation
 * - Hardware PLL provides smooth frequency transitions
 * - Typical range: ±100ppm (±100,000ppb) for Intel devices
 * - Maintains timestamp accuracy during adjustment periods
 * 
 * gPTP Integration:
 * - Called by gPTP clock servo for slave synchronization
 * - Frequency adjustments tracked for performance monitoring
 * - Critical path for gPTP ±80ns synchronization target
 */
network_hal_result_t gptp_hal_adjust_frequency(
    gptp_hal_device_context_t *device_context,
    int32_t frequency_ppb
);

/* ============================================================================
 * DEVICE INFORMATION API
 * ============================================================================ */

/**
 * @brief Get gPTP device capabilities
 * 
 * Retrieves device capabilities relevant to gPTP operation.
 * Used for feature detection and algorithm selection.
 * 
 * @param[in] device_context    gPTP device context
 * 
 * @return Capability flags (network_hal_capability_t bitmask)
 * 
 * Implementation Notes:
 * - Returns cached capability flags from device enumeration
 * - Fast operation suitable for runtime capability checking
 * - Used by gPTP daemon for timestamping method selection
 */
uint32_t gptp_hal_get_capabilities(
    gptp_hal_device_context_t *device_context
);

/**
 * @brief Get gPTP timestamping statistics
 * 
 * Retrieves runtime statistics for performance monitoring and debugging.
 * Used for gPTP quality assessment and troubleshooting.
 * 
 * @param[in]  device_context   gPTP device context
 * @param[out] statistics       Runtime statistics structure
 * 
 * @return NETWORK_HAL_SUCCESS on success, error code on failure
 * 
 * Implementation Notes:
 * - Statistics updated during timestamp operations
 * - Accuracy statistics calculated using sliding window
 * - Thread-safe statistics collection
 * - Useful for gPTP performance tuning and diagnostics
 */
network_hal_result_t gptp_hal_get_statistics(
    gptp_hal_device_context_t *device_context,
    gptp_hal_statistics_t *statistics
);

/**
 * @brief Check if Intel HAL is available
 * 
 * Tests if Intel hardware timestamping is available for this device.
 * Quick check for gPTP timestamping method selection.
 * 
 * @param[in] device_context    gPTP device context
 * 
 * @return true if Intel HAL available, false otherwise
 * 
 * Implementation Notes:
 * - Tests device vendor type and hardware timestamping capability
 * - Returns cached result from device initialization
 * - Used by gPTP daemon for optimal timestamping path selection
 */
bool gptp_hal_is_intel_available(
    gptp_hal_device_context_t *device_context
);

/**
 * @brief Get current timestamp accuracy
 * 
 * Returns current timestamp accuracy estimate in nanoseconds.
 * Used for gPTP quality assessment and synchronization validation.
 * 
 * @param[in] device_context    gPTP device context
 * 
 * @return Current accuracy estimate in nanoseconds
 * 
 * Implementation Notes:
 * - Intel enhanced timestamping: ~40ns typical
 * - Intel basic timestamping: ~80ns typical  
 * - Software timestamping: ~1000ns typical
 * - Accuracy may vary based on system load and hardware condition
 */
uint32_t gptp_hal_get_accuracy(
    gptp_hal_device_context_t *device_context
);

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

/**
 * @brief Convert Generic HAL timestamp to gPTP timestamp
 * 
 * Converts Generic Network HAL timestamp format to gPTP timestamp format.
 * Used internally for timestamp format bridging.
 * 
 * @param[in]  hal_timestamp    Generic HAL timestamp
 * @param[out] gptp_timestamp   gPTP timestamp format
 * 
 * Implementation Notes:
 * - Handles 64-bit nanosecond to seconds/nanoseconds conversion
 * - Preserves accuracy and source information
 * - Used throughout gPTP HAL integration layer
 */
void gptp_hal_convert_timestamp(
    const network_hal_timestamp_t *hal_timestamp,
    gptp_hal_timestamp_t *gptp_timestamp
);

/**
 * @brief Convert gPTP timestamp to Generic HAL timestamp  
 * 
 * Converts gPTP timestamp format to Generic Network HAL timestamp format.
 * Used internally for reverse timestamp format bridging.
 * 
 * @param[in]  gptp_timestamp   gPTP timestamp format
 * @param[out] hal_timestamp    Generic HAL timestamp
 * 
 * Implementation Notes:
 * - Handles seconds/nanoseconds to 64-bit nanosecond conversion
 * - Preserves accuracy information for Generic HAL
 * - Used for timestamp setting and frequency adjustment operations
 */
void gptp_hal_convert_timestamp_reverse(
    const gptp_hal_timestamp_t *gptp_timestamp,
    network_hal_timestamp_t *hal_timestamp
);

/**
 * @brief Get gPTP HAL version information
 * 
 * Returns version information for compatibility checking.
 * Used for version validation between gPTP daemon and HAL integration.
 * 
 * @param[out] major            Major version number
 * @param[out] minor            Minor version number  
 * @param[out] patch            Patch version number
 * 
 * Implementation Notes:
 * - Version follows semantic versioning (major.minor.patch)
 * - Used for API compatibility validation
 * - Thread-safe operation returning constant values
 */
void gptp_hal_get_version(
    uint32_t *major,
    uint32_t *minor,
    uint32_t *patch
);

#ifdef __cplusplus
}
#endif

#endif /* __GPTP_HAL_INTEGRATION_H__ */

/**
 * @example gptp_hal_usage.c
 * 
 * Basic usage example for gPTP HAL Integration:
 * 
 * ```c
 * #include "gptp_hal_integration.h"
 * 
 * // In gPTP daemon initialization
 * network_hal_result_t result = gptp_hal_init();
 * if (result != NETWORK_HAL_SUCCESS) {
 *     GPTP_LOG_ERROR("Failed to initialize gPTP HAL: %s", 
 *                    network_hal_result_string(result));
 *     return false;
 * }
 * 
 * // Open device for timestamping
 * gptp_hal_device_context_t *device_context;
 * result = gptp_hal_device_open("Intel(R) Ethernet Controller I219-V", &device_context);
 * if (result != NETWORK_HAL_SUCCESS) {
 *     GPTP_LOG_ERROR("Failed to open gPTP device: %s",
 *                    network_hal_result_string(result));
 *     gptp_hal_cleanup();
 *     return false;
 * }
 * 
 * // In HWTimestamper_gettime() implementation
 * bool WindowsEtherTimestamper::HWTimestamper_gettime(
 *     Timestamp *system_time, Timestamp *device_time,
 *     uint32_t *local_clock, uint32_t *nominal_clock_rate) const
 * {
 *     if (!gptp_device_context) {
 *         return false;  // Fallback to existing implementation
 *     }
 * 
 *     gptp_hal_timestamp_t hal_system_time, hal_device_time;
 *     
 *     bool success = gptp_hal_get_time(gptp_device_context,
 *                                      &hal_system_time, &hal_device_time,
 *                                      local_clock, nominal_clock_rate);
 *     
 *     if (success) {
 *         // Convert to gPTP timestamp format
 *         system_time->seconds_ls = hal_system_time.seconds_ls;
 *         system_time->seconds_ms = hal_system_time.seconds_ms;
 *         system_time->nanoseconds = hal_system_time.nanoseconds;
 *         system_time->_version = hal_system_time.version;
 *         
 *         device_time->seconds_ls = hal_device_time.seconds_ls;
 *         device_time->seconds_ms = hal_device_time.seconds_ms;
 *         device_time->nanoseconds = hal_device_time.nanoseconds;
 *         device_time->_version = hal_device_time.version;
 *         
 *         GPTP_LOG_DEBUG("gPTP HAL timestamp: accuracy=%uns, source=%d",
 *                        hal_device_time.accuracy_ns, hal_device_time.timestamp_source);
 *     }
 *     
 *     return success;
 * }
 * 
 * // In gPTP daemon cleanup
 * gptp_hal_device_close(device_context);
 * gptp_hal_cleanup();
 * ```
 */
