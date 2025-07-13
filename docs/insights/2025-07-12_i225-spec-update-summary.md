# Insight: Intel I225 Specification Update (v1.2)

**Context:**
Summary of Intel® Ethernet Controller I225 Specification Update, Revision 1.2 (Nov 2020), for OpenAvnu hardware and driver development.

**Observation:**
- The I225 controller has multiple part numbers and versions (V1, V2, V3, LM, V, IT) with different markings and branding strings.
- Device identification is critical: always check the latest part identification summary and marking diagrams for accurate hardware detection.
- Errata related to IEEE behavior in 2.5Gbps mode is documented.
- Marking diagrams specify logo, lot, spec#, and Pb-free symbol placement for device verification.

**Root Cause:**
- Multiple silicon revisions and packaging options require careful device identification and driver support.

**Resolution:**
- Use the latest part identification tables and marking diagrams to ensure correct hardware support and troubleshooting.
- For any IEEE or 2.5Gbps mode issues, consult the errata section for workarounds or limitations.

**Takeaways:**
- Always verify the exact I225 part and revision before deployment or debugging.
- Update test and validation procedures if new errata or part numbers are published.
- Maintain up-to-date documentation of supported device IDs and markings.


**Reference:**
- `spec/621661-Intel® Ethernet Controller I225-Public External Specification Update-v1.2.md` (Intel I225 Specification Update, v1.2, Nov 2020)

---

## Register and Address Reference (TODO)


### Common Control and Status Registers (from I225 datasheet)

| Register Name | Address (Offset) | Description |
|---------------|------------------|-------------|
| CTRL          | 0x00000          | Device Control Register: Global device enable, reset, link, speed, etc. |
| STATUS        | 0x00008          | Device Status Register: Link status, speed, duplex, etc. |
| EEC           | 0x00010          | EEPROM/Flash Control Register |
| MDIC          | 0x00020          | MDI Control Register: PHY access (read/write) |
| RCTL          | 0x00100          | Receive Control Register: Enables/disables receiver, sets buffer sizes, etc. |
| TCTL          | 0x00400          | Transmit Control Register: Enables/disables transmitter, pad short packets, etc. |
| RDBAL         | 0x02800          | Receive Descriptor Base Address Low |
| RDBAH         | 0x02804          | Receive Descriptor Base Address High |
| TDBAL         | 0x03800          | Transmit Descriptor Base Address Low |
| TDBAH         | 0x03804          | Transmit Descriptor Base Address High |
| IMS           | 0x000D0          | Interrupt Mask Set/Read Register |
| IMC           | 0x000D8          | Interrupt Mask Clear Register |
| ICR           | 0x000C0          | Interrupt Cause Read Register |


### AVB/TSN and Timestamping Related Registers

| Register Name | Address (Offset) | Description |
|---------------|------------------|-------------|
| TSYNCRXCTL    | 0x0B620          | Timestamp RX Control: Enables/disables RX timestamping, configures timestamping mode |
| TSYNCTXCTL    | 0x0B614          | Timestamp TX Control: Enables/disables TX timestamping, configures timestamping mode |
| TSYNCRXCFG    | 0x0B634          | Timestamp RX Configuration: Filters, timestamping options |
| TSYNCTXCFG    | 0x0B618          | Timestamp TX Configuration: Filters, timestamping options |
| SYSTIML       | 0x0B600          | System Time Low: Lower 32 bits of the 64-bit system time counter |
| SYSTIMH       | 0x0B604          | System Time High: Upper 32 bits of the 64-bit system time counter |
| SYSTIMR       | 0x0B608          | System Time Residue: Sub-nanosecond residue for high-precision time |
| TIMINCA       | 0x0B608          | Time Increment Adjustment: Adjusts system time increment per clock |
| RXSTMPL       | 0x0B624          | RX Timestamp Low: Captured timestamp for received packets |
| RXSTMPH       | 0x0B628          | RX Timestamp High: Captured timestamp for received packets |
| TXSTMPL       | 0x0B618          | TX Timestamp Low: Captured timestamp for transmitted packets |
| TXSTMPH       | 0x0B61C          | TX Timestamp High: Captured timestamp for transmitted packets |

### Diagnostics and Miscellaneous Registers

| Register Name | Address (Offset) | Description |
|---------------|------------------|-------------|
| GPIE          | 0x01514          | General Purpose Interrupt Enable Register |
| LEDCTL        | 0x0E00           | LED Control Register |
| PBA           | 0x01000          | Packet Buffer Allocation Register |
| FCRTL         | 0x02160          | Flow Control Receive Threshold Low |
| FCRTH         | 0x02168          | Flow Control Receive Threshold High |


### EEPROM/Flash and PHY Registers
### Descriptor Ring and Buffer Management Registers
### Interrupt and Event Registers
### Power Management and Advanced Feature Registers
### PCIe and Bus Interface Registers
### MAC Address and Filtering Registers

