# Hardware Testing Setup Documentation

**Date**: July 11, 2025  
**Purpose**: Systematic real hardware testing plan for OpenAvnu

---

## Testing Requirements

All new features and bug fixes must include unit tests with at least 80% coverage.
Integration tests must cover all critical workflows and interactions between components.
Test results must be stored in `docs/tests/results/` with date and time of test runs.

## Testing Environment

Before running tests, ensure the environment is set up correctly:
- Use `docs/machine/` for machine-specific information
- Respect machine capabilities and limitations  
- Document any specific configurations or dependencies required for testing

---

## Test Cases and Expected Outcomes

### Phase 1: Network Interface Detection and Basic Communication
**Objective**: Verify all Intel NICs are properly detected and accessible

#### Test Cases:
1. **Hardware Detection Test**
   - Detect all Intel I210/I219/I225/I226 adapters
   - Verify PCI device IDs and driver versions
   - Check adapter status and capabilities

2. **Basic Network Communication Test**
   - Ping connectivity between adapters
   - Basic Ethernet frame transmission/reception
   - Link speed and duplex verification

3. **Driver Interface Test**
   - NDIS interface functionality (Windows)
   - PTP device access (Linux)
   - Register access capabilities

**Expected Results**: All adapters detected, basic communication established

### Phase 2: Register Access and Hardware Interface Testing
**Objective**: Verify low-level hardware register access and control

#### Test Cases:
1. **Register Read/Write Test**
   - Access IEEE 1588 timestamp registers
   - Test TSN-specific registers (I225/I226)
   - Verify register value persistence

2. **Hardware Capability Detection**
   - Query timestamp capabilities
   - Detect TSN feature support
   - Verify clock source availability

3. **Intel HAL Integration Test**
   - HAL device initialization
   - Capability enumeration
   - Error handling validation

**Expected Results**: Full register access, HAL integration functional

### Phase 3: Timestamp Functionality Testing
**Objective**: Validate hardware timestamping accuracy and precision

#### Test Cases:
1. **Basic Timestamp Test**
   - Enable hardware timestamping
   - Capture TX/RX timestamps
   - Compare hardware vs software timestamps

2. **Timestamp Accuracy Test**
   - Measure timestamp precision and jitter
   - Test under various network loads
   - Validate against reference clock

3. **Multi-Adapter Timestamp Sync**
   - Synchronize between multiple adapters
   - Cross-adapter timing comparison
   - Drift measurement over time

**Expected Results**: Sub-microsecond timestamp precision, stable synchronization

### Phase 4: gPTP Protocol Integration Testing
**Objective**: Verify gPTP daemon operation with real hardware

#### Test Cases:
1. **gPTP Daemon Basic Operation**
   - Start gPTP daemon with Intel HAL
   - Verify PTP message exchange
   - Monitor synchronization status

2. **Master-Slave Synchronization**
   - Configure master/slave roles
   - Measure synchronization accuracy
   - Test role switching scenarios

3. **Multi-Device Network Testing**
   - Connect multiple devices
   - Verify network-wide synchronization
   - Test with AVB-capable switches

**Expected Results**: Stable gPTP operation, network synchronization established

### Phase 5: Performance and Stress Testing
**Objective**: Validate performance under real-world conditions

#### Test Cases:
1. **Load Testing**
   - High packet rate scenarios
   - Multiple concurrent streams
   - Network congestion handling

2. **Long-Duration Testing**
   - 24+ hour stability tests
   - Drift measurement over time
   - Memory leak detection

3. **Interoperability Testing**
   - Test with professional AVB devices
   - Cross-vendor compatibility
   - Different network topologies

**Expected Results**: Stable operation under stress, no performance degradation

---

## Test Scenarios and Expected Results

### Scenario 1: Single Adapter Basic Validation
**Setup**: One Intel adapter, basic network connection
**Tests**: Detection, register access, basic timestamping
**Expected**: Full functionality, baseline performance established

