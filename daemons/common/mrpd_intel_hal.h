/*******************************************************************************
Copyright (c) 2025, Intel Corporation
All rights reserved.

MRPD Daemon Intel HAL Integration
Enhanced Intel adapter support for MRP (Multiple Registration Protocol) daemon

This module provides Intel HAL integration for precise timing in MRP protocols
including MVRP, MMRP, and MSRP.
*******************************************************************************/

#ifndef MRPD_INTEL_HAL_H
#define MRPD_INTEL_HAL_H

#include <stdint.h>
#include <stdbool.h>

#ifdef OPENAVNU_BUILD_INTEL_HAL
#include "intel_ethernet_hal.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

// MRPD Intel HAL integration structure
typedef struct {
    bool available;
    bool initialized;
    char interface_name[256];
    uint8_t mac_address[6];
    uint64_t last_timestamp;
    uint32_t capabilities;
    // MRP-specific timing statistics
    uint64_t registration_timestamps[64]; // Rolling buffer
    int timestamp_index;
    uint64_t min_interval_ns;
    uint64_t max_interval_ns;
#ifdef OPENAVNU_BUILD_INTEL_HAL
    intel_device_t *device;
#endif
} mrpd_intel_hal_t;

// MRPD Intel HAL API
int mrpd_intel_hal_init(const char *interface_name);
void mrpd_intel_hal_cleanup(void);
bool mrpd_intel_hal_is_available(void);
uint64_t mrpd_intel_hal_get_timestamp(void);
int mrpd_intel_hal_get_interface_info(uint8_t *mac_address, uint32_t *capabilities);
const char *mrpd_intel_hal_get_interface_name(void);

// MRP-specific Intel HAL functions
uint64_t mrpd_intel_hal_get_registration_timestamp(void);
uint64_t mrpd_intel_hal_get_leave_timestamp(void);
bool mrpd_intel_hal_supports_precision_timing(void);
int mrpd_intel_hal_get_timing_precision_ns(void);
void mrpd_intel_hal_record_message_timing(uint64_t timestamp);
uint64_t mrpd_intel_hal_get_average_interval(void);

// Protocol-specific timing functions
uint64_t mrpd_intel_hal_get_mvrp_timestamp(void);
uint64_t mrpd_intel_hal_get_mmrp_timestamp(void);
uint64_t mrpd_intel_hal_get_msrp_timestamp(void);

// Fallback timestamp function
uint64_t mrpd_get_fallback_timestamp(void);

#ifdef __cplusplus
}
#endif

#endif /* MRPD_INTEL_HAL_H */
