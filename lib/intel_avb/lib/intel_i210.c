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

/* I210 specific register definitions */
#define I210_REG_TSYNCRXCTL      0x0B620  /* Rx Time Sync Control */
#define I210_REG_TSYNCTXCTL      0x0B614  /* Tx Time Sync Control */
#define I210_REG_RXSTMPL         0x0B624  /* Rx Time Stamp Low */
#define I210_REG_RXSTMPH         0x0B628  /* Rx Time Stamp High */
#define I210_REG_TXSTMPL         0x0B618  /* Tx Time Stamp Low */
#define I210_REG_TXSTMPH         0x0B61C  /* Tx Time Stamp High */

/* Time sync control register bits */
#define I210_TSYNCRXCTL_ENABLED  (1 << 4)   /* Rx timestamping enabled */
#define I210_TSYNCTXCTL_ENABLED  (1 << 4)   /* Tx timestamping enabled */

/* I210 private data structure */
struct i210_private {
    uint8_t timestamping_enabled;
    uint8_t rx_ts_enabled;
    uint8_t tx_ts_enabled;
    uint64_t last_rx_timestamp;
    uint64_t last_tx_timestamp;
};

/**
 * @brief Read register from I210 device
 */
static int i210_read_reg(struct intel_private *priv, uint32_t offset, uint32_t *value)
{
    if (!priv || !priv->mmio_base || !value) {
        return -EINVAL;
    }
    
    /* Memory-mapped I/O read */
    *value = *((volatile uint32_t *)((char *)priv->mmio_base + offset));
    
    return 0;
}

/**
 * @brief Write register to I210 device
 */
static int i210_write_reg(struct intel_private *priv, uint32_t offset, uint32_t value)
{
    if (!priv || !priv->mmio_base) {
        return -EINVAL;
    }
    
    /* Memory-mapped I/O write */
    *((volatile uint32_t *)((char *)priv->mmio_base + offset)) = value;
    
    return 0;
}

/**
 * @brief Get system time from I210 device
 */
static int i210_get_systime(struct intel_private *priv, uint64_t *systime)
{
    uint32_t systiml, systimh;
    int ret;
    
    if (!priv || !systime) {
        return -EINVAL;
    }
    
    /* Read high part first to latch the time */
    ret = i210_read_reg(priv, INTEL_REG_SYSTIMH, &systimh);
    if (ret < 0) {
        return ret;
    }
    
    ret = i210_read_reg(priv, INTEL_REG_SYSTIML, &systiml);
    if (ret < 0) {
        return ret;
    }
    
    *systime = ((uint64_t)systimh << 32) | systiml;
    
    return 0;
}

/**
 * @brief Set system time on I210 device
 */
static int i210_set_systime(struct intel_private *priv, uint64_t systime)
{
    uint32_t systiml, systimh;
    int ret;
    
    if (!priv) {
        return -EINVAL;
    }
    
    systiml = (uint32_t)(systime & 0xFFFFFFFF);
    systimh = (uint32_t)(systime >> 32);
    
    /* Write low part first */
    ret = i210_write_reg(priv, INTEL_REG_SYSTIML, systiml);
    if (ret < 0) {
        return ret;
    }
    
    /* Write high part to commit the time */
    ret = i210_write_reg(priv, INTEL_REG_SYSTIMH, systimh);
    if (ret < 0) {
        return ret;
    }
    
    return 0;
}

/**
 * @brief Adjust system time frequency on I210 device
 */
static int i210_adjust_systime(struct intel_private *priv, int32_t ppb)
{
    uint32_t timinca;
    uint64_t incvalue;
    int ret;
    
    if (!priv) {
        return -EINVAL;
    }
    
    /* Calculate increment value based on PPB adjustment */
    /* Base increment for I210 depends on link speed */
    /* For 1Gbps, typically 8ns increment */
    incvalue = 8000000000ULL; /* 8ns in picoseconds */
    
    /* Adjust for PPB */
    if (ppb != 0) {
        int64_t adj = (int64_t)incvalue * ppb / 1000000000LL;
        incvalue += adj;
    }
    
    /* Convert to TIMINCA register format */
    timinca = (uint32_t)(incvalue & 0xFFFFFFFF);
    
    ret = i210_write_reg(priv, INTEL_REG_TIMINCA, timinca);
    if (ret < 0) {
        return ret;
    }
    
    return 0;
}

/**
 * @brief Enable timestamping on I210
 */
static int i210_enable_timestamping(struct intel_private *priv)
{
    struct i210_private *i210_priv;
    uint32_t tsyncrxctl, tsynctxctl;
    int ret;
    
    if (!priv || !priv->device_private) {
        return -EINVAL;
    }
    
    i210_priv = (struct i210_private *)priv->device_private;
    
    /* Enable Rx timestamping */
    ret = i210_read_reg(priv, I210_REG_TSYNCRXCTL, &tsyncrxctl);
    if (ret < 0) {
        return ret;
    }
    
    tsyncrxctl |= I210_TSYNCRXCTL_ENABLED;
    ret = i210_write_reg(priv, I210_REG_TSYNCRXCTL, tsyncrxctl);
    if (ret < 0) {
        return ret;
    }
    
    /* Enable Tx timestamping */
    ret = i210_read_reg(priv, I210_REG_TSYNCTXCTL, &tsynctxctl);
    if (ret < 0) {
        return ret;
    }
    
    tsynctxctl |= I210_TSYNCTXCTL_ENABLED;
    ret = i210_write_reg(priv, I210_REG_TSYNCTXCTL, tsynctxctl);
    if (ret < 0) {
        return ret;
    }
    
    i210_priv->rx_ts_enabled = 1;
    i210_priv->tx_ts_enabled = 1;
    i210_priv->timestamping_enabled = 1;
    
    return 0;
}