### Scenario 2: Multi-Adapter Configuration
**Setup**: Multiple Intel adapters (I210, I219, I225)
**Tests**: Concurrent operation, cross-adapter sync, capability differences
**Expected**: Independent operation, synchronized timing

### Scenario 3: Real AVB Network Integration
**Setup**: Professional AVB devices (RME, Motu, Presonus)
**Tests**: Interoperability, stream establishment, audio/video streaming
**Expected**: Full AVB compatibility, professional-grade performance

### Scenario 4: Production Environment Simulation
**Setup**: Multiple devices, realistic network loads, long-duration
**Tests**: Stability, performance under load, fault tolerance
**Expected**: Production-ready reliability and performance

---

## Test Setup Instructions

### Hardware Requirements
- Intel I210/I219/I225/I226 network adapters
- AVB-capable Ethernet switches (optional)
- Professional AVB devices for interoperability testing
- High-precision timing reference (GPS, atomic clock)

### Software Requirements
- OpenAvnu build environment
- Python testing framework
- Network analysis tools (Wireshark, tcpdump)
- Performance monitoring tools

### Environment Configuration
1. **Network Setup**
   - Direct connections for basic testing
   - Switched network for multi-device testing
   - Isolated test network to avoid interference

2. **System Configuration**
   - Administrator/root privileges
   - Real-time scheduling capabilities
   - Network interface optimization

3. **Monitoring Setup**
   - Packet capture capabilities
   - Performance counter access
   - System resource monitoring

---

## Test Documentation Requirements

### Test Results Documentation
Store all test results in `docs/tests/results/` with:
- Date and time of test runs
- Complete system configuration
- Summary of results
- Any issues or failures encountered
- Performance metrics and measurements

### Machine-Specific Documentation
Update `docs/machine/` with:
- Exact hardware specifications
- Driver versions and configurations
- Known limitations or issues
- Optimization settings applied

### Test Automation
Automate tests using CI/CD pipelines where possible:
- All tests run automatically on pull requests
- Test results reported back to pull requests
- Regression detection and alerting

---

**Next Steps**: 
1. ✅ Complete machine documentation with actual hardware specs
2. ✅ Implement Phase 1 tests (network interface detection)
3. ✅ Validate test environment setup
4. ✅ **COMPLETED**: Create unified testing framework (`tests/hardware/unified_test_framework.ps1`)
5. 🔄 **CURRENT**: Consolidate legacy test scripts and update CI/CD workflows
6. 🔄 Test unified framework on DAW02 (I225/I210 hardware)
7. Execute systematic testing progression through remaining phases (3-5)

---

## 🚀 NEW: Unified Testing Framework

**Primary Testing Tool**: `tests/hardware/unified_test_framework.ps1`

### Quick Start
```powershell
# List available adapters
./tests/hardware/unified_test_framework.ps1 -ListAdapters

# Run all test phases with verbose output
./tests/hardware/unified_test_framework.ps1 -Phase all -Verbose -SaveResults

# Test specific adapter type
./tests/hardware/unified_test_framework.ps1 -Phase 1 -AdapterFilter "I219"

# Run specific phase
./tests/hardware/unified_test_framework.ps1 -Phase 2 -SaveResults
```

### Features
- **Multi-Adapter Support**: I210, I219, I225, I226
- **Phased Testing**: Progressive validation (Phase 1: Detection, Phase 2: Register Access)
- **Cross-System**: Works on DEV01, DAW02, and future systems
- **CI/CD Ready**: Integrated with GitHub Actions workflow
- **Comprehensive Logging**: JSON results and detailed console output

### Consolidation Status
- ✅ **Phase 1-2**: Implemented and tested on I219-LM
- 🔄 **Legacy Scripts**: Being moved to `tests/hardware/legacy/`
- 🔄 **GitHub Workflow**: Updated to use unified framework
- 📋 **Python Tools**: Integration planned for specialized analysis
