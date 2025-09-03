/**
 * @file network_hal.h
 * @brief Generic Network Hardware Abstraction Layer for OpenAvnu
 * 
 * Provides vendor-agnostic hardware abstraction for AVB/TSN network adapters.
 * Supports Intel I210/I219/I225/I226 via intel_avb integration with graceful
 * fallback to software-only operation on other hardware.
 * 
 * @author OpenAvnu Development Team
 * @date September 3, 2025
 * @version 1.0.0
 * 
 * Key Design Principles:
 * - Vendor-agnostic interface design
 * - Capability-based feature detection  
 * - Clean separation between generic and vendor-specific code
 * - Future-proof extensibility for additional vendors
 * - Respect for existing vendor implementations (intel_avb unchanged)
 * 
 * Hardware Requirements:
 * - Intel I210/I219/I225/I226: Full hardware timestamping and TSN features
 * - Other adapters: Software fallback with basic functionality
 * 
 * @copyright Copyright (c) 2025, The OpenAvnu Contributors
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __OPENAVNU_NETWORK_HAL_H__
#define __OPENAVNU_NETWORK_HAL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

/* ============================================================================
 * CONSTANTS & DEFINITIONS
 * ============================================================================ */

/**
 * @brief Generic Network HAL Version Information
 * 
 * Used for version compatibility checking and feature detection.
 */
#define NETWORK_HAL_VERSION_MAJOR   1
#define NETWORK_HAL_VERSION_MINOR   0
#define NETWORK_HAL_VERSION_PATCH   0

/**
 * @brief Maximum supported network devices
 * 
 * Conservative limit for device enumeration to prevent resource exhaustion.
 * Can be adjusted based on system requirements.
 */
#define NETWORK_HAL_MAX_DEVICES     16

/**
 * @brief Maximum device name length
 * 
 * Sufficient for Windows interface names and Linux device paths.
 */
#define NETWORK_HAL_MAX_NAME_LEN    256

/**
 * @brief Maximum vendor identification string length
 * 
 * Used for vendor-specific adapter selection and debugging.
 */
#define NETWORK_HAL_MAX_VENDOR_LEN  64

/* ============================================================================
 * CAPABILITY FLAGS
 * ============================================================================ */

/**
 * @brief Hardware capability flags
 * 
 * Bit flags indicating hardware features available on the network adapter.
 * Used for feature detection and graceful degradation.
 * 
 * Design rationale: Capability-based design allows runtime feature detection
 * and enables optimal code paths without hardcoded vendor assumptions.
 */
typedef enum {
    /** Basic network functionality (all adapters) */
    NETWORK_HAL_CAP_BASIC           = (1 << 0),
    
    /** Hardware timestamping support (Intel I210+) */
    NETWORK_HAL_CAP_HW_TIMESTAMP    = (1 << 1),
    
    /** IEEE 1588 PTP support (Intel I210+) */
    NETWORK_HAL_CAP_PTP             = (1 << 2),
    
    /** AVB traffic classes and credit-based shaper (Intel I210+) */
    NETWORK_HAL_CAP_AVB             = (1 << 3),
    
    /** Time-Aware Shaper 802.1Qbv (Intel I225+) */
    NETWORK_HAL_CAP_TIME_AWARE_SHAPER = (1 << 4),
    
    /** Frame Preemption 802.1Qbu (Intel I225+) */
    NETWORK_HAL_CAP_FRAME_PREEMPTION = (1 << 5),
    
    /** PCIe Precision Time Measurement (Intel I225+) */
    NETWORK_HAL_CAP_PCIE_PTM        = (1 << 6),
    
    /** 2.5 Gbps support (Intel I225+) */
    NETWORK_HAL_CAP_2_5G            = (1 << 7),
    
    /** MDIO register access (Intel all models) */
    NETWORK_HAL_CAP_MDIO            = (1 << 8),
    
    /** Memory-mapped I/O register access (Intel all models) */
    NETWORK_HAL_CAP_MMIO            = (1 << 9)
} network_hal_capability_t;

/* ============================================================================
 * RESULT CODES  
 * ============================================================================ */

