# Generic Intel Adapter Testing Framework

## Overview

This comprehensive testing framework validates **ALL** supported Intel Ethernet adapters on Windows 10 and Windows 11 systems. Following repository rules, it provides generic testing applicable across all Intel adapter types with comprehensive hardware validation.

## Supported Hardware

### Intel I210 Family
- **I210 Copper** (0x1533)
- **I210-T1** (0x1536) 
- **I210-IS** (0x1537)

### Intel I219 Family  
- **I219-LM** (0x15B7, 0x15D7)
- **I219-V** (0x15B8, 0x15D6, 0x15D8)
- **I219-LM Gen 22** (0x0DC7) ← Most common on modern systems

### Intel I225 Family
- **I225-LM** (0x15F2)
- **I225-V** (0x15F3)

### Intel I226 Family
- **I226-LM** (0x125B)
- **I226-V** (0x125C)

## Windows Support

- ✅ **Windows 10** (Build 19041+)
- ✅ **Windows 11** (All builds)
- ✅ **Windows Server 2019/2022**

## Test Phases

### Phase 1: Detection and Enumeration
- Adapter discovery and identification
- Driver validation
- Capability mapping
- Hardware feature detection

### Phase 2: Register Access
- **I210**: MMIO register access testing
- **I219**: MDIO register access testing  
- **I225/I226**: TSN register access testing
- Hardware communication validation

### Phase 3: Timestamping Capabilities
- Hardware timestamping support
- Precision measurement validation
- Capability-specific testing

### Phase 4: gPTP Integration
- gPTP daemon startup testing
- Intel HAL integration validation
- Cross-timestamping functionality

### Phase 5: Performance and Compliance
- Performance baseline measurement
- TSN profile compliance testing
- Latency and precision validation

## Usage

### Quick Start
```powershell
# Run all tests on all detected adapters
.\generic_adapter_testing_framework.ps1

# List available adapters without testing
.\generic_adapter_testing_framework.ps1 -ListAdapters

# Test specific adapter family
.\generic_adapter_testing_framework.ps1 -AdapterFilter "I219"

# Run specific test phase
.\generic_adapter_testing_framework.ps1 -TestPhase "detection"

# Verbose output with result saving
.\generic_adapter_testing_framework.ps1 -Verbose -SaveResults
```

### Advanced Usage
```powershell
# Continue testing even if phases fail
.\generic_adapter_testing_framework.ps1 -ContinueOnFailure

# Dry run (detection only, no actual testing)
.\generic_adapter_testing_framework.ps1 -DryRun

# Custom results path
.\generic_adapter_testing_framework.ps1 -ResultsPath "my_test_results.json"

# Test only register access phase with verbose output
.\generic_adapter_testing_framework.ps1 -TestPhase "register" -Verbose
```

## Parameters

| Parameter | Description | Values |
|-----------|-------------|--------|
| `TestPhase` | Which test phase to run | `all`, `detection`, `register`, `timestamp`, `gptp`, `performance` |
| `AdapterFilter` | Filter by adapter family | `I210`, `I219`, `I225`, `I226` |
| `Verbose` | Enable detailed output | Switch |
| `SaveResults` | Save test results to JSON | Switch (default: true) |
| `ContinueOnFailure` | Continue testing after failures | Switch |
| `ResultsPath` | Custom path for results file | String |
| `DryRun` | Run detection only | Switch |
| `ListAdapters` | Show detected adapters and exit | Switch |

## Output and Results

### Console Output
- Real-time test progress with color-coded results
- Detailed error information with verbose mode
- Summary report with recommendations
- Adapter-specific capability information

### JSON Results File
Results are automatically saved to `docs/tests/results/` with:
- Test execution metadata
- Per-adapter test results
- Performance measurements
- Error details and recommendations
- System information

### Example Output
```
================================================================================
 Generic Intel Adapter Testing Framework
================================================================================
Testing ALL supported Intel adapters on Windows 10/11

System Information:
  OS: Microsoft Windows 11 Pro
  Version: 10.0.22621
  Build: 22621
  Architecture: 64-bit

------------------------------------------------------------
 Detecting Intel Network Adapters
------------------------------------------------------------
Found 2 supported Intel adapters

Detected Intel Adapters:
Name   Family DeviceId Status Speed     Driver HW Timestamp Advanced TSN
----   ------ -------- ------ -----     ------ ------------ ------------
Eth1   I219   0x0DC7   Up     1000 Mbps 12.3.0 ✅           ❌
Eth2   I225   0x15F3   Up     2500 Mbps 12.3.0 ✅           ✅

------------------------------------------------------------
 Phase 1: Adapter Detection and Enumeration
------------------------------------------------------------
✅ PASSED - Basic Adapter Enumeration - Found 2 adapters
✅ PASSED - Family Validation - Adapter family: I219 [Eth1]
✅ PASSED - Family Validation - Adapter family: I225 [Eth2]
✅ PASSED - Driver Validation - Driver version: 12.3.0 [Eth1]
✅ PASSED - Driver Validation - Driver version: 12.3.0 [Eth2]
✅ PASSED - Capability Mapping - Capabilities: BASIC_1588, MDIO, NATIVE_OS [Eth1]
✅ PASSED - Capability Mapping - Capabilities: BASIC_1588, ENHANCED_TS, TSN_TAS, TSN_FP [Eth2]
```

