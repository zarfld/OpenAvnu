---
applyTo: '**'
---

# OpenAvnu AI Coding Agent Instructions

## Working principles
-- ensure you understand the architecture and patterns before coding
-- No Fake, No Stubs, no Simulations, simplified code allowed in productive Code
-- no implementation based assumtions, use specification or analysis results (ask if required)
-- no false advertising, prove and ensure correctness
-- never fix public APIs in a way that breaks existing clients, prefer fixing implementation instead
-- prevent dead code or orphan files: fix code rather that creating new versions, ensure ALL code compiles.
-- always use real hardware access patterns
-- use Intel hardware specifications for register access
-- code needs to compile before commit, no broken code
-- Always reference the exact Intel datasheet section or spec version when implementing register access.
-- Validate all hardware reads/writes with range checks or masks from the specification.
-- Every function must have a Doxygen comment explaining purpose, parameters, return values, and hardware context.
-- no duplicate or redundant implementations to avoid inconsistencies and confusion; use centralized, reusable functions instead
-- prevent downgrades e.g. fix rather than delete
-- no ad-hoc file copies (e.g., *_fixed, *_new, *_correct); refactor in place step-by-step to avoid breakage
-- Clean submit rules:
   - each commit compiles and passes checks
   - small, single-purpose, reviewable diffs (no WIP noise)
   - no dead or commented-out code; remove unused files
   - run formatter and static analysis before commit
   - update docs/tests and reference the spec/issue in the message
   - use feature flags or compatibility layers when incremental changes risk breakage
-- Avoid unnecessary duplication. Duplication is acceptable when it improves clarity, isolates modules, or is required for performance.
-- Avoid code that is difficult to understand. Prefer clear naming and structure over excessive comments or unnecessary helper variables.
-- Avoid unnecessary complexity. Keep required abstractions for maintainability, testability, or hardware safety
-- Design modules so that changes in one module do not require changes in unrelated modules. Avoid dependencies that cause single changes to break multiple areas.
-- Design components for reuse where practical, but prioritize correctness and domain fit over forced generalization.
-- Prefer incremental modification of existing code over reimplementation; adapt existing functions instead of creating redundant new ones

## Project Overview

OpenAvnu is a comprehensive Audio Video Bridging (AVB)/Time-Sensitive Networking (TSN) implementation for real-time media streaming. The project provides IEEE 802.1AS (gPTP) time synchronization, IEEE 1722 (AVTP) media transport, and IEEE 1722.1 (AVDECC) device control protocols.

**Architecture**: Multi-layered system with hardware abstraction (Intel NICs), protocol stacks (gPTP/AVTP/AVDECC), daemons for system services, and example applications.

## CRITICAL: Automation Scripts and Folder Structure

### **📁 MANDATORY Folder Structure:**

```
OpenAvnu-2/                           ← Main repository root
├── build/                           ← Main OpenAvnu CMake builds
│   ├── Debug/Release/               ← Integrated builds (gptp2 target)
│   └── thirdparty/gptp/Debug/       ← Main build gPTP location
├── docs/                           ← Architecture documentation
│   ├── tests/results/              ← Test output logs
│   └── completed/archive/          ← Documentation lifecycle
├── examples/                       ← Application examples
│   └── asio-listener/              ← ASIO integration scripts
├── testing/unified/                ← Cross-component testing
│   └── generic_adapter_testing_framework.ps1
├── thirdparty/
│   ├── gptp/                       ← gPTP submodule
│   │   ├── build_gptp/             ← ⚠️ CRITICAL: Milan test directory
│   │   │   ├── Debug/              ← test_milan_compliance_validation.ps1 expects THIS
│   │   │   │   ├── gptp.exe        ← Built executable
│   │   │   │   ├── test_milan_config.ini ← Config files copied here
│   │   │   │   └── *.log           ← Test output logs
│   │   │   └── Release/            ← Production builds
│   │   ├── build/                  ← Alternative build (main integration)
│   │   └── *.ps1                   ← Milan/Automotive test scripts
│   ├── intel-ethernet-hal/         ← Intel HAL submodule
│   │   └── build_windows.bat       ← Windows build script
│   └── intel_avb/                  ← IOCTL interface library
└── lib/Standards/                  ← Hardware-agnostic protocols
```