/**
 * @brief Generic Network HAL result codes
 * 
 * Consistent error reporting across all vendor implementations.
 * Maps to vendor-specific error codes internally.
 */
typedef enum {
    /** Success */
    NETWORK_HAL_SUCCESS             = 0,
    
    /** Generic failure */
    NETWORK_HAL_ERROR_GENERAL       = -1,
    
    /** Invalid parameter */
    NETWORK_HAL_ERROR_INVALID_PARAM = -2,
    
    /** Device not found or not available */
    NETWORK_HAL_ERROR_DEVICE_NOT_FOUND = -3,
    
    /** Feature not supported on this hardware */
    NETWORK_HAL_ERROR_NOT_SUPPORTED = -4,
    
    /** Device busy or resource conflict */
    NETWORK_HAL_ERROR_DEVICE_BUSY   = -5,
    
    /** Insufficient memory or resources */
    NETWORK_HAL_ERROR_NO_MEMORY     = -6,
    
    /** Hardware access timeout */
    NETWORK_HAL_ERROR_TIMEOUT       = -7,
    
    /** Hardware I/O error */
    NETWORK_HAL_ERROR_IO            = -8
} network_hal_result_t;

/* ============================================================================
 * DATA STRUCTURES
 * ============================================================================ */

/**
 * @brief Network device vendor identification
 * 
 * Used for vendor-specific adapter selection and feature detection.
 */
typedef enum {
    NETWORK_HAL_VENDOR_UNKNOWN = 0,
    NETWORK_HAL_VENDOR_INTEL,
    NETWORK_HAL_VENDOR_ATHEROS,
    NETWORK_HAL_VENDOR_BROADCOM,
    NETWORK_HAL_VENDOR_REALTEK,
    NETWORK_HAL_VENDOR_OTHER
} network_hal_vendor_t;

/**
 * @brief Generic network device information
 * 
 * Vendor-agnostic device identification and capability information.
 * Populated during device enumeration and used for adapter selection.
 */
typedef struct {
    /** Device name/path (e.g., "Intel(R) Ethernet Controller I210" or "eth0") */
    char name[NETWORK_HAL_MAX_NAME_LEN];
    
    /** Hardware MAC address */
    uint8_t mac_address[6];
    
    /** Vendor identification */
    network_hal_vendor_t vendor;
    
    /** Vendor-specific identifier string */
    char vendor_info[NETWORK_HAL_MAX_VENDOR_LEN];
    
    /** PCI Vendor ID (0 if not applicable) */
    uint16_t vendor_id;
    
    /** PCI Device ID (0 if not applicable) */
    uint16_t device_id;
    
    /** Hardware capability flags */
    uint32_t capabilities;
    
    /** Link speed in Mbps (0 if not available) */
    uint32_t link_speed_mbps;
    
    /** Device is currently active and available */
    bool is_available;
    
    /** Vendor-specific context pointer (opaque) */
    void *vendor_context;
} network_hal_device_info_t;

/**
 * @brief Generic network device handle
 * 
 * Opaque handle representing an attached network device.
 * Contains vendor-specific context and function pointers.
 */
typedef struct network_hal_device network_hal_device_t;

/**
 * @brief Hardware timestamp structure
 * 
 * Unified timestamp representation for hardware timestamping.
 * Compatible with Intel hardware timestamp format.
 */
typedef struct {
    /** Timestamp in nanoseconds since device time base */
    uint64_t hw_timestamp_ns;
    
    /** Associated system timestamp for correlation */
    struct timespec system_timestamp;
    
    /** Timestamp accuracy/precision in nanoseconds */
    uint32_t accuracy_ns;
    
    /** Vendor-specific timestamp flags */
    uint32_t flags;
} network_hal_timestamp_t;

/* ============================================================================
 * CORE API FUNCTIONS
 * ============================================================================ */

/**
 * @brief Initialize the Generic Network HAL
 * 
 * Initializes the HAL framework and discovers available vendor adapters.
 * Must be called before any other HAL functions.
 * 
 * @return NETWORK_HAL_SUCCESS on success, error code on failure
 * 
 * Implementation Notes:
 * - Automatically detects and initializes vendor-specific adapters
 * - Intel adapter initialized via intel_avb library if available  
 * - Safe to call multiple times (reference counted)
 * - Thread-safe initialization
 */
