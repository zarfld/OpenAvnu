/******************************************************************************

  Copyright (c) 2025, Intel Corporation
  All rights reserved.

  Intel AVB Integration Header for Common AVB Layer

******************************************************************************/

#ifndef __AVB_INTEL_H__
#define __AVB_INTEL_H__

#include <intel.h>

#define INTEL_BIND_NAMESZ 24

/**
 * @brief Connect to Intel PCI device
 * 
 * This function provides integration with the common AVB layer,
 * similar to the existing pci_connect function for IGB devices.
 * 
 * @param intel_dev Pointer to Intel device structure
 * @return 0 on success, negative error code on failure
 */
int pci_connect_intel(device_t *intel_dev);

/**
 * @brief Intel device initialization helper
 * 
 * @param dev_path Path to device (platform-specific)
 * @param intel_dev Device structure to initialize
 * @return 0 on success, negative error code on failure
 */
int intel_avb_init(char *dev_path, device_t *intel_dev);

/**
 * @brief Intel device cleanup helper
 * 
 * @param intel_dev Device structure to cleanup
 */
void intel_avb_cleanup(device_t *intel_dev);

#endif /* __AVB_INTEL_H__ */
