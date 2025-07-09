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

/* I225/I226 specific register definitions */
#define I225_REG_TAS_CONFIG_0    0x8604  /* Time Aware Shaper Config 0 */
#define I225_REG_TAS_CONFIG_1    0x8608  /* Time Aware Shaper Config 1 */
#define I225_REG_TAS_GATE_LIST   0x8610  /* Gate List Base */
#define I225_REG_FP_CONFIG       0x8700  /* Frame Preemption Config */
#define I225_REG_FP_STATUS       0x8704  /* Frame Preemption Status */
#define I225_REG_PTM_CONFIG      0x8800  /* PTM Configuration */
#define I225_REG_PTM_STATUS      0x8804  /* PTM Status */

/* TAS Control Register bits */
#define I225_TAS_CTRL_EN         (1 << 0)   /* TAS Enable */
#define I225_TAS_CTRL_GATE_LIST  (1 << 1)   /* Gate List Ready */
#define I225_TAS_CTRL_BASE_TIME  (1 << 2)   /* Base Time Valid */

/* Frame Preemption Control bits */
#define I225_FP_CTRL_EN          (1 << 0)   /* Frame Preemption Enable */
#define I225_FP_CTRL_VERIFY_DIS  (1 << 1)   /* Disable Verification */

/* PTM Control bits */
#define I225_PTM_CTRL_EN         (1 << 0)   /* PTM Enable */
#define I225_PTM_CTRL_AUTO_UPD   (1 << 1)   /* Auto Update */

/* I225/I226 private data structure */
struct i225_private {
    /* TAS configuration */
    uint8_t tas_enabled;
    uint64_t tas_base_time;
    uint32_t tas_cycle_time;
    
    /* Frame Preemption configuration */
    uint8_t fp_enabled;
    uint8_t fp_verify_disabled;
    uint32_t fp_min_fragment_size;
    
    /* PTM configuration */
    uint8_t ptm_enabled;
    uint32_t ptm_granularity;
};

/**
 * @brief Read register from I225/I226 device
 */
static int i225_read_reg(struct intel_private *priv, uint32_t offset, uint32_t *value)
{
    if (!priv || !priv->mmio_base || !value) {
        return -EINVAL;
    }
    
    /* Memory-mapped I/O read */
    *value = *((volatile uint32_t *)((char *)priv->mmio_base + offset));
    
    return 0;
}

/**
 * @brief Write register to I225/I226 device
 */
static int i225_write_reg(struct intel_private *priv, uint32_t offset, uint32_t value)
{
    if (!priv || !priv->mmio_base) {
        return -EINVAL;
    }
    
    /* Memory-mapped I/O write */
    *((volatile uint32_t *)((char *)priv->mmio_base + offset)) = value;
    
    return 0;
}

/**
 * @brief Get system time from I225/I226 device
 */
static int i225_get_systime(struct intel_private *priv, uint64_t *systime)
{
    uint32_t systiml, systimh;
    int ret;
    
    if (!priv || !systime) {
        return -EINVAL;
    }
    
    /* Read high part first to latch the time */
    ret = i225_read_reg(priv, INTEL_REG_SYSTIMH, &systimh);
    if (ret < 0) {
        return ret;
    }
    
    ret = i225_read_reg(priv, INTEL_REG_SYSTIML, &systiml);
    if (ret < 0) {
        return ret;
    }
    
    *systime = ((uint64_t)systimh << 32) | systiml;
    
    return 0;
}

/**
 * @brief Set system time on I225/I226 device
 */
static int i225_set_systime(struct intel_private *priv, uint64_t systime)
{
    uint32_t systiml, systimh;
    int ret;
    
    if (!priv) {
        return -EINVAL;
    }
    
    systiml = (uint32_t)(systime & 0xFFFFFFFF);
    systimh = (uint32_t)(systime >> 32);
    
    /* Write low part first */
    ret = i225_write_reg(priv, INTEL_REG_SYSTIML, systiml);
    if (ret < 0) {
        return ret;
    }
    
    /* Write high part to commit the time */
    ret = i225_write_reg(priv, INTEL_REG_SYSTIMH, systimh);
    if (ret < 0) {
        return ret;
    }
    
    return 0;
}

/**
 * @brief Adjust system time frequency on I225/I226 device
 */
