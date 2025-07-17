---
applyTo: '**'
---

# OpenAvnu AI Coding Agent Instructions

## Project Overview

OpenAvnu is a comprehensive Audio Video Bridging (AVB)/Time-Sensitive Networking (TSN) implementation for real-time media streaming. The project provides IEEE 802.1AS (gPTP) time synchronization, IEEE 1722 (AVTP) media transport, and IEEE 1722.1 (AVDECC) device control protocols.

**Architecture**: Multi-layered system with hardware abstraction (Intel NICs), protocol stacks (gPTP/AVTP/AVDECC), daemons for system services, and example applications.

## Critical Build System Knowledge

### CMake-Centric Build Process
```bash
# Standard build pattern
mkdir -p build && cd build
cmake .. -DOPENAVNU_BUILD_GPTP=ON -DOPENAVNU_BUILD_LA_AVDECC=ON
cmake --build . --config Release
```

**Key CMake Options**:
- `OPENAVNU_BUILD_INTEL_HAL=ON` - Intel I210/I219/I225/I226 hardware support
- `OPENAVNU_BUILD_GPTP=ON` - gPTP time synchronization daemon  
- `OPENAVNU_BUILD_LA_AVDECC=ON` - Modern AVDECC (IEEE 1722.1-2021) support
- `OPENAVNU_BUILD_TESTING=ON` - Comprehensive test framework

### PCAP Detection System
The project uses unified PCAP detection (`cmake/FindPCAP.cmake`) supporting both Npcap and WinPcap on Windows. Set environment variables:
- `NPCAP_DIR` or `NPCAP_SDK_DIR` for Npcap
- `WPCAP_DIR` for WinPcap (legacy fallback)

## Architecture Components

### Core Libraries (`lib/`)
- **`avtp_pipeline/`** - AVTP media streaming engine with GStreamer integration
- **`intel_avb/`** - Intel hardware timestamping and register access
- **`atl_avb/`** - ATL (Atheros) hardware support  
- **`la_avdecc/`** - Modern AVDECC implementation (IEEE 1722.1-2021)
- **`common/`** - Shared utilities and platform abstraction

### System Daemons (`daemons/`)
- **`maap/`** - Multicast Address Allocation Protocol daemon
- **`mrpd/`** - Multiple Registration Protocol daemon (SRP)
- **`shaper/`** - Traffic shaping for QoS compliance

### Hardware Abstraction
The Intel HAL (`thirdparty/intel-ethernet-hal/`) provides unified access to I210/I219/I225/I226 features:
- Hardware timestamping
- Precision Time Protocol (PTP) registers
- AVB queue configuration

## Critical Development Patterns

### Protocol Compliance Focus
Always use authoritative constants from jdksavdecc library:
```c
#include "../avdecc-lib/jdksavdecc-c/include/jdksavdecc_aem_command.h"
// Use: JDKSAVDECC_AEM_COMMAND_READ_DESCRIPTOR  
// NOT: hardcoded 0x0004
```

### AEM Checksum Implementation
IEEE 1722.1 requires AEM checksums for descriptor integrity:
```c
// Calculate CRC32 over descriptor excluding checksum field
uint32_t aem_checksum = calculate_aem_checksum(descriptor, sizeof(descriptor), 
                                              offsetof(descriptor_t, aem_checksum));
descriptor->aem_checksum = htonl(aem_checksum);
```

### Testing Framework Integration
Use the unified testing framework (`testing/unified/`) for hardware validation:
```powershell
# Run all adapter tests
./testing/unified/generic_adapter_testing_framework.ps1 -SaveResults -Verbose

# Test specific hardware
./testing/unified/generic_adapter_testing_framework.ps1 -AdapterFilter "I225" -TestPhase "timestamp"
```

## Project-Specific Conventions

