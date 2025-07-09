/******************************************************************************

  Copyright (c) 2025, Intel Corporation
  All rights reserved.

  Intel AVB HAL - Windows Driver Interface Header

******************************************************************************/

#ifndef _INTEL_WINDOWS_H_DEFINED_
#define _INTEL_WINDOWS_H_DEFINED_

#include <windows.h>
#include "intel.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Platform operations structure */
struct platform_ops {
    /* Initialization and cleanup */
    int (*init)(device_t *dev);
    void (*cleanup)(device_t *dev);
    
    /* PCI configuration space access */
    int (*pci_read_config)(device_t *dev, DWORD offset, DWORD *value);
    int (*pci_write_config)(device_t *dev, DWORD offset, DWORD value);
    
    /* MMIO register access */
    int (*mmio_read)(device_t *dev, uint32_t offset, uint32_t *value);
    int (*mmio_write)(device_t *dev, uint32_t offset, uint32_t value);
    
    /* MDIO operations (for I219) */
    int (*mdio_read)(device_t *dev, uint16_t phy_addr, uint16_t reg_addr, uint16_t *value);
    int (*mdio_write)(device_t *dev, uint16_t phy_addr, uint16_t reg_addr, uint16_t value);
    
    /* IEEE 1588 timestamp access */
    int (*read_timestamp)(device_t *dev, uint64_t *timestamp);
};

/* Get Windows platform operations */
const struct platform_ops *intel_get_windows_platform_ops(void);

/* I219 specific register definitions */
#define I219_REG_CTRL           0x0000  /* Device Control */
#define I219_REG_STATUS         0x0008  /* Device Status */
#define I219_REG_MDIO_CTRL      0x0020  /* MDIO Control */
#define I219_REG_MDIO_DATA      0x0024  /* MDIO Data */
#define I219_REG_1588_CTRL      0x0B00  /* IEEE 1588 Control */
#define I219_REG_1588_STATUS    0x0B04  /* IEEE 1588 Status */
#define I219_REG_1588_TS_LOW    0x0B10  /* Timestamp Low */
#define I219_REG_1588_TS_HIGH   0x0B14  /* Timestamp High */

/* MDIO register addresses for I219 PHY */
#define I219_PHY_CTRL           0x00    /* PHY Control Register */
#define I219_PHY_STATUS         0x01    /* PHY Status Register */
#define I219_PHY_ID1            0x02    /* PHY Identifier 1 */
#define I219_PHY_ID2            0x03    /* PHY Identifier 2 */
#define I219_PHY_AUTONEG_ADV    0x04    /* Auto-Negotiation Advertisement */
#define I219_PHY_1588_CTRL      0x20    /* IEEE 1588 PHY Control */
#define I219_PHY_1588_STATUS    0x21    /* IEEE 1588 PHY Status */

/* PHY addresses */
#define I219_PHY_ADDR           0x01    /* Default PHY address for I219 */

#ifdef __cplusplus
}
#endif

#endif /* _INTEL_WINDOWS_H_DEFINED_ */