### **🔧 Automation Scripts Inventory:**

#### **Main Repository Scripts (Root Level):**
- **Deployment**: `create_deployment_package*.ps1` - Production packaging
- **Testing**: `test_*.ps1` - Component integration tests  
- **ASIO Integration**: `debug_asio_integration.ps1`, `test_asio_*` - Audio integration
- **Network Validation**: `verify_milan_*.ps1`, `diagnose_ptp_*.ps1` - Network testing
- **Build Automation**: VS Code tasks.json with comprehensive build/test workflows

## **📝 Practical Examples**

### **Complete Development Workflow Example:**

#### **Scenario: Adding New AVDECC Entity Support**
```bash
# 1. Setup and build environment
git submodule update --init --recursive
mkdir -p build && cd build
cmake .. -DOPENAVNU_BUILD_LA_AVDECC=ON -DOPENAVNU_BUILD_GPTP=ON

# 2. Build with proper configuration
cmake --build . --config Release --target la_avdecc_cxx

# 3. Test on real hardware
cd ../testing/unified
./generic_adapter_testing_framework.ps1 -AdapterFilter "I225" -TestPhase "avdecc"
```

#### **Example: AVDECC Entity Implementation**
```cpp
// Standards Layer - Hardware agnostic
#include "../avdecc-lib/jdksavdecc-c/include/jdksavdecc_aem_command.h"

// Use standard constants, never hardcoded values
uint16_t command_type = JDKSAVDECC_AEM_COMMAND_READ_DESCRIPTOR;  // ✅ CORRECT
// uint16_t command_type = 0x0004;  // ❌ WRONG - hardcoded

// Calculate AEM checksum for descriptor integrity
uint32_t calculate_aem_checksum(void* descriptor, size_t size, size_t checksum_offset) {
    // IEEE 1722.1 compliant CRC32 calculation
    return crc32_calculate(descriptor, size, checksum_offset);
}
```

### **Architecture Pattern Examples:**

#### **✅ CORRECT: Layered Architecture with Dependency Injection**
```cpp
// Standards Layer (lib/Standards/) - Pure protocol logic
typedef struct {
    int (*get_timestamp)(uint64_t* timestamp);
    int (*send_packet)(const uint8_t* data, size_t length);
} hardware_interface_t;

void gptp_process_sync(const hardware_interface_t* hw_if) {
    uint64_t tx_time;
    if (hw_if->get_timestamp(&tx_time) == 0) {
        // Process using timestamp
    }
}

// Service Layer - Bridges Standards to Hardware
void gptp_service_init() {
    hardware_interface_t intel_interface = {
        .get_timestamp = intel_hal_get_timestamp,  // Intel HAL function
        .send_packet = intel_hal_send_packet
    };
    gptp_process_sync(&intel_interface);
}
```

#### **❌ WRONG: Direct Hardware Coupling in Standards**
```cpp
// ❌ Standards Layer should NEVER include hardware headers
#include "intel_ethernet_hal.h"  // ARCHITECTURAL VIOLATION!

void gptp_process_sync() {
    // ❌ Direct hardware calls in Standards layer
    intel_hal_get_timestamp(&timestamp);
}
```

### **Script Usage Examples:**

#### **Milan Profile Testing Workflow:**
```powershell
# Complete Milan validation sequence
cd "thirdparty/gptp"

# Build for Milan testing (CRITICAL: use build_gptp directory)
cmake --build build_gptp --config Debug

# Run Milan compliance validation
.\test_milan_compliance_validation.ps1  # Expects build_gptp/Debug/gptp.exe

# Check specific Milan requirements
Select-String "neighborPropDelay" .\build_gptp\Debug\milan_test_output.log
Select-String "asCapable.*true" .\build_gptp\Debug\milan_test_output.log
```

