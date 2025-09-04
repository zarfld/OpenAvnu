#ifndef INTEL_TSN_INTEGRATION_H
#define INTEL_TSN_INTEGRATION_H

#include <stdint.h>
#include <stdbool.h>
#include <errno.h>

/* Ensure proper Windows header management before including Intel HAL */
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#define _WINSOCKAPI_  // Prevent winsock.h inclusion
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#endif

/* Use Intel Ethernet HAL for hardware access */
#include "../../thirdparty/intel-ethernet-hal/include/intel_ethernet_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Intel AVB Stream Classes - Use HAL definitions directly
 * Note: INTEL_AVB_CLASS_A and INTEL_AVB_CLASS_B are defined in intel_ethernet_hal.h
 */
typedef int intel_avb_stream_class_t;

/**
 * Intel TSN status structure
 */
typedef struct {
    char interface_name[256];        /**< Network interface name */
    char device_id[64];             /**< Intel device ID */
    uint32_t device_family;         /**< Device family (I210/I219/I225/I226) */
    char driver_version[32];        /**< Driver version */
    
    /* TSN Capabilities */
    bool tas_capable;               /**< Time-Aware Shaper capability */
    bool fp_capable;                /**< Frame Preemption capability */
    bool hardware_timestamping;     /**< Hardware timestamping capability */
    
    /* Current TSN State */
    bool tas_enabled;               /**< Time-Aware Shaper enabled */
    bool fp_enabled;                /**< Frame Preemption enabled */
    bool timed_tx_enabled;          /**< Timed transmission enabled */
    uint64_t reserved_bandwidth_bps; /**< Reserved bandwidth in bps */
} intel_tsn_status_t;

/**
 * Initialize Intel TSN integration with AVB pipeline
 * @param interface_name Network interface name (e.g. "Intel(R) Ethernet Controller I225-V")
 * @return 0 on success, negative on error
 */
int intel_tsn_init(const char* interface_name);

/**
 * Initialize Intel TSN integration with a specific Intel device ID
 * @param interface_name Network interface name
 * @param target_device_id Intel device ID (e.g., 0x1533 for I210, 0x125c for I226-V)
 * @return 0 on success, negative on error
 */
int intel_tsn_init_with_device(const char* interface_name, uint16_t target_device_id);

/**
 * Get TSN status and capabilities
 * @param status Pointer to status structure to fill
 * @return 0 on success, negative on error  
 */
int intel_tsn_get_status(intel_tsn_status_t* status);

/**
 * Configure Time-Aware Shaper for specific stream class
 * @param stream_class Intel AVB stream class (A or B)
 * @param bandwidth_mbps Bandwidth in Mbps to reserve
 * @return 0 on success, negative on error
 */
int intel_tsn_configure_tas(intel_avb_stream_class_t stream_class, int bandwidth_mbps);

/**
 * Configure Frame Preemption
 * @param enable Enable or disable frame preemption
 * @return 0 on success, negative on error
 */
int intel_tsn_configure_frame_preemption(bool enable);

/**
 * Configure timed transmission for specific stream class
 * @param stream_class Intel AVB stream class (A or B)
 * @param tx_time_us Transmission time in microseconds
 * @return 0 on success, negative on error
 */
int intel_tsn_configure_timed_tx(intel_avb_stream_class_t stream_class, uint64_t tx_time_us);

/**
 * Transmit a packet with precise timing (LAUNCHTIME)
 * @param stream_class AVB stream class
 * @param packet_data Pointer to packet data
 * @param packet_length Length of packet data
 * @param launch_time_ns Launch time in nanoseconds
 * @return 0 on success, negative error code on failure
 */
int intel_tsn_transmit_timed_packet(intel_avb_stream_class_t stream_class, 
                                   const void* packet_data, size_t packet_length, 
                                   uint64_t launch_time_ns);

/**
 * Get TSN capabilities of the Intel adapter
 * @return Capability bitmask
 */
uint32_t intel_tsn_get_capabilities(void);

/**
 * Cleanup Intel TSN integration
 */
void intel_tsn_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif /* INTEL_TSN_INTEGRATION_H */
