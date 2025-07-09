/******************************************************************************

  Copyright (c) 2025, Intel Corporation
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

   3. Neither the name of the Intel Corporation nor the names of its
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.

******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "intel.h"
#include "intel_private.h"

/* Device capability mapping table */
struct intel_device_info {
    uint16_t device_id;
    intel_device_type_t device_type;
    uint32_t capabilities;
    const char *name;
    const char *description;
};

static const struct intel_device_info intel_devices[] = {
    /* I210 Series */
    { 0x1533, INTEL_DEVICE_I210, 
      INTEL_CAP_BASIC_1588 | INTEL_CAP_MMIO,
      "I210", "Intel I210 Gigabit Network Connection" },
    { 0x1536, INTEL_DEVICE_I210, 
      INTEL_CAP_BASIC_1588 | INTEL_CAP_MMIO,
      "I210-T1", "Intel I210-T1 Gigabit Network Connection" },
    { 0x1537, INTEL_DEVICE_I210, 
      INTEL_CAP_BASIC_1588 | INTEL_CAP_MMIO,
      "I210-IS", "Intel I210-IS Gigabit Network Connection" },
    
    /* I219 Series */
    { 0x15b7, INTEL_DEVICE_I219, 
      INTEL_CAP_BASIC_1588 | INTEL_CAP_MDIO,
      "I219-LM", "Intel I219-LM Gigabit Network Connection" },
    { 0x15b8, INTEL_DEVICE_I219, 
      INTEL_CAP_BASIC_1588 | INTEL_CAP_MDIO,
      "I219-V", "Intel I219-V Gigabit Network Connection" },
    { 0x15d6, INTEL_DEVICE_I219, 
      INTEL_CAP_BASIC_1588 | INTEL_CAP_MDIO,
      "I219-V", "Intel I219-V Gigabit Network Connection" },
    { 0x15d7, INTEL_DEVICE_I219, 
      INTEL_CAP_BASIC_1588 | INTEL_CAP_MDIO,
      "I219-LM", "Intel I219-LM Gigabit Network Connection" },
    { 0x15d8, INTEL_DEVICE_I219, 
      INTEL_CAP_BASIC_1588 | INTEL_CAP_MDIO,
      "I219-V", "Intel I219-V Gigabit Network Connection" },
    
    /* I225 Series */
    { 0x15f2, INTEL_DEVICE_I225, 
      INTEL_CAP_BASIC_1588 | INTEL_CAP_ENHANCED_TS | INTEL_CAP_TSN_TAS | 
      INTEL_CAP_TSN_FP | INTEL_CAP_PCIe_PTM | INTEL_CAP_2_5G | INTEL_CAP_MMIO,
      "I225-LM", "Intel I225-LM 2.5 Gigabit Network Connection" },
    { 0x15f3, INTEL_DEVICE_I225, 
      INTEL_CAP_BASIC_1588 | INTEL_CAP_ENHANCED_TS | INTEL_CAP_TSN_TAS | 
      INTEL_CAP_TSN_FP | INTEL_CAP_PCIe_PTM | INTEL_CAP_2_5G | INTEL_CAP_MMIO,
      "I225-V", "Intel I225-V 2.5 Gigabit Network Connection" },
    
    /* I226 Series */
    { 0x125b, INTEL_DEVICE_I226, 
      INTEL_CAP_BASIC_1588 | INTEL_CAP_ENHANCED_TS | INTEL_CAP_TSN_TAS | 
      INTEL_CAP_TSN_FP | INTEL_CAP_PCIe_PTM | INTEL_CAP_2_5G | INTEL_CAP_MMIO,
      "I226-LM", "Intel I226-LM 2.5 Gigabit Network Connection" },
    { 0x125c, INTEL_DEVICE_I226, 
      INTEL_CAP_BASIC_1588 | INTEL_CAP_ENHANCED_TS | INTEL_CAP_TSN_TAS | 
      INTEL_CAP_TSN_FP | INTEL_CAP_PCIe_PTM | INTEL_CAP_2_5G | INTEL_CAP_MMIO,
      "I226-V", "Intel I226-V 2.5 Gigabit Network Connection" },
    
    /* Terminator */
    { 0, INTEL_DEVICE_UNKNOWN, 0, NULL, NULL }
};