network_hal_result_t network_hal_init(void);

/**
 * @brief Cleanup the Generic Network HAL
 * 
 * Releases all resources and closes vendor adapters.
 * Should be called at application shutdown.
 * 
 * @return NETWORK_HAL_SUCCESS on success, error code on failure
 * 
 * Implementation Notes:
 * - Automatically closes all open devices
 * - Releases vendor-specific resources
 * - Reference counted cleanup (matches init calls)
 * - Safe to call even if init failed
 */
network_hal_result_t network_hal_cleanup(void);

/**
 * @brief Enumerate available network devices
 * 
 * Discovers all network devices and populates device information array.
 * Used for device selection and capability assessment.
 * 
 * @param[out] device_list      Array to receive device information
 * @param[in]  max_devices      Maximum devices to enumerate
 * @param[out] device_count     Actual number of devices found
 * 
 * @return NETWORK_HAL_SUCCESS on success, error code on failure
 * 
 * Implementation Notes:
 * - Combines devices from all registered vendor adapters
 * - Intel devices enumerated via intel_enum_adapters() when available
 * - Device capability flags populated during enumeration
 * - Results cached for performance (refresh via separate API)
 */
network_hal_result_t network_hal_enumerate_devices(
    network_hal_device_info_t *device_list,
    uint32_t max_devices,
    uint32_t *device_count
);

/**
 * @brief Open and attach to a specific network device
 * 
 * Establishes connection to network device and prepares for operations.
 * Device must have been previously enumerated.
 * 
 * @param[in]  device_info      Device information from enumeration
 * @param[out] device_handle    Handle for subsequent operations
 * 
 * @return NETWORK_HAL_SUCCESS on success, error code on failure
 * 
 * Implementation Notes:
 * - Delegates to appropriate vendor-specific attach function
 * - Intel devices attached via intel_attach() from intel_avb
 * - Performs capability validation and device initialization
 * - Device handle contains vendor-specific context and function pointers
 */
network_hal_result_t network_hal_device_open(
    const network_hal_device_info_t *device_info,
    network_hal_device_t **device_handle
);

/**
 * @brief Close and detach from network device
 * 
 * Releases device resources and closes vendor-specific connection.
 * Device handle becomes invalid after successful close.
 * 
 * @param[in] device_handle     Device handle from open operation
 * 
 * @return NETWORK_HAL_SUCCESS on success, error code on failure
 * 
 * Implementation Notes:
 * - Delegates to vendor-specific detach function
 * - Intel devices detached via intel_detach() from intel_avb
 * - Automatically releases all associated resources
 * - Safe to call with NULL handle (no-op)
 */
network_hal_result_t network_hal_device_close(
    network_hal_device_t *device_handle
);

/* ============================================================================
 * HARDWARE TIMESTAMPING API
 * ============================================================================ */

/**
 * @brief Get current hardware time
 * 
 * Retrieves high-precision hardware timestamp from device.
 * Primary interface for gPTP time synchronization.
 * 
 * @param[in]  device_handle    Device handle from open operation  
 * @param[out] timestamp        Hardware timestamp structure
 * 
 * @return NETWORK_HAL_SUCCESS on success, error code on failure
 * 
 * Implementation Notes:
 * - Intel devices: Uses intel_gettime() for nanosecond precision
 * - Non-Intel devices: Software fallback with system clock
 * - Critical path for gPTP synchronization (≤ ±80ns target)
 * - Timestamp includes accuracy estimation for quality assessment
 * 
 * Hardware Context (Intel):
 * - Accesses SYSTIML/SYSTIMH registers via intel_avb
 * - Provides hardware correlation with system time
 * - Supports both IEEE 1588 and proprietary timestamp formats
 */
network_hal_result_t network_hal_get_time(
    network_hal_device_t *device_handle,
    network_hal_timestamp_t *timestamp
);