| Register Name | Address (Offset) | Description |
|---------------|------------------|-------------|
| RAL0          | 0x05400          | Receive Address Low 0: Lower 32 bits of the first MAC address |
| RAH0          | 0x05404          | Receive Address High 0: Upper 16 bits of the first MAC address and address valid bit |
| RAL1-15       | 0x05408+         | Additional Receive Address Low registers for unicast/multicast addresses |
| RAH1-15       | 0x0540C+         | Additional Receive Address High registers for unicast/multicast addresses |
| MTA0-127      | 0x05200+         | Multicast Table Array: 128 registers for multicast address filtering |
| VFTA0-127     | 0x05600+         | VLAN Filter Table Array: 128 registers for VLAN ID filtering |

#### Example Bitfields (RAH0 Register):

- `ADDR` (bits 0-15): Upper 16 bits of MAC address
- `AV` (bit 31): Address valid

// Add more MAC address and filtering register details as needed for unicast/multicast filtering and address management.

| Register Name | Address (Offset) | Description |
|---------------|------------------|-------------|
| GCR           | 0x05B00          | General Control Register: Controls PCIe and bus interface features |
| GCR2          | 0x05B64          | General Control Register 2: Additional PCIe/bus features |
| PCIEMISC      | 0x05BB8          | PCIe Miscellaneous Control: Misc PCIe configuration and status |
| PXPCAP        | 0x0C000          | PCIe Capability Structure: PCIe link status, negotiated width, etc. |
| PBA_ECC       | 0x01100          | Packet Buffer ECC Status: ECC error status for packet buffer |
| HLREG0        | 0x04240          | High-Level Control Register 0: Jumbo frame, CRC, and bus mode control |
| HLREG1        | 0x04244          | High-Level Control Register 1: Additional bus and MAC control |

#### Example Bitfields (GCR Register):

- `RXD_NO_SNOOP` (bit 0): Disable PCIe snooping for RX descriptors
- `TXD_NO_SNOOP` (bit 1): Disable PCIe snooping for TX descriptors
- `PCI_MASTER_DIS` (bit 2): Disable PCIe master capability

// Add more PCIe and bus interface register details as needed for low-level hardware and bus troubleshooting.

| Register Name | Address (Offset) | Description |
|---------------|------------------|-------------|
| CTRL_EXT      | 0x00018          | Extended Device Control: Power management, link, and advanced features |
| FWSM          | 0x05800          | Firmware Semaphore Register: Synchronization with firmware/BIOS |
| MANC          | 0x05820          | Management Control Register: Enables/disables management features |
| PROXYFC       | 0x05F60          | Proxy Filter Control: Enables/disables proxy filtering for low-power states |
| WUC           | 0x05800          | Wake Up Control: Controls wake-on-LAN and wake events |
| WUFC          | 0x05808          | Wake Up Filter Control: Configures wake event filtering |
| IPAV          | 0x05838          | IP Address Valid: Indicates valid IP address for management |
| EEMNGCTL      | 0x01010          | Management Control for NVM/EEPROM |

#### Example Bitfields (CTRL_EXT Register):

- `SPEED_1000` (bit 23): Force 1Gbps operation
- `RO_DIS` (bit 30): Receive only disable
- `DRV_LOAD` (bit 28): Driver loaded indicator

// Add more power management and advanced feature register details as needed for low-power and advanced configuration use cases.

| Register Name | Address (Offset) | Description |
|---------------|------------------|-------------|
| EICR          | 0x01580          | Extended Interrupt Cause Read: Indicates which events triggered an interrupt |
| EIAC          | 0x0158C          | Extended Interrupt Auto Clear: Controls auto-clear of EICR bits |
| EIMS          | 0x01590          | Extended Interrupt Mask Set/Read: Enables/disables specific interrupt sources |
| EIMC          | 0x01594          | Extended Interrupt Mask Clear: Disables specific interrupt sources |
| EIAM          | 0x01598          | Extended Interrupt Auto Mask: Controls auto-masking of interrupt sources |
| IVAR          | 0x01700          | Interrupt Vector Allocation Register: Maps interrupt causes to MSI-X vectors |
| ITR0          | 0x00820          | Interrupt Throttling Rate 0: Controls interrupt moderation timing |
| ITR1          | 0x00824          | Interrupt Throttling Rate 1 |
| ITR2          | 0x00828          | Interrupt Throttling Rate 2 |
| ITR3          | 0x0082C          | Interrupt Throttling Rate 3 |

#### Example Bitfields (EICR Register):

- `TXDW` (bit 0): Transmit Descriptor Written interrupt
- `TXQE` (bit 1): Transmit Queue Empty interrupt
- `LSC` (bit 2): Link Status Change interrupt
- `RXSEQ` (bit 3): Receive Sequence Error interrupt
- `RXDMT0` (bit 4): Receive Descriptor Minimum Threshold interrupt
- `RXO` (bit 6): Receiver Overrun interrupt

// Add more interrupt and event register details as needed for event handling and debugging.