/**
 * @brief Identify Intel device type and capabilities from PCI device ID
 */
static const struct intel_device_info *intel_identify_device(uint16_t device_id)
{
    const struct intel_device_info *info;
    
    for (info = intel_devices; info->device_id != 0; info++) {
        if (info->device_id == device_id) {
            return info;
        }
    }
    
    return NULL;
}

/**
 * @brief Probe Intel device to check if it's supported
 */
int intel_probe(device_t *dev)
{
    const struct intel_device_info *info;
    
    if (!dev) {
        return -EINVAL;
    }
    
    /* Check vendor ID */
    if (dev->pci_vendor_id != INTEL_VENDOR_ID) {
        return -ENODEV;
    }
    
    /* Identify device */
    info = intel_identify_device(dev->pci_device_id);
    if (!info) {
        return -ENODEV;
    }
    
    /* Set device type and capabilities */
    dev->device_type = info->device_type;
    dev->capabilities = info->capabilities;
    
    return 0;
}

/**
 * @brief Get device capabilities
 */
uint32_t intel_get_capabilities(device_t *dev)
{
    if (!dev) {
        return 0;
    }
    
    return dev->capabilities;
}

/**
 * @brief Get device name string
 */
const char *intel_get_device_name(device_t *dev)
{
    const struct intel_device_info *info;
    
    if (!dev) {
        return "Unknown";
    }
    
    info = intel_identify_device(dev->pci_device_id);
    if (!info) {
        return "Unknown Intel Device";
    }
    
    return info->name;
}

/**
 * @brief Get detailed device information
 */
int intel_get_device_info(device_t *dev, char *info_buffer, size_t buffer_size)
{
    const struct intel_device_info *info;
    int ret;
    
    if (!dev || !info_buffer) {
        return -EINVAL;
    }
    
    info = intel_identify_device(dev->pci_device_id);
    if (!info) {
        ret = snprintf(info_buffer, buffer_size, 
                      "Unknown Intel Device (ID: 0x%04x)", dev->pci_device_id);
    } else {
        ret = snprintf(info_buffer, buffer_size, 
                      "%s - %s (ID: 0x%04x, Caps: 0x%08x)", 
                      info->name, info->description, 
                      dev->pci_device_id, dev->capabilities);
    }
    
    return (ret >= (int)buffer_size) ? -ENOSPC : 0;
}

/**
 * @brief Check if device supports a specific capability
 */
int intel_has_capability(device_t *dev, uint32_t capability)
{
    if (!dev) {
        return 0;
    }
    
    return (dev->capabilities & capability) != 0;
}

/**
 * @brief Common initialization for all Intel devices
 */
int intel_common_init(device_t *dev)
{
    struct intel_private *priv;
    
    if (!dev) {
        return -EINVAL;
    }
    
    /* Allocate private data structure */
    priv = calloc(1, sizeof(struct intel_private));
    if (!priv) {
        return -ENOMEM;
    }
    
    /* Initialize private data */
    priv->device_type = dev->device_type;
    priv->capabilities = dev->capabilities;
    priv->initialized = 1;
    
    dev->private_data = priv;
    
    return 0;
}

/**
 * @brief Common cleanup for all Intel devices
 */
void intel_common_cleanup(device_t *dev)
{
    struct intel_private *priv;
    
    if (!dev || !dev->private_data) {
        return;
    }
    
    priv = (struct intel_private *)dev->private_data;
    priv->initialized = 0;
    
    free(dev->private_data);
    dev->private_data = NULL;
}

/**
 * @brief Validate packet for transmission
 */
int intel_validate_packet(struct intel_packet *packet)
{
    if (!packet) {
        return -EINVAL;
    }
    
    if (!packet->vaddr || packet->len == 0) {
        return -EINVAL;
    }
    
    /* Check for reasonable packet size */
    if (packet->len > 9000) { /* Jumbo frame limit */
        return -EMSGSIZE;
    }
    
    return 0;
}
