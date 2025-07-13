# Machine Environment Description: Windows Test System (DAW02)

**Date:** 2025-07-11

## Machine Identifier
- Hostname: DAW02
- Location: [Optional, e.g. Lab 1]
- Serial/Asset Tag: [Optional]

## Hardware
- CPU: [Intel-based system capable of supporting multiple Intel adapters]
- RAM: [Adequate for running multiple Intel adapter enumeration]
- Network Adapters: ✅ **CONFIRMED INTEL HARDWARE**
  - **Intel I225-V #2**: MAC C4:62:37:05:B4:64, 2500 Mbps, Interface 5 (DOWN)
  - **Intel I210-T1**: MAC 68:05:CA:8B:0D:D0, 1000 Mbps, Interface 17 (UP) ✅
  - **Intel I225-V**: MAC C4:62:37:05:B4:65, 1000 Mbps, Interface 12 (UP) ✅

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
- ✅ **RESOLVED**: Multiple Intel adapters now successfully detected (3x adapters confirmed)
- ✅ **WORKING**: Windows IPHLPAPI integration functional with real hardware data
- ✅ **CONFIRMED**: MAC address enumeration working (all 3 adapters detected)
- **Performance**: Adapter enumeration latency ~1.5ms (acceptable for current use)
- **Missing**: No I217 adapters available for I217 implementation testing

## Custom Configuration
- [Please fill in any custom network, BIOS, or driver settings relevant for AVB/TSN]

## Test Results Summary (2025-07-12)
✅ **Hardware Validation COMPLETE**
- **Test Type**: Phase 2 baseline measurement with 10,000 iterations
- **Adapters Tested**: All 3 Intel adapters successfully enumerated
- **Performance**: 1494-1496μs average enumeration time
- **Compliance**: Repository rule enforced - NO mock/simulated data
- **Status**: PRODUCTION READY for I225/I210 series testing

## Hardware Validation Status
- **I225-V Series**: ✅ CONFIRMED (2x adapters, mixed speeds)
- **I210-T1 Series**: ✅ CONFIRMED (1x adapter, operational)
- **I217 Series**: ⚠️ NOT AVAILABLE (no hardware for testing)
- **Windows Integration**: ✅ WORKING (IPHLPAPI enumeration functional)

---
*Last Updated: 2025-07-13 - Hardware validation complete with test results*

---
*This file is machine-specific for DAW02. Please update with any hardware or software changes relevant for reproducibility or debugging.*