#### **ASIO Integration Testing:**
```powershell
# Multi-adapter daemon startup
cd examples/asio-listener
.\start_openavnu_final.ps1

# Test ASIO listener with hardware validation
cd ../..
.\test_asio_multiadapter_cmake.ps1 -Verbose -HardwareValidation
```

### **Common Error Examples and Solutions:**

#### **Build Directory Confusion:**
```
ERROR: test_milan_compliance_validation.ps1 cannot find gptp.exe
CAUSE: Script expects build_gptp/Debug/ but executable in build/thirdparty/gptp/Debug/
SOLUTION: Always use correct build directory for Milan testing:
  cmake --build build_gptp --config Debug  # For Milan tests
  cmake --build build --config Release --target gptp2  # For main integration
```

#### **Hardware Timestamping Privilege Error:**
```
ERROR: Hardware timestamping initialization failed
CAUSE: Insufficient privileges for Intel hardware access
SOLUTION: Run PowerShell as Administrator:
  Start-Process powershell -Verb RunAs
  cd "path\to\OpenAvnu-2"
  .\test_milan_compliance_validation.ps1
```

#### **PCAP Detection Issues:**
```
ERROR: CMake cannot find PCAP libraries
CAUSE: Environment variables not set correctly
SOLUTION: Set proper PCAP paths:
  $env:NPCAP_DIR="C:\npcap-sdk"  # For modern Npcap
  $env:WPCAP_DIR="C:\WpdPack"    # For legacy WinPcap fallback
```

### **🔧 Automation Scripts Inventory:**

#### **Main Repository Scripts (Root Level):**
- **Deployment**: `create_deployment_package*.ps1` - Production packaging
- **Testing**: `test_*.ps1` - Component integration tests  
- **ASIO Integration**: `debug_asio_integration.ps1`, `test_asio_*` - Audio integration
- **Network Validation**: `verify_milan_*.ps1`, `diagnose_ptp_*.ps1` - Network testing
- **Build Automation**: VS Code tasks.json with comprehensive build/test workflows

#### **gPTP Submodule Scripts (thirdparty/gptp/):**
- **🎯 KEY**: `test_milan_compliance_validation.ps1` - Main validation (expects build_gptp/Debug/)
- **Profile Testing**: `test_automotive_profile.ps1`, `run_admin_milan.ps1`
- **Hardware Validation**: `verify_pdelay_exchange.ps1`, `test_privileges.ps1`
- **Diagnostics**: `simple_intel_diagnosis.ps1`, `final_intel_diagnosis*.ps1`

#### **ASIO Integration Scripts (examples/asio-listener/):**
- **Daemon Management**: `start_openavnu_final.ps1` - Multi-adapter daemon startup
- **Testing**: `start_openavnu_corrected.ps1` - I219-specific testing

#### **Hardware HAL Scripts (thirdparty/intel-ethernet-hal/):**
- **Build**: `build_windows.bat` - Windows build automation

### **⚠️ CRITICAL Build Directory Rules:**

1. **Milan Testing**: ALWAYS use `thirdparty/gptp/build_gptp/` directory
   ```powershell
   cd "thirdparty/gptp"
   cmake --build build_gptp --config Debug
   .\test_milan_compliance_validation.ps1  # Expects build_gptp/Debug/
   ```

2. **Main Integration**: Use main `build/` for OpenAvnu integration
   ```powershell
   cmake --build build --config Release --target gptp2  # Main integration
   ```

3. **Log Management**: Scripts create logs in their respective build directories
   - `build_gptp/Debug/milan_test_output.log` - Milan test logs
   - `docs/tests/results/` - Main repository test results
   - Component-specific logs in respective build directories