/**
 * @brief Get last Rx timestamp from I210
 */
static int i210_get_rx_timestamp(struct intel_private *priv, uint64_t *timestamp)
{
    struct i210_private *i210_priv;
    uint32_t rxstmpl, rxstmph;
    int ret;
    
    if (!priv || !priv->device_private || !timestamp) {
        return -EINVAL;
    }
    
    i210_priv = (struct i210_private *)priv->device_private;
    
    if (!i210_priv->rx_ts_enabled) {
        return -ENOTSUP;
    }
    
    /* Read timestamp registers */
    ret = i210_read_reg(priv, I210_REG_RXSTMPH, &rxstmph);
    if (ret < 0) {
        return ret;
    }
    
    ret = i210_read_reg(priv, I210_REG_RXSTMPL, &rxstmpl);
    if (ret < 0) {
        return ret;
    }
    
    *timestamp = ((uint64_t)rxstmph << 32) | rxstmpl;
    i210_priv->last_rx_timestamp = *timestamp;
    
    return 0;
}

/**
 * @brief Get last Tx timestamp from I210
 */
static int i210_get_tx_timestamp(struct intel_private *priv, uint64_t *timestamp)
{
    struct i210_private *i210_priv;
    uint32_t txstmpl, txstmph;
    int ret;
    
    if (!priv || !priv->device_private || !timestamp) {
        return -EINVAL;
    }
    
    i210_priv = (struct i210_private *)priv->device_private;
    
    if (!i210_priv->tx_ts_enabled) {
        return -ENOTSUP;
    }
    
    /* Read timestamp registers */
    ret = i210_read_reg(priv, I210_REG_TXSTMPH, &txstmph);
    if (ret < 0) {
        return ret;
    }
    
    ret = i210_read_reg(priv, I210_REG_TXSTMPL, &txstmpl);
    if (ret < 0) {
        return ret;
    }
    
    *timestamp = ((uint64_t)txstmph << 32) | txstmpl;
    i210_priv->last_tx_timestamp = *timestamp;
    
    return 0;
}

/**
 * @brief Initialize I210 device
 */
int intel_i210_init(device_t *dev)
{
    struct intel_private *priv;
    struct i210_private *i210_priv;
    uint32_t ctrl;
    
    if (!dev || !dev->private_data) {
        return -EINVAL;
    }
    
    priv = (struct intel_private *)dev->private_data;
    
    /* Allocate I210-specific private data */
    i210_priv = calloc(1, sizeof(struct i210_private));
    if (!i210_priv) {
        return -ENOMEM;
    }
    
    priv->device_private = i210_priv;
    
    /* Set up function pointers */
    priv->read_reg = i210_read_reg;
    priv->write_reg = i210_write_reg;
    priv->get_systime = i210_get_systime;
    priv->set_systime = i210_set_systime;
    priv->adjust_systime = i210_adjust_systime;
    
    /* Initialize I210-specific settings */
    i210_priv->timestamping_enabled = 0;
    i210_priv->rx_ts_enabled = 0;
    i210_priv->tx_ts_enabled = 0;
    i210_priv->last_rx_timestamp = 0;
    i210_priv->last_tx_timestamp = 0;
    
    /* TODO: Map MMIO region for register access */
    /* This would need platform-specific implementation */
    
    /* Try to read control register to verify device access */
    if (priv->read_reg && priv->read_reg(priv, INTEL_REG_CTRL, &ctrl) == 0) {
        /* Device accessible, try to enable timestamping */
        i210_enable_timestamping(priv);
    }
    
    return 0;
}

/**
 * @brief Cleanup I210 device
 */
void intel_i210_cleanup(device_t *dev)
{
    struct intel_private *priv;
    struct i210_private *i210_priv;
    
    if (!dev || !dev->private_data) {
        return;
    }
    
    priv = (struct intel_private *)dev->private_data;
    
    if (priv->device_private) {
        i210_priv = (struct i210_private *)priv->device_private;
        
        /* Disable timestamping if it was enabled */
        if (i210_priv->timestamping_enabled && priv->write_reg) {
            uint32_t tsyncrxctl, tsynctxctl;
            
            /* Disable Rx timestamping */
            if (priv->read_reg(priv, I210_REG_TSYNCRXCTL, &tsyncrxctl) == 0) {
                tsyncrxctl &= ~I210_TSYNCRXCTL_ENABLED;
                priv->write_reg(priv, I210_REG_TSYNCRXCTL, tsyncrxctl);
            }
            
            /* Disable Tx timestamping */
            if (priv->read_reg(priv, I210_REG_TSYNCTXCTL, &tsynctxctl) == 0) {
                tsynctxctl &= ~I210_TSYNCTXCTL_ENABLED;
                priv->write_reg(priv, I210_REG_TSYNCTXCTL, tsynctxctl);
            }
        }
        
        free(priv->device_private);
        priv->device_private = NULL;
    }
    
    /* TODO: Unmap MMIO region */
}