static int i225_adjust_systime(struct intel_private *priv, int32_t ppb)
{
    uint32_t timinca;
    uint64_t incvalue;
    int ret;
    
    if (!priv) {
        return -EINVAL;
    }
    
    /* Calculate increment value based on PPB adjustment */
    /* Base increment for 2.5GHz clock is typically around 4ns */
    incvalue = 4000000000ULL; /* 4ns in picoseconds */
    
    /* Adjust for PPB */
    if (ppb != 0) {
        int64_t adj = (int64_t)incvalue * ppb / 1000000000LL;
        incvalue += adj;
    }
    
    /* Convert to TIMINCA register format */
    timinca = (uint32_t)(incvalue & 0xFFFFFFFF);
    
    ret = i225_write_reg(priv, INTEL_REG_TIMINCA, timinca);
    if (ret < 0) {
        return ret;
    }
    
    return 0;
}

/**
 * @brief Configure Time Aware Shaper on I225/I226
 */
static int i225_setup_tas(struct intel_private *priv, struct tsn_tas_config *config)
{
    struct i225_private *i225_priv;
    uint32_t tas_ctrl, tas_config;
    int ret;
    
    if (!priv || !priv->device_private || !config) {
        return -EINVAL;
    }
    
    i225_priv = (struct i225_private *)priv->device_private;
    
    /* Disable TAS first */
    ret = i225_read_reg(priv, INTEL_REG_TAS_CTRL, &tas_ctrl);
    if (ret < 0) {
        return ret;
    }
    
    tas_ctrl &= ~I225_TAS_CTRL_EN;
    ret = i225_write_reg(priv, INTEL_REG_TAS_CTRL, tas_ctrl);
    if (ret < 0) {
        return ret;
    }
    
    /* Configure base time */
    ret = i225_write_reg(priv, I225_REG_TAS_CONFIG_0, 
                        (uint32_t)(config->base_time_s & 0xFFFFFFFF));
    if (ret < 0) {
        return ret;
    }
    
    ret = i225_write_reg(priv, I225_REG_TAS_CONFIG_1, 
                        (uint32_t)(config->base_time_s >> 32));
    if (ret < 0) {
        return ret;
    }
    
    /* Configure cycle time and gate states */
    tas_config = (config->cycle_time_s << 16) | (config->cycle_time_ns >> 16);
    ret = i225_write_reg(priv, I225_REG_TAS_CONFIG_0 + 8, tas_config);
    if (ret < 0) {
        return ret;
    }
    
    /* Program gate list (simplified implementation) */
    for (int i = 0; i < 8; i++) {
        uint32_t gate_entry = (config->gate_states[i] << 24) | 
                             config->gate_durations[i];
        ret = i225_write_reg(priv, I225_REG_TAS_GATE_LIST + (i * 4), gate_entry);
        if (ret < 0) {
            return ret;
        }
    }
    
    /* Enable TAS */
    tas_ctrl |= I225_TAS_CTRL_EN | I225_TAS_CTRL_GATE_LIST | I225_TAS_CTRL_BASE_TIME;
    ret = i225_write_reg(priv, INTEL_REG_TAS_CTRL, tas_ctrl);
    if (ret < 0) {
        return ret;
    }
    
    /* Save configuration */
    i225_priv->tas_enabled = 1;
    i225_priv->tas_base_time = (config->base_time_s * NSEC_PER_SEC) + config->base_time_ns;
    i225_priv->tas_cycle_time = (config->cycle_time_s * NSEC_PER_SEC) + config->cycle_time_ns;
    
    return 0;
}

/**
 * @brief Configure Frame Preemption on I225/I226
 */
static int i225_setup_fp(struct intel_private *priv, struct tsn_fp_config *config)
{
    struct i225_private *i225_priv;
    uint32_t fp_ctrl;
    int ret;
    
    if (!priv || !priv->device_private || !config) {
        return -EINVAL;
    }
    
    i225_priv = (struct i225_private *)priv->device_private;
    
    /* Configure frame preemption */
    fp_ctrl = 0;
    if (config->verify_disable) {
        fp_ctrl |= I225_FP_CTRL_VERIFY_DIS;
    }
    
    /* Set preemptable queues (implementation specific) */
    fp_ctrl |= (config->preemptable_queues << 8);
    
    /* Set minimum fragment size */
    fp_ctrl |= (config->min_fragment_size << 16);
    
    ret = i225_write_reg(priv, I225_REG_FP_CONFIG, fp_ctrl);
    if (ret < 0) {
        return ret;
    }
    
    /* Enable frame preemption */
    fp_ctrl |= I225_FP_CTRL_EN;
    ret = i225_write_reg(priv, I225_REG_FP_CONFIG, fp_ctrl);
    if (ret < 0) {
        return ret;
    }
    
    /* Save configuration */
    i225_priv->fp_enabled = 1;
    i225_priv->fp_verify_disabled = config->verify_disable;
    i225_priv->fp_min_fragment_size = config->min_fragment_size;
    
    return 0;
}

/**
 * @brief Configure PCIe PTM on I225/I226
 */
