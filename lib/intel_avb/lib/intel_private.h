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

#ifndef _INTEL_PRIVATE_H_
#define _INTEL_PRIVATE_H_

#include "intel.h"

/* Private device data structure */
struct intel_private {
    intel_device_type_t device_type;
    uint32_t capabilities;
    int initialized;
    
    /* Memory mapped I/O base address */
    void *mmio_base;
    size_t mmio_size;
    
    /* Device-specific handlers */
    int (*read_reg)(struct intel_private *priv, uint32_t offset, uint32_t *value);
    int (*write_reg)(struct intel_private *priv, uint32_t offset, uint32_t value);
    int (*get_systime)(struct intel_private *priv, uint64_t *systime);
    int (*set_systime)(struct intel_private *priv, uint64_t systime);
    int (*adjust_systime)(struct intel_private *priv, int32_t ppb);
    
    /* TSN-specific handlers (I225/I226) */
    int (*setup_tas)(struct intel_private *priv, struct tsn_tas_config *config);
    int (*setup_fp)(struct intel_private *priv, struct tsn_fp_config *config);
    int (*setup_ptm)(struct intel_private *priv, struct ptm_config *config);
    
    /* Platform-specific operations */
    const struct platform_ops *platform_ops;
    void *platform_data;  /* Platform-specific context (e.g., Windows HW context) */
    
    /* MDIO handlers (I219) */
    int (*mdio_read)(struct intel_private *priv, uint32_t page, uint32_t reg, uint16_t *value);
    int (*mdio_write)(struct intel_private *priv, uint32_t page, uint32_t reg, uint16_t value);
    
    /* Device-specific private data */
    void *device_private;
};

/* Common register offsets (shared across devices) */
#define INTEL_REG_CTRL         0x00000  /* Device Control */
#define INTEL_REG_STATUS       0x00008  /* Device Status */
#define INTEL_REG_EECD         0x00010  /* EEPROM/Flash Control */
#define INTEL_REG_EERD         0x00014  /* EEPROM Read */
#define INTEL_REG_FLA          0x0001C  /* Flash Access */
#define INTEL_REG_MDIC         0x00020  /* MDI Control */
#define INTEL_REG_SCTL         0x00024  /* SerDes Control */
#define INTEL_REG_FCAL         0x00028  /* Flow Control Address Low */
#define INTEL_REG_FCAH         0x0002C  /* Flow Control Address High */
#define INTEL_REG_FCT          0x00030  /* Flow Control Type */
#define INTEL_REG_VET          0x00038  /* VLAN EtherType */
#define INTEL_REG_ICR          0x000C0  /* Interrupt Cause Read */
#define INTEL_REG_ITR          0x000C4  /* Interrupt Throttling Rate */
#define INTEL_REG_ICS          0x000C8  /* Interrupt Cause Set */
#define INTEL_REG_IMS          0x000D0  /* Interrupt Mask Set */
#define INTEL_REG_IMC          0x000D8  /* Interrupt Mask Clear */

/* Time synchronization registers */
#define INTEL_REG_SYSTIML      0x0B600  /* System Time Low */
#define INTEL_REG_SYSTIMH      0x0B604  /* System Time High */
#define INTEL_REG_TIMINCA      0x0B608  /* Time Increment Attributes */
#define INTEL_REG_TSAUXC       0x0B640  /* Auxiliary Time Stamp Control */
#define INTEL_REG_TRGTTIML0    0x0B644  /* Target Time Low 0 */
#define INTEL_REG_TRGTTIMH0    0x0B648  /* Target Time High 0 */
#define INTEL_REG_AUXSTMPL0    0x0B65C  /* Auxiliary Time Stamp Low 0 */
#define INTEL_REG_AUXSTMPH0    0x0B660  /* Auxiliary Time Stamp High 0 */

/* TSN registers (I225/I226 specific) */
#define INTEL_REG_TAS_CTRL     0x8600   /* Time Aware Shaper Control */
#define INTEL_REG_TAS_CONFIG   0x8604   /* Time Aware Shaper Config */
#define INTEL_REG_FP_CTRL      0x8700   /* Frame Preemption Control */
#define INTEL_REG_PTM_CTRL     0x8800   /* PTM Control */

/* Common control register bits */
#define INTEL_CTRL_RESET       (1 << 26)
#define INTEL_CTRL_PHY_RST     (1 << 31)

/* Common status register bits */
#define INTEL_STATUS_LU        (1 << 1)   /* Link Up */
#define INTEL_STATUS_TXOFF     (1 << 4)   /* Transmission Paused */
#define INTEL_STATUS_SPEED_10  (0 << 6)   /* 10 Mbps */
#define INTEL_STATUS_SPEED_100 (1 << 6)   /* 100 Mbps */
#define INTEL_STATUS_SPEED_1000 (2 << 6)  /* 1000 Mbps */
#define INTEL_STATUS_SPEED_2500 (3 << 6)  /* 2500 Mbps */

/* Function prototypes for internal use */
int intel_common_init(device_t *dev);
void intel_common_cleanup(device_t *dev);
int intel_has_capability(device_t *dev, uint32_t capability);
int intel_validate_packet(struct intel_packet *packet);

/* Device-specific initialization functions */
int intel_i210_init(device_t *dev);
int intel_i219_init(device_t *dev);
int intel_i225_init(device_t *dev);
int intel_i226_init(device_t *dev);

/* Device-specific cleanup functions */
void intel_i210_cleanup(device_t *dev);
void intel_i219_cleanup(device_t *dev);
void intel_i225_cleanup(device_t *dev);
void intel_i226_cleanup(device_t *dev);

/* Utility macros */
#define INTEL_READ32(priv, reg) \
    ({ uint32_t val; (priv)->read_reg((priv), (reg), &val); val; })

#define INTEL_WRITE32(priv, reg, val) \
    (priv)->write_reg((priv), (reg), (val))

#define NSEC_PER_SEC   1000000000ULL
#define USEC_PER_SEC   1000000ULL
#define MSEC_PER_SEC   1000ULL

#endif /* _INTEL_PRIVATE_H_ */