### **🔄 Script Dependencies and Execution Order:**

1. **Prerequisites**: Administrator privileges for hardware timestamping
2. **Build Order**: Components → Integration → Testing
3. **Configuration**: Scripts expect config files in build output directories
4. **Network Testing**: Requires actual Milan-certified devices for full validation

## CRITICAL: IEEE Standards Reference Documents

**IMPORTANT**: The following authoritative specification documents are available via MCP-Server "markitdown_standards" for compliance verification:

### IEEE Protocol Standards:
- `file:///workdir/IEEE 1722.1-2021-en.pdf` - AVDECC Device Control (Latest)
- `file:///workdir/IEEE 1722.1-2013-en.pdf` - AVDECC Device Control (Legacy) 
- `file:///workdir/IEEE 1722-2016-en.pdf` - Audio Video Transport Protocol (AVTP)
- `file:///workdir/ISO-IEC-IEEE 8802-1Q-2020-en.pdf` - VLAN/QoS Standards
- `file:///workdir/ISO-IEC-IEEE 8802-1BA-2016-en.pdf` - Audio Video Bridging (AVB) Profiles

### AVnu Alliance Milan Standards:
- `file:///workdir/Milan_Specification_Consolidated_v1.2_Final_Approved-20231130.pdf` - Milan v1.2 (Current)
- `file:///workdir/Milan-Discovery-Connection-Control-Talkers-And-Listener-Specification-1.1a_20190122.pdf` - Milan Discovery/Control

**LICENSE NOTICE**: These documents are referenced for compliance only. DO NOT copy content directly into repository - license restrictions apply. Use only for implementation guidance and verification.

**Usage**: When implementing standards-related code, reference these documents via MCP-Server to ensure:
- Protocol message format compliance
- State machine behavior correctness  
- Timing requirement adherence
- Milan professional interoperability
- IEEE specification conformance

## CRITICAL: Layered Architecture Principles

### **📋 Architecture Overview**
OpenAvnu follows a strict 3-layer architecture to ensure maintainability, testability, and hardware abstraction:

```
┌─────────────────────────────────────────────────────┐
│ Standards Layer (lib/Standards/)                    │
│ • Hardware agnostic protocol implementations        │
│ • IEEE 802.1AS, IEEE 1722, AVnu Milan profiles     │
│ • Uses dependency injection for hardware access     │
└─────────────────────────────────────────────────────┘
                              ↕
┌─────────────────────────────────────────────────────┐
│ Service Layer (e.g., gPTP daemon)                  │
│ • Bridges Standards to Hardware                     │
│ • Routes protocol needs to hardware capabilities    │
│ • Handles configuration and lifecycle management    │
└─────────────────────────────────────────────────────┘
                              ↕
┌─────────────────────────────────────────────────────┐
│ Intel Hardware Layer (intel-ethernet-hal)          │
│ • Direct Intel NIC hardware access                 │
│ • Timestamping, register access, device control    │
│ • IOCTL interface to NDIS driver                   │
└─────────────────────────────────────────────────────┘
```

### **🎯 Standards Layer Rules - MUST BE HARDWARE AGNOSTIC**
- **ONLY pure protocol implementations** (IEEE, AVnu, AES, etc.)
- **OS and hardware agnostic** - compilable without any specific hardware drivers
- **Mockable/testable** without hardware present
- **NEVER includes vendor-specific headers** (intel-ethernet-hal, intel_avb, etc.)
- **Uses dependency injection** - receives hardware abstraction via interfaces/function pointers
- **Responsibility**: Protocol logic, state machines, packet formats, timing calculations, standard-defined structures

### **🔌 Service Layer Rules - BRIDGES STANDARDS TO HARDWARE**
- **Uses Standards** for protocol logic implementation
- **Connects to Intel HAL** for hardware access via intel-ethernet-hal submodule
- **Bridges Standards interfaces to Intel Hardware interfaces**
- **Responsibility**: Routing timestamping requests, connecting protocol needs to hardware capabilities