static int i225_setup_ptm(struct intel_private *priv, struct ptm_config *config)
{
    struct i225_private *i225_priv;
    uint32_t ptm_ctrl;
    int ret;
    
    if (!priv || !priv->device_private || !config) {
        return -EINVAL;
    }
    
    i225_priv = (struct i225_private *)priv->device_private;
    
    /* Configure PTM */
    ptm_ctrl = 0;
    if (config->enabled) {
        ptm_ctrl |= I225_PTM_CTRL_EN | I225_PTM_CTRL_AUTO_UPD;
    }
    
    /* Set clock granularity */
    ptm_ctrl |= (config->clock_granularity << 8);
    
    ret = i225_write_reg(priv, I225_REG_PTM_CONFIG, ptm_ctrl);
    if (ret < 0) {
        return ret;
    }
    
    /* Save configuration */
    i225_priv->ptm_enabled = config->enabled;
    i225_priv->ptm_granularity = config->clock_granularity;
    
    return 0;
}

/**
 * @brief Initialize I225 device
 */
int intel_i225_init(device_t *dev)
{
    struct intel_private *priv;
    struct i225_private *i225_priv;
    
    if (!dev || !dev->private_data) {
        return -EINVAL;
    }
    
    priv = (struct intel_private *)dev->private_data;
    
    /* Allocate I225-specific private data */
    i225_priv = calloc(1, sizeof(struct i225_private));
    if (!i225_priv) {
        return -ENOMEM;
    }
    
    priv->device_private = i225_priv;
    
    /* Set up function pointers */
    priv->read_reg = i225_read_reg;
    priv->write_reg = i225_write_reg;
    priv->get_systime = i225_get_systime;
    priv->set_systime = i225_set_systime;
    priv->adjust_systime = i225_adjust_systime;
    priv->setup_tas = i225_setup_tas;
    priv->setup_fp = i225_setup_fp;
    priv->setup_ptm = i225_setup_ptm;
    
    /* TODO: Map MMIO region for register access */
    /* This would need platform-specific implementation */
    
    return 0;
}

/**
 * @brief Initialize I226 device (same as I225)
 */
int intel_i226_init(device_t *dev)
{
    /* I226 uses the same implementation as I225 */
    return intel_i225_init(dev);
}

/**
 * @brief Cleanup I225 device
 */
void intel_i225_cleanup(device_t *dev)
{
    struct intel_private *priv;
    
    if (!dev || !dev->private_data) {
        return;
    }
    
    priv = (struct intel_private *)dev->private_data;
    
    if (priv->device_private) {
        free(priv->device_private);
        priv->device_private = NULL;
    }
    
    /* TODO: Unmap MMIO region */
}

/**
 * @brief Cleanup I226 device (same as I225)
 */
void intel_i226_cleanup(device_t *dev)
{
    intel_i225_cleanup(dev);
}

/**
 * @brief Setup Time Aware Shaper (public API)
 */
int intel_setup_time_aware_shaper(device_t *dev, struct tsn_tas_config *config)
{
    struct intel_private *priv;
    
    if (!dev || !dev->private_data || !config) {
        return -EINVAL;
    }
    
    priv = (struct intel_private *)dev->private_data;
    
    if (!intel_has_capability(dev, INTEL_CAP_TSN_TAS)) {
        return -ENOTSUP;
    }
    
    if (!priv->setup_tas) {
        return -ENOTSUP;
    }
    
    return priv->setup_tas(priv, config);
}

/**
 * @brief Setup Frame Preemption (public API)
 */
int intel_setup_frame_preemption(device_t *dev, struct tsn_fp_config *config)
{
    struct intel_private *priv;
    
    if (!dev || !dev->private_data || !config) {
        return -EINVAL;
    }
    
    priv = (struct intel_private *)dev->private_data;
    
    if (!intel_has_capability(dev, INTEL_CAP_TSN_FP)) {
        return -ENOTSUP;
    }
    
    if (!priv->setup_fp) {
        return -ENOTSUP;
    }
    
    return priv->setup_fp(priv, config);
}

/**
 * @brief Setup PCIe PTM (public API)
 */
int intel_setup_ptm(device_t *dev, struct ptm_config *config)
{
    struct intel_private *priv;
    
    if (!dev || !dev->private_data || !config) {
        return -EINVAL;
    }
    
    priv = (struct intel_private *)dev->private_data;
    
    if (!intel_has_capability(dev, INTEL_CAP_PCIe_PTM)) {
        return -ENOTSUP;
    }
    
    if (!priv->setup_ptm) {
        return -ENOTSUP;
    }
    
    return priv->setup_ptm(priv, config);
}