/**
 * @brief Set hardware time
 * 
 * Sets device hardware clock to specified time.
 * Used for time synchronization initialization.
 * 
 * @param[in] device_handle     Device handle from open operation
 * @param[in] timestamp         Target hardware timestamp  
 * 
 * @return NETWORK_HAL_SUCCESS on success, error code on failure
 * 
 * Implementation Notes:
 * - Intel devices: Uses intel_set_systime() for direct register access
 * - Non-Intel devices: No-op (software timestamps only)
 * - Used during gPTP master clock synchronization
 * 
 * Hardware Context (Intel):
 * - Writes SYSTIML/SYSTIMH registers atomically
 * - Triggers hardware timestamp reset and recalibration
 * - May affect in-flight packet timestamps briefly
 */
network_hal_result_t network_hal_set_time(
    network_hal_device_t *device_handle,
    const network_hal_timestamp_t *timestamp
);

/**
 * @brief Adjust hardware time frequency
 * 
 * Adjusts hardware clock frequency for fine synchronization.
 * Primary mechanism for gPTP clock discipline.
 * 
 * @param[in] device_handle     Device handle from open operation
 * @param[in] frequency_ppb     Frequency adjustment in parts per billion
 * 
 * @return NETWORK_HAL_SUCCESS on success, error code on failure
 * 
 * Implementation Notes:
 * - Intel devices: Uses intel_adjust_systime() for hardware clock adjustment  
 * - Non-Intel devices: Software compensation only
 * - Critical for maintaining gPTP synchronization accuracy
 * - Frequency adjustment range vendor-specific (typically ±100ppm)
 * 
 * Hardware Context (Intel):
 * - Adjusts FREQOUT register for clock frequency compensation
 * - Hardware PLL provides smooth frequency transitions
 * - Maintains timestamp accuracy during adjustment periods
 */
network_hal_result_t network_hal_adjust_frequency(
    network_hal_device_t *device_handle,
    int32_t frequency_ppb
);

/* ============================================================================
 * TSN FEATURE API  
 * ============================================================================ */

/**
 * @brief Configure Time-Aware Shaper (IEEE 802.1Qbv)
 * 
 * Configures hardware time-aware scheduling for deterministic latency.
 * Available on Intel I225/I226 adapters.
 * 
 * @param[in] device_handle     Device handle from open operation
 * @param[in] config           Time-aware shaper configuration
 * 
 * @return NETWORK_HAL_SUCCESS on success, error code on failure
 *         NETWORK_HAL_ERROR_NOT_SUPPORTED if hardware lacks TAS support
 * 
 * Implementation Notes:
 * - Intel I225+: Uses intel_setup_time_aware_shaper() from intel_avb
 * - Other devices: Returns NETWORK_HAL_ERROR_NOT_SUPPORTED
 * - Requires NETWORK_HAL_CAP_TIME_AWARE_SHAPER capability
 * 
 * Hardware Context (Intel I225/I226):
 * - Configures QBVCTRL, QBVCYCLET, QBVGCL registers
 * - Programs gate control list for up to 8 traffic classes
 * - Synchronizes with hardware PTP clock for precise timing
 */
network_hal_result_t network_hal_configure_time_aware_shaper(
    network_hal_device_t *device_handle,
    const void *config  /* vendor-specific config structure */
);

/**
 * @brief Configure Frame Preemption (IEEE 802.1Qbu)  
 * 
 * Configures hardware frame preemption for ultra-low latency.
 * Available on Intel I225/I226 adapters.
 * 
 * @param[in] device_handle     Device handle from open operation
 * @param[in] config           Frame preemption configuration
 * 
 * @return NETWORK_HAL_SUCCESS on success, error code on failure
 *         NETWORK_HAL_ERROR_NOT_SUPPORTED if hardware lacks FP support
 * 
 * Implementation Notes:
 * - Intel I225+: Uses intel_setup_frame_preemption() from intel_avb
 * - Other devices: Returns NETWORK_HAL_ERROR_NOT_SUPPORTED  
 * - Requires NETWORK_HAL_CAP_FRAME_PREEMPTION capability
 * 
 * Hardware Context (Intel I225/I226):
 * - Configures PFCTRL register for preemption control
 * - Sets minimum fragment size and preemptable queue mask
 * - Coordinates with time-aware shaper for optimal scheduling
 */