### Documentation Lifecycle Management
- **Active tasks**: Update `docs/TODO.md` when starting work
- **Completed features**: Move documentation to `docs/completed/`
- **Obsolete content**: Archive to `docs/archive/`
- **Status tracking**: Use `PROJECT_OVERVIEW.md` for high-level status

### Hardware-First Validation
Never mark features as "complete" without real hardware testing. The project has specific Intel NIC hardware requirements (I210/I219/I225/I226) and requires actual network synchronization validation.

### VS Code Task Integration
The project provides comprehensive VS Code tasks for all major operations:
- Build tasks for different configurations (Debug/Release/RelWithDebInfo)
- Hardware-specific tests (Intel adapter types)
- Daemon lifecycle management (start/stop/configure)
- Testing phases (detection/register/timestamp/gPTP/performance)

## AVDECC Entity Development

### Protocol Compliance Requirements
When developing AVDECC entities for Hive-AVDECC compatibility:
1. Use jdksavdecc constants for all command/descriptor types
2. Implement proper AEM checksums using CRC32
3. Handle available_index increment correctly (only on state change)
4. Support Milan extensions (command 0x004b GET_DYNAMIC_INFO)
5. Include all mandatory descriptors (ENTITY, CONFIGURATION, STREAM_INPUT/OUTPUT, AVB_INTERFACE)

### Testing with Real Controllers
Examples include realistic implementations:
- `examples/hive_compatible_avdecc_entity.c` - Hive-AVDECC validated entity
- `examples/professional_avdecc_entity.c` - Production-grade implementation
- `examples/raw_ethernet_avdecc_entity.c` - Low-level packet handling

## Cross-Platform Considerations

### Windows-Specific Requirements
- Requires PCAP (Npcap recommended, WinPcap legacy support)
- Use PowerShell for automation scripts
- Intel driver compatibility critical for hardware timestamping

### Linux Requirements  
- Requires libpcap-dev, libpci-dev for hardware access
- Use bash scripts for automation
- Kernel module support for Intel drivers

## Integration Points

### External Dependencies
- **GStreamer**: Media processing pipeline integration
- **PCAP**: Network packet capture (cross-platform)
- **CppUTest**: Unit testing framework
- **L-Acoustics AVDECC**: Modern AVDECC protocol stack

### Cross-Component Communication
- gPTP provides time base for AVTP streams
- MAAP allocates multicast addresses for streams  
- MRP reserves bandwidth for AVB streams
- AVDECC provides device discovery and control

## Development Workflow

1. **Setup**: Initialize git submodules (`git submodule update --init --recursive`)
2. **Configure**: Create build directory and run CMake with appropriate options
3. **Build**: Use cmake --build for cross-platform compilation
4. **Test**: Run unified testing framework before claiming completion
5. **Document**: Update status in PROJECT_OVERVIEW.md and move docs appropriately

## Performance Expectations

**gPTP Synchronization**: ±80ns accuracy target (Milan: stricter requirements)
**AVTP Latency**: Sub-millisecond for real-time audio
**Memory Usage**: Stable allocation patterns for long-running daemons
**CPU Usage**: Minimal impact on host system performance

## Essential Testing & Validation Rules

### Real-World Hardware Testing Required
Never mark features as "complete" without testing on actual Intel hardware (I210/I219/I225/I226). Use the unified testing framework in `testing/unified/` which provides comprehensive validation across all adapter types.

### Documentation Lifecycle
- **TODO tracking**: Use `docs/TODO.md` for active work items
- **Completion**: Move finished work documentation to `docs/completed/`
- **Archive**: Move obsolete documentation to `docs/archive/`
- **Insights**: Document lessons learned in `docs/insights/` to prevent recurring issues

### No Mocking for Integration Tests
Do not use mock or simulated data for integration testing - OpenAvnu requires real hardware validation for network synchronization, timestamping, and protocol compliance.

### Verify All Assumptions
Always verify behavior through testing, logs, or direct inspection. If unclear - investigate until confirmed. Document findings in `docs/insights/` for future reference.