### **⚙️ Intel Hardware Layer Rules - HARDWARE ACCESS ONLY**
- **CRITICAL**: Use intel-ethernet-hal submodule (thirdparty/intel-ethernet-hal) as PRIMARY hardware interface
- **NOT lib/common/hal/**: This is generic abstraction, USE Intel-specific submodules instead
- **Standards should NEVER directly touch Intel Hardware Layer**
- **Responsibility**: Intel NIC hardware abstraction, timestamping, register access, device control

### **🔗 Intel Hardware Access Chain (CRITICAL ORDER - MANDATORY):**
```
intel-ethernet-hal → intel_avb → NDISIntelFilterDriver → hardware
```
- **intel-ethernet-hal** (thirdparty/intel-ethernet-hal): PRIMARY Intel abstraction layer - USE THIS
- **intel_avb** (lib/intel_avb): Hardware access library with IOCTL interface to NDIS driver
- **NDISIntelFilterDriver**: Kernel-mode filter driver for actual hardware access
- **Intel register access**: ONLY through this complete chain
- **Functions like intel_hal_get_tx_timestamp()**: ONLY called within Intel HAL layer

### **❌ ARCHITECTURAL VIOLATIONS TO AVOID:**
1. **Standards contamination**: Direct hardware calls in Standards layer
2. **Wrong abstraction direction**: HAL using Standards instead of Services using both
3. **Redundant abstractions**: Duplicating existing intel-ethernet-hal functionality
4. **Direct vendor coupling**: Standards including vendor-specific headers
5. **⚠️ CRITICAL: Using lib/common/hal/ for Intel hardware** - USE intel-ethernet-hal submodule instead!

### **🔧 BEFORE MAKING CHANGES:**
1. **Understand call sites**: Find all places calling a function before moving it
2. **Respect existing APIs**: Use intel-ethernet-hal and intel_avb as-is - DO NOT use lib/common/hal/ for Intel hardware
3. **Create proper interfaces**: Abstract hardware access through dependency injection
4. **Move hardware code**: From Standards to Intel Hardware Layer (intel-ethernet-hal submodule)
5. **⚠️ NEVER use lib/common/hal/ for Intel NICs**: Always use thirdparty/intel-ethernet-hal submodule

## ⚠️ CRITICAL: Common Architectural Mistakes to Avoid

### ❌ **WRONG: Using lib/common/hal/ for Intel Hardware**
```cpp
// WRONG - Generic abstraction that bypasses Intel submodules
#include "../../common/hal/network_hal.h"
if (network_hal_initialize() != 0) { /* ... */ }
```

### ✅ **CORRECT: Using Intel-Ethernet-HAL Submodule**
```cpp
// CORRECT - Direct Intel HAL integration
#include "../../../thirdparty/intel-ethernet-hal/include/intel_ethernet_hal.h"
#include "../../intel_avb/include/avb_ioctl.h"

intel_hal_result_t result = intel_hal_init();
intel_hal_enumerate_devices(devices, &count);
intel_hal_open_device(device_id, &device);
```

### ❌ **WRONG: Standards Layer Hardware Contamination**
```cpp
// WRONG - Standards directly including hardware headers
#include "intel_ethernet_hal.h"  // ❌ NO! Standards must be hardware agnostic
```

### ✅ **CORRECT: Service Layer Hardware Bridge**
```cpp
// CORRECT - Only Service Layer touches hardware
// Standards Layer: Pure protocol logic with dependency injection
// Service Layer: Bridges Standards to Intel Hardware via function pointers
```

### **Key Rule**: 
**Intel Hardware Integration Chain**: `intel-ethernet-hal → intel_avb → NDISIntelFilterDriver → hardware`  
**NEVER**: `lib/common/hal/` for Intel hardware - this bypasses the proper chain!

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
