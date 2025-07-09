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

#ifndef _WIN32
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#endif

#include "intel.h"
#include "intel_private.h"

/**
 * @brief Attach to an Intel device
 */
int intel_attach(char *dev_path, device_t *pdev)
{
    struct intel_private *priv;
    int ret;
    
    if (!dev_path || !pdev) {
        return -EINVAL;
    }
    
    /* Probe device first */
    ret = intel_probe(pdev);
    if (ret < 0) {
        return ret;
    }
    
    /* Initialize common structures */
    ret = intel_common_init(pdev);
    if (ret < 0) {
        return ret;
    }
    
    priv = (struct intel_private *)pdev->private_data;
    
    /* Device-specific initialization */
    switch (pdev->device_type) {
    case INTEL_DEVICE_I210:
        ret = intel_i210_init(pdev);
        break;
    case INTEL_DEVICE_I219:
        ret = intel_i219_init(pdev);
        break;
    case INTEL_DEVICE_I225:
        ret = intel_i225_init(pdev);
        break;
    case INTEL_DEVICE_I226:
        ret = intel_i226_init(pdev);
        break;
    default:
        ret = -ENODEV;
        break;
    }
    
    if (ret < 0) {
        intel_common_cleanup(pdev);
        return ret;
    }
    
    return 0;
}

/**
 * @brief Initialize Intel device for receive operations
 */
int intel_attach_rx(device_t *pdev)
{
    struct intel_private *priv;
    
    if (!pdev || !pdev->private_data) {
        return -EINVAL;
    }
    
    priv = (struct intel_private *)pdev->private_data;
    
    /* Device-specific RX initialization would go here */
    /* For now, just mark as successful */
    
    return 0;
}

/**
 * @brief Initialize Intel device for transmit operations
 */
int intel_attach_tx(device_t *pdev)
{
    struct intel_private *priv;
    
    if (!pdev || !pdev->private_data) {
        return -EINVAL;
    }
    
    priv = (struct intel_private *)pdev->private_data;
    
    /* Device-specific TX initialization would go here */
    /* For now, just mark as successful */
    
    return 0;
}

/**
 * @brief Detach from Intel device
 */
int intel_detach(device_t *dev)
{
    if (!dev || !dev->private_data) {
        return -EINVAL;
    }
    
    /* Device-specific cleanup */
    switch (dev->device_type) {
    case INTEL_DEVICE_I210:
        intel_i210_cleanup(dev);
        break;
    case INTEL_DEVICE_I219:
        intel_i219_cleanup(dev);
        break;
    case INTEL_DEVICE_I225:
        intel_i225_cleanup(dev);
        break;
    case INTEL_DEVICE_I226:
        intel_i226_cleanup(dev);
        break;
    default:
        break;
    }
    
    /* Common cleanup */
    intel_common_cleanup(dev);
    
    return 0;
}

/**
 * @brief Initialize Intel device
 */
int intel_init(device_t *dev)
{
    struct intel_private *priv;
    uint32_t ctrl;
    
    if (!dev || !dev->private_data) {
        return -EINVAL;
    }
    
    priv = (struct intel_private *)dev->private_data;
    
    /* Read control register to verify device is accessible */
    if (priv->read_reg && priv->read_reg(priv, INTEL_REG_CTRL, &ctrl) == 0) {
        /* Device is accessible via register reads */
        return 0;
    }
    
    return -ENODEV;
}

/**
 * @brief Allocate DMA memory page
 */
int intel_dma_malloc_page(device_t *dev, struct intel_dma_alloc *page)
{
    if (!dev || !page) {
        return -EINVAL;
    }
    
    /* This would need to be implemented with platform-specific DMA allocation */
    /* For now, just allocate regular memory */
    page->mmap_size = 4096; /* One page */
    page->dma_vaddr = malloc(page->mmap_size);
    if (!page->dma_vaddr) {
        return -ENOMEM;
    }
    
    /* In a real implementation, this would be the physical/DMA address */
    page->dma_paddr = (uint64_t)(uintptr_t)page->dma_vaddr;
    
    return 0;
}

/**
 * @brief Free DMA memory page
 */
void intel_dma_free_page(device_t *dev, struct intel_dma_alloc *page)
{
    if (!dev || !page || !page->dma_vaddr) {
        return;
    }
    
    free(page->dma_vaddr);
    memset(page, 0, sizeof(*page));
}

/**
 * @brief Transmit packet
 */