network_hal_result_t network_hal_configure_frame_preemption(
    network_hal_device_t *device_handle,
    const void *config  /* vendor-specific config structure */
);

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

/**
 * @brief Check device capability
 * 
 * Tests if device supports specific hardware feature.
 * Used for runtime feature detection and graceful degradation.
 * 
 * @param[in] device_handle     Device handle from open operation
 * @param[in] capability        Capability flag to test
 * 
 * @return true if capability supported, false otherwise
 * 
 * Implementation Notes:
 * - Checks capability flags from device enumeration
 * - Intel devices: Maps to intel_has_capability() internally
 * - Fast inline operation (capability flags cached)
 */
bool network_hal_has_capability(
    network_hal_device_t *device_handle,
    network_hal_capability_t capability
);

/**
 * @brief Get device information
 * 
 * Retrieves current device information and status.
 * Used for debugging and status reporting.
 * 
 * @param[in]  device_handle    Device handle from open operation
 * @param[out] device_info      Current device information  
 * 
 * @return NETWORK_HAL_SUCCESS on success, error code on failure
 * 
 * Implementation Notes:
 * - Updates dynamic information (link status, statistics)
 * - Static information (capabilities, IDs) cached from enumeration
 * - Thread-safe for multiple concurrent queries
 */
network_hal_result_t network_hal_get_device_info(
    network_hal_device_t *device_handle,
    network_hal_device_info_t *device_info
);

/**
 * @brief Convert result code to human-readable string
 * 
 * Provides error descriptions for debugging and user feedback.
 * 
 * @param[in] result            Network HAL result code
 * 
 * @return Constant string describing the result code
 * 
 * Implementation Notes:
 * - Returns constant strings (no memory allocation)
 * - Thread-safe operation
 * - Handles both generic and vendor-specific error codes
 */
const char* network_hal_result_string(network_hal_result_t result);

#ifdef __cplusplus
}
#endif

#endif /* __OPENAVNU_NETWORK_HAL_H__ */

/**
 * @example generic_hal_usage.c
 * 
 * Basic usage example for Generic Network HAL:
 * 
 * ```c
 * #include "network_hal.h"
 * 
 * int main() {
 *     network_hal_result_t result;
 *     network_hal_device_info_t devices[NETWORK_HAL_MAX_DEVICES];
 *     uint32_t device_count;
 *     network_hal_device_t *intel_device = NULL;
 * 
 *     // Initialize HAL
 *     result = network_hal_init();
 *     if (result != NETWORK_HAL_SUCCESS) {
 *         printf("HAL init failed: %s\n", network_hal_result_string(result));
 *         return -1;
 *     }
 * 
 *     // Enumerate devices
 *     result = network_hal_enumerate_devices(devices, NETWORK_HAL_MAX_DEVICES, &device_count);
 *     if (result != NETWORK_HAL_SUCCESS) {
 *         printf("Device enumeration failed: %s\n", network_hal_result_string(result));
 *         goto cleanup;
 *     }
 * 
 *     // Find Intel device with hardware timestamping
 *     for (uint32_t i = 0; i < device_count; i++) {
 *         if (devices[i].vendor == NETWORK_HAL_VENDOR_INTEL &&
 *             devices[i].capabilities & NETWORK_HAL_CAP_HW_TIMESTAMP) {
 * 
 *             result = network_hal_device_open(&devices[i], &intel_device);
 *             if (result == NETWORK_HAL_SUCCESS) {
 *                 printf("Opened Intel device: %s\n", devices[i].name);
 *                 break;
 *             }
 *         }
 *     }
 * 
 *     if (intel_device) {
 *         // Use hardware timestamping
 *         network_hal_timestamp_t timestamp;
 *         result = network_hal_get_time(intel_device, &timestamp);
 *         if (result == NETWORK_HAL_SUCCESS) {
 *             printf("Hardware time: %llu ns\n", timestamp.hw_timestamp_ns);
 *         }
 * 
 *         // Cleanup
 *         network_hal_device_close(intel_device);
 *     }
 * 
 * cleanup:
 *     network_hal_cleanup();
 *     return 0;
 * }
 * ```
 */
