# Machine Environment Description: Windows Test System (DAW02)

**Date:** 2025-07-11

## Machine Identifier
- Hostname: DAW02
- Location: [Optional, e.g. Lab 1]
- Serial/Asset Tag: [Optional]

## Hardware
- CPU: [Please fill in, e.g. Intel Core i7-xxxx]
- RAM: [Please fill in, e.g. 16 GB]
- Network Adapters:
  - Intel I210
  - Intel I225-V
  - [List any other relevant adapters]

## Operating System
- Windows 10 Pro, Version 22H2 (Build 19045)
- 64-bit

## Software & Drivers
- Visual Studio 2022 (MSVC 19.38)
- CMake 3.12+
- WinPcap SDK (legacy, C:\WpdPack)
- [Npcap if installed: Please fill in]
- Intel Ethernet drivers: [Please fill in version]

## Dependencies
- All dependencies installed as per OpenAvnu documentation
- Python, Git, PowerShell 5.1

## Known Issues / Limitations
- MAC address cannot be read by HAL on Windows (see insights)
- NDIS timestamping only partially supported (fallback mode)
- Only one adapter per type detected in test logs

## Custom Configuration
- [Please fill in any custom network, BIOS, or driver settings relevant for AVB/TSN]

---
*This file is machine-specific for DAW02. Please update with any hardware or software changes relevant for reproducibility or debugging.*