int intel_xmit(device_t *dev, unsigned int queue_index, struct intel_packet *packet)
{
    struct intel_private *priv;
    int ret;
    
    if (!dev || !dev->private_data || !packet) {
        return -EINVAL;
    }
    
    priv = (struct intel_private *)dev->private_data;
    
    /* Validate packet */
    ret = intel_validate_packet(packet);
    if (ret < 0) {
        return ret;
    }
    
    /* Device-specific transmission would be implemented here */
    /* For now, just return success */
    
    return 0;
}

/**
 * @brief Refresh receive buffers
 */
int intel_refresh_buffers(device_t *dev, uint32_t idx,
                         struct intel_packet **rxbuf_packets, uint32_t num_bufs)
{
    if (!dev || !rxbuf_packets) {
        return -EINVAL;
    }
    
    /* Device-specific buffer refresh would be implemented here */
    /* For now, just return success */
    
    return 0;
}

/**
 * @brief Receive packets
 */
int intel_receive(device_t *dev, unsigned int queue_index, 
                 struct intel_packet **received_packets, uint32_t *count)
{
    if (!dev || !received_packets || !count) {
        return -EINVAL;
    }
    
    /* Device-specific packet reception would be implemented here */
    /* For now, indicate no packets received */
    *count = 0;
    
    return 0;
}

/**
 * @brief Clean up transmitted packets
 */
void intel_clean(device_t *dev, struct intel_packet **cleaned_packets)
{
    if (!dev || !cleaned_packets) {
        return;
    }
    
    /* Device-specific cleanup would be implemented here */
}

/**
 * @brief Read register value
 */
int intel_read_reg(device_t *dev, uint32_t offset, uint32_t *value)
{
    struct intel_private *priv;
    
    if (!dev || !dev->private_data || !value) {
        return -EINVAL;
    }
    
    priv = (struct intel_private *)dev->private_data;
    
    if (!priv->read_reg) {
        return -ENOTSUP;
    }
    
    return priv->read_reg(priv, offset, value);
}

/**
 * @brief Write register value
 */
int intel_write_reg(device_t *dev, uint32_t offset, uint32_t value)
{
    struct intel_private *priv;
    
    if (!dev || !dev->private_data) {
        return -EINVAL;
    }
    
    priv = (struct intel_private *)dev->private_data;
    
    if (!priv->write_reg) {
        return -ENOTSUP;
    }
    
    return priv->write_reg(priv, offset, value);
}

/**
 * @brief Get wallclock time
 */
int intel_get_wallclock(device_t *dev, uint64_t *curtime, uint64_t *rdtsc)
{
    if (!dev || !curtime) {
        return -EINVAL;
    }
    
    /* This would need platform-specific implementation */
    /* For now, return current system time */
    *curtime = 0; /* Would need actual implementation */
    if (rdtsc) {
        *rdtsc = 0; /* Would need actual TSC read */
    }
    
    return 0;
}

/**
 * @brief Get system time from device
 */
int intel_get_systime(device_t *dev, uint64_t *systime)
{
    struct intel_private *priv;
    
    if (!dev || !dev->private_data || !systime) {
        return -EINVAL;
    }
    
    priv = (struct intel_private *)dev->private_data;
    
    if (!priv->get_systime) {
        return -ENOTSUP;
    }
    
    return priv->get_systime(priv, systime);
}

/**
 * @brief Set system time on device
 */
int intel_set_systime(device_t *dev, uint64_t systime)
{
    struct intel_private *priv;
    
    if (!dev || !dev->private_data) {
        return -EINVAL;
    }
    
    priv = (struct intel_private *)dev->private_data;
    
    if (!priv->set_systime) {
        return -ENOTSUP;
    }
    
    return priv->set_systime(priv, systime);
}

/**
 * @brief Adjust system time frequency
 */
int intel_adjust_systime(device_t *dev, int32_t ppb)
{
    struct intel_private *priv;
    
    if (!dev || !dev->private_data) {
        return -EINVAL;
    }
    
    priv = (struct intel_private *)dev->private_data;
    
    if (!priv->adjust_systime) {
        return -ENOTSUP;
    }
    
    return priv->adjust_systime(priv, ppb);
}

/**
 * @brief Get time using clock ID
 */
int intel_gettime(device_t *dev, clockid_t clk_id, uint64_t *curtime,
                 struct timespec *system_time)
{
    if (!dev || !curtime) {
        return -EINVAL;
    }
    
    /* This would need to map clock IDs to appropriate time sources */
    /* For now, just delegate to get_systime */
    return intel_get_systime(dev, curtime);
}