| Register Name | Address (Offset) | Description |
|---------------|------------------|-------------|
| RDLEN         | 0x02808          | Receive Descriptor Length: Number of bytes allocated for RX descriptor ring |
| RDH           | 0x02810          | Receive Descriptor Head: Index of the next descriptor to be processed by hardware |
| RDT           | 0x02818          | Receive Descriptor Tail: Index of the last descriptor to be processed by hardware |
| TDLEN         | 0x03808          | Transmit Descriptor Length: Number of bytes allocated for TX descriptor ring |
| TDH           | 0x03810          | Transmit Descriptor Head: Index of the next descriptor to be processed by hardware |
| TDT           | 0x03818          | Transmit Descriptor Tail: Index of the last descriptor to be processed by hardware |
| SRRCTL        | 0x02100          | Split Receive Control: Controls buffer size, header splitting, and descriptor type |
| DCA_RXCTRL    | 0x0100C          | Direct Cache Access RX Control: Enables/disables DCA for RX |
| DCA_TXCTRL    | 0x07200          | Direct Cache Access TX Control: Enables/disables DCA for TX |

#### Example Bitfields (SRRCTL Register):

- `BSIZEPKT` (bits 10-15): Receive buffer size per packet
- `DESCTYPE` (bits 25-27): Descriptor type (legacy, advanced, split, etc.)

// Add more descriptor ring and buffer management register details as needed for driver and DMA development.

| Register Name | Address (Offset) | Description |
|---------------|------------------|-------------|

| EERD          | 0x00014          | EEPROM Read Register: Used to read data from the EEPROM/Flash |
| EEMNGCTL      | 0x01010          | Management Control Register: Controls management interface to EEPROM/Flash |
| FEXTNVM6      | 0x01028          | NVM Feature Register: Contains feature enable bits from NVM |
| PHY_CTRL      | 0x000E0          | PHY Control Register: Controls PHY reset, power, and loopback |
| PHY_STATUS    | 0x000E4          | PHY Status Register: Indicates PHY link, speed, and status |

#### EERD Register Bitfields

| Bit(s)   | Name   | Description                       |
|----------|--------|-----------------------------------|
| 0        | START  | Start EEPROM Read Operation       |
| 4        | DONE   | EEPROM Read Done                  |
| 8-19     | ADDR   | EEPROM Address to Read            |
| 20-31    | DATA   | Data Read from EEPROM             |

#### PHY_CTRL Register Bitfields

| Bit(s)   | Name         | Description                                 |
|----------|--------------|---------------------------------------------|
| 6        | SPEED_SELECT | Speed Selection (see below)                 |
| 13       | SPEED_SELECT | Speed Selection (see below)                 |
| 14       | LOOPBACK     | Enable PHY Loopback                         |
| 15       | PHY_RESET    | PHY Reset                                   |

// SPEED_SELECT: 00=10Mbps, 01=100Mbps, 10=1000Mbps, 11=2.5Gbps



---

## Device Identification Table (from Spec v1.2)

| Part #      | Version | Intel MM # | Spec Code | Type | Network Driver Branding String |
|-------------|---------|------------|-----------|------|-------------------------------|
| I225-LM     | V1      | 999JW1     | SLN9B     | T&R  | Intel(R) Ethernet Controller I225-LM |
| I225-LM     | V1      | 999JW0     | SLN9A     | Tray | Intel(R) Ethernet Controller I225-LM |
| I225-V      | V1      | 999JW7     | SLN9D     | T&R  | Intel(R) Ethernet Controller I225-V |
| I225-V      | V1      | 999JW5     | SLN9C     | Tray | Intel(R) Ethernet Controller I225-V |
| I225-LM     | V2      | 99A1TL     | SLNJW     | T&R  | Intel(R) Ethernet Controller (2) I225-LM |
| I225-LM     | V2      | 99A1T7     | SLNJV     | Tray | Intel(R) Ethernet Controller (2) I225-LM |
| I225-V      | V2      | 99A1VF     | SLNJY     | T&R  | Intel(R) Ethernet Controller (2) I225-V |
| I225-V      | V2      | 99A1V1     | SLNJX     | Tray | Intel(R) Ethernet Controller (2) I225-V |
| I225-LM     | V3      | 99A57P     | SLNNJ     | T&R  | Intel(R) Ethernet Controller (3) I225-LM |
| I225-LM     | V3      | 99A57N     | SLNNH     | Tray | Intel(R) Ethernet Controller (3) I225-LM |
| I225-V      | V3      | 99A3W6     | SLNMH     | T&R  | Intel(R) Ethernet Controller (3) I225-V |
| I225-V      | V3      | 99A3W5     | SLMNG     | Tray | Intel(R) Ethernet Controller (3) I225-V |
| I225-IT     | V3      | 99A57T     | SLNNL     | T&R  | Intel(R) Ethernet Controller (3) I225-IT |
| I225-IT     | V3      | 99A57R     | SLNNK     | Tray | Intel(R) Ethernet Controller (3) I225-IT |

## Marking Diagram Notes

- LINE1: Logo - Intel Swirl Logo
- LINE2: FPO# - Assembly lot marking, 8 characters
- LINE3: Spec#
- LINE4: e# - Pb-free symbol
- All lines of text are centered on the package and left justified.

Refer to the full spec for visual diagrams and further details.