## Integration with Repository Rules

### Following Repository Guidelines
- ✅ **Real-World Testing**: No simulation, actual hardware validation
- ✅ **Generic Implementation**: Works across ALL supported adapter types
- ✅ **Windows 10/11 Support**: Full compatibility with both Windows versions
- ✅ **Documentation**: Comprehensive setup and usage documentation
- ✅ **Result Logging**: Automatic test result saving and reporting

### Test Documentation
Results are saved to `docs/tests/results/` following repository structure:
- Timestamp-based file naming
- JSON format for programmatic processing
- Detailed error logging for troubleshooting
- System information for reproducibility

## Prerequisites

### Hardware Requirements
- Intel Ethernet adapter (I210/I219/I225/I226 series)
- Windows 10 (Build 19041+) or Windows 11
- Administrator privileges for hardware access

### Software Requirements  
- PowerShell 5.1 or later
- Intel Ethernet drivers installed
- OpenAvnu built with Intel HAL support (optional but recommended)

### Optional Components
- Intel HAL validation test (`tests/intel_hal/intel_hal_validation_test.exe`)
- Performance testing suite (`testing/performance/`)
- gPTP daemon (`build/Release/daemon_cl.exe`)

## Troubleshooting

### Common Issues

#### No Adapters Found
```
❌ No supported Intel adapters found. Exiting.
```
**Solution**: 
- Verify Intel adapter is installed and recognized by Windows
- Check Device Manager for adapter status
- Ensure Intel drivers are properly installed

#### Register Access Failed
```
❌ FAILED - I219 MDIO Access - Exception: Access denied
```
**Solution**:
- Run PowerShell as Administrator
- Verify adapter drivers are properly installed
- Check Windows security policies

#### gPTP Integration Failed
```
❌ FAILED - gPTP Startup - Daemon initialization [Eth1]
```
**Solution**:
- Build OpenAvnu with gPTP support
- Verify gPTP configuration file
- Check Intel HAL integration

### Verbose Mode
Use `-Verbose` parameter for detailed diagnostic information:
```powershell
.\generic_adapter_testing_framework.ps1 -Verbose
```

### Hardware-Specific Issues

#### I210 Adapters
- May require Intel PROSet for advanced features
- MMIO access requires proper driver installation
- Some I210 variants have limited OID support

#### I219 Adapters  
- MDIO access is family-specific
- Ensure latest Intel drivers for optimal support
- Registry-based capability detection

#### I225/I226 Adapters
- Full TSN support requires Windows 10 2004+ or Windows 11
- Advanced features may need Intel-specific drivers
- 2.5G support depends on system capabilities

## CI/CD Integration

### Automated Testing
The framework integrates with VS Code tasks and CI/CD pipelines:

```json
{
    "label": "Run Generic Adapter Tests",
    "type": "shell",
    "command": "powershell",
    "args": [
        "-ExecutionPolicy", "Bypass",
        "-File", "testing/unified/generic_adapter_testing_framework.ps1",
        "-SaveResults", "-ContinueOnFailure"
    ],
    "group": "test"
}
```

### GitHub Actions Integration
```yaml
- name: Test Intel Adapters
  run: |
    powershell -ExecutionPolicy Bypass -File testing/unified/generic_adapter_testing_framework.ps1 -SaveResults
  continue-on-error: true
```

## Extension and Customization

### Adding New Adapter Support
1. Update `$SUPPORTED_ADAPTERS` hashtable with new device IDs
2. Add family-specific test functions
3. Update capability detection logic
4. Add documentation to this README

### Custom Test Phases
Create new test phase functions following the pattern:
```powershell
function Test-CustomPhase {
    param([array]$Adapters)
    # Implementation
}
```

### Configuration Files
Optional JSON configuration for custom test parameters:
```json
{
    "test_timeout": 30,
    "precision_threshold": 1000,
    "performance_iterations": 1000
}
```

## Related Documentation

- [Intel HAL Validation Test Suite](../intel_hal/README.md)
- [Hardware Testing Setup](../setup.md)
- [Performance Testing Framework](../../performance/README.md)
- [gPTP Integration Guide](../../../docs/gPTP_Integration.md)

## Contributing

When modifying this framework:
1. Ensure compatibility with all supported adapter families
2. Test on both Windows 10 and Windows 11
3. Follow repository documentation guidelines
4. Add appropriate error handling and logging
5. Update this README with any new features

## Version History

- **v1.0**: Initial generic framework supporting I210/I219/I225/I226
- **v1.1**: Added advanced TSN testing for I225/I226
- **v1.2**: Enhanced Windows 11 compatibility and performance testing
