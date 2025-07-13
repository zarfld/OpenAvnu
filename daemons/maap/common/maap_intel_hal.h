/*******************************************************************************
Copyright (c) 2025, Intel Corporation
All rights reserved.

MAAP Daemon Intel HAL Integration
Modern Intel adapter support for MAAP (MAC Address Allocation Protocol)

This module provides Intel HAL integration for enhanced timestamping and 
Intel adapter capabilities in the MAAP daemon.
*******************************************************************************/

#ifndef MAAP_INTEL_HAL_H
#define MAAP_INTEL_HAL_H

#include <stdint.h>
#include <stdbool.h>

#ifdef OPENAVNU_BUILD_INTEL_HAL
#include "intel_ethernet_hal.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

// MAAP Intel HAL integration structure
typedef struct {
    bool available;
    bool initialized;
    char interface_name[256];
    uint8_t mac_address[6];
    uint64_t last_timestamp;
    uint32_t capabilities;
#ifdef OPENAVNU_BUILD_INTEL_HAL
    intel_device_t *device;
#endif
} maap_intel_hal_t;

// MAAP Intel HAL API
int maap_intel_hal_init(const char *interface_name);
void maap_intel_hal_cleanup(void);
bool maap_intel_hal_is_available(void);
uint64_t maap_intel_hal_get_timestamp(void);
int maap_intel_hal_get_interface_info(uint8_t *mac_address, uint32_t *capabilities);
const char *maap_intel_hal_get_interface_name(void);

// MAAP-specific Intel HAL functions
uint64_t maap_intel_hal_get_allocation_timestamp(void);
bool maap_intel_hal_supports_precision_timing(void);
int maap_intel_hal_get_timing_precision_ns(void);

// Fallback timestamp function
uint64_t maap_get_fallback_timestamp(void);

#ifdef __cplusplus
}
#endif

#endif /* MAAP_INTEL_HAL_H */
