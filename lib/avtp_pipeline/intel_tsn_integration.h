/******************************************************************************

  Copyright (c) 2025, Intel Corporation
  All rights reserved.

  AVTP Pipeline Intel TSN Integration Header
  
  This header defines the interface for Intel Time-Sensitive Networking (TSN)
  integration with the AVTP pipeline, providing access to:
  - Time-Aware Shaper (TAS) - 802.1Qbv
  - Frame Preemption (FP) - 802.1Qbu  
  - Timed Transmission (LAUNCHTIME)
  - AVB queue bandwidth reservation

******************************************************************************/

#ifndef INTEL_TSN_INTEGRATION_H
#define INTEL_TSN_INTEGRATION_H

#include <stdint.h>
#include <stdbool.h>
#include "../../thirdparty/intel-ethernet-hal/include/intel_ethernet_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief AVB stream classes as defined in IEEE 802.1BA
 */
typedef enum {
    INTEL_AVB_CLASS_A = 0,  ///< Class A: 125µs cycle, ≤2ms latency
    INTEL_AVB_CLASS_B = 1   ///< Class B: 250µs cycle, ≤50ms latency
} intel_avb_stream_class_t;

/**
 * @brief TSN integration status and capabilities
 */
typedef struct {
    bool initialized;              ///< TSN integration initialized
    bool tas_enabled;              ///< Time-Aware Shaper enabled
    bool fp_enabled;               ///< Frame Preemption enabled  
    bool timed_tx_enabled;         ///< Timed transmission enabled
    uint32_t reserved_bandwidth_bps; ///< Reserved bandwidth in bps
    
    // Hardware capabilities
    bool tas_capable;              ///< Hardware supports TAS
    bool fp_capable;               ///< Hardware supports Frame Preemption
    bool hardware_timestamping;    ///< Hardware timestamping available
} intel_tsn_status_t;

/**
 * @brief Initialize Intel TSN integration for AVTP pipeline
 * 
 * This function initializes the TSN integration context and detects
 * available Intel hardware with TSN capabilities.
 * 
 * @param interface_name Network interface name (e.g., "Ethernet")
 * @return 0 on success, negative error code on failure
 */
int intel_tsn_init(const char *interface_name);

/**
 * @brief Configure Time-Aware Shaper for AVB streaming
 * 
 * Configures TAS with deterministic scheduling for AVB Class A and B traffic.
 * Uses IEEE 802.1BA recommended cycle times and gate schedules.
 * 
 * @param stream_class AVB stream class (A or B)
 * @param bandwidth_mbps Required bandwidth in Mbps
 * @return 0 on success, negative error code on failure
 */
int intel_tsn_configure_tas(intel_avb_stream_class_t stream_class, uint32_t bandwidth_mbps);

/**
 * @brief Configure Frame Preemption for low-latency AVB streams
 * 
 * Sets up Frame Preemption to minimize latency for time-critical AVB traffic
 * by allowing preemption of lower-priority frames.
 * 
 * @param enable_preemption Enable/disable frame preemption
 * @return 0 on success, negative error code on failure
 */
int intel_tsn_configure_frame_preemption(bool enable_preemption);

/**
 * @brief Transmit AVTP packet with precise timing (LAUNCHTIME)
 * 
 * Transmits an AVTP packet using Intel's timed transmission feature
 * for precise delivery scheduling.
 * 
 * @param packet_data Pointer to packet data
 * @param packet_len Length of packet data
 * @param stream_class AVB stream class (determines queue)
 * @param launch_time_ns Precise transmission time in nanoseconds
 * @return 0 on success, negative error code on failure
 */
int intel_tsn_transmit_timed_packet(void *packet_data, uint32_t packet_len,
                                   intel_avb_stream_class_t stream_class,
                                   uint64_t launch_time_ns);

/**
 * @brief Get TSN integration status and capabilities
 * 
 * Returns current status of TSN features and hardware capabilities.
 * 
 * @param status Pointer to status structure to fill
 * @return 0 on success, negative error code on failure
 */
int intel_tsn_get_status(intel_tsn_status_t *status);

/**
 * @brief Cleanup TSN integration resources
 * 
 * Cleans up all resources allocated for TSN integration.
 */
void intel_tsn_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif /* INTEL_TSN_INTEGRATION_H */
