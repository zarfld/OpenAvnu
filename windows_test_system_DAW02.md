# Machine Environment Description: Windows Test System (DAW02)

**Date:** 2025-07-11

## Hardware
- CPU: Intel(R) Xeon(R) CPU           X5680  @ 3.33GHz
- RAM: 20 GB
- Network Adapters:
  - Name: Ethernet | Description: Intel(R) Ethernet I210-T1 GbE NIC | MAC: 68-05-CA-8B-0D-D0 | Status: Up | Driver: 14.0.7.0 | Provider: Intel | PNP: PCI\VEN_8086&DEV_1533&SUBSYS_0003103C&REV_03\4&3ed1ce&0&0008
  - Name: Ethernet 2 | Description: Intel(R) Ethernet Controller (3) I225-V | MAC: C4-62-37-05-B4-65 | Status: Up | Driver: 1.1.4.43 | Provider: Intel | PNP: PCI\VEN_8086&DEV_15F3&SUBSYS_00008086&REV_03\6&1b54528c&0&001800E0
  - Name: Ethernet 5 | Description: Intel(R) Ethernet Controller (3) I225-V #2 | MAC: C4-62-37-05-B4-64 | Status: Disconnected | Driver: 1.1.4.43 | Provider: Intel | PNP: PCI\VEN_8086&DEV_15F3&SUBSYS_00008086&REV_03\6&b26c8c5&0&001000E0

## Operating System
- Microsoft Windows 10 Pro, Version 10.0.19045 (Build 19045)
- 64-Bit

## Software & Drivers
- Visual Studio: 
- CMake: 4.1.0-rc1
- WinPcap SDK: C:\WpdPack
- Npcap: Not installed
- Intel Ethernet drivers: [Please fill in version]

## Dependencies
- Python: Python wurde nicht gefunden; ohne Argumente ausf³hren, um aus dem Microsoft Store zu installieren, oder deaktivieren Sie diese Verkn³pfung unter "Einstellungen > Apps > Erweiterte App-Einstellungen > App-Ausf³hrungsaliase".
- Git: git version 2.47.0.windows.2
- PowerShell: 5.1.19041.6093

## Known Issues / Limitations
- MAC address cannot be read by HAL on Windows (see insights)
- NDIS timestamping only partially supported (fallback mode)
- Only one adapter per type detected in test logs

## Custom Configuration
- [Please fill in any custom network, BIOS, or driver settings relevant for AVB/TSN]

---
*Please update this file with any hardware or software changes relevant for reproducibility or debugging.*
