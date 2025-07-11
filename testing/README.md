# OpenAvnu Testing Framework

## Overview

This directory contains the comprehensive testing framework for OpenAvnu, consolidating all testing tools, conformance tests, hardware validation, and performance analysis into a unified structure.

## Directory Structure

```
testing/
├── conformance/           # Standards compliance testing
│   └── avnu_alliance/    # Avnu Alliance test suite
│       └── gptp/         # gPTP conformance tests
├── hardware/             # Hardware-specific validation
│   └── intel/           # Intel hardware tests
│       └── hal/         # Intel HAL validation
├── integration/          # System integration testing
│   ├── phase2/          # Phase 2 integration tests
│   └── phase3/          # Phase 3 production tests
├── performance/          # Performance and benchmarking
│   └── timestamp_analysis/ # Timestamp precision analysis
├── tools/               # Testing utilities and scripts
│   └── python/          # Python testing utilities
├── results/             # Test result storage
├── docs/                # Testing documentation
├── CMakeLists.txt       # Unified build configuration
└── README.md           # This file
```

## Quick Start

### Building All Tests
```bash
# From OpenAvnu root directory
mkdir -p build && cd build
cmake .. -DOPENAVNU_BUILD_TESTING=ON
cmake --build . --config Release
```

### Running Conformance Tests
```bash
# gPTP Clock Quality Tests
./testing/conformance/avnu_alliance/gptp/clock_quality/Release/clock_quality_tests.exe

# All conformance tests
ctest -L conformance
```

### Running Hardware Tests
```bash
# Intel HAL Validation
./testing/hardware/intel/hal/Release/intel_hal_validation_test.exe

# All hardware tests
ctest -L hardware
```

### VS Code Integration
Use these keyboard shortcuts for quick testing:
- **Ctrl+Shift+T Q** - Run clock quality tests
- **Ctrl+Shift+T H** - Run Intel HAL tests
- **Ctrl+Shift+T Shift+A** - Run all tests

## Test Categories

### 1. Conformance Testing (`conformance/`)
Standards compliance validation for:
- **Avnu Alliance gPTP Test Plan v1.0** - Complete conformance test suite
- **IEEE 802.1AS** - Timing and synchronization compliance
- **IEEE 1722.1 AVDECC** - Device discovery and control compliance

### 2. Hardware Testing (`hardware/`)
Device-specific validation for:
- **Intel HAL** - Hardware abstraction layer validation
- **I210/I219/I225** - Intel Ethernet controller testing
- **Generic hardware** - Cross-platform compatibility tests

### 3. Integration Testing (`integration/`)
System-level testing for:
- **Phase 2** - Multi-component integration
- **Phase 3** - Production system validation
- **End-to-end** - Complete AVB network testing

### 4. Performance Testing (`performance/`)
Benchmarking and analysis for:
- **Timestamp Analysis** - Precision timing measurement
- **Latency Measurement** - Network and processing delays
- **Throughput Benchmarking** - Data rate validation

## Test Execution

### Individual Test Suites
```bash
# Conformance tests
cd testing/conformance/avnu_alliance/gptp/clock_quality
./Release/clock_quality_tests.exe

# Hardware tests
cd testing/hardware/intel/hal
./Release/intel_hal_validation_test.exe
```

### Automated Test Execution
```bash
# Run all tests with CTest
ctest --output-on-failure

# Run specific test categories
ctest -L conformance
ctest -L hardware
ctest -L integration
ctest -L performance
```

### CI/CD Integration
Tests are automatically executed in GitHub Actions workflows:
- **Manual Test Workflow** - Manual trigger for comprehensive testing
- **Pull Request Validation** - Automated testing on code changes
- **Nightly Conformance** - Regular compliance validation

## Adding New Tests

### 1. Conformance Tests
Add to `conformance/avnu_alliance/gptp/` or appropriate standards directory.

### 2. Hardware Tests
Add to `hardware/[vendor]/[device_type]/` directory.

### 3. Integration Tests
Add to `integration/[phase]/` or create new integration category.

### 4. Performance Tests
Add to `performance/[test_type]/` directory.

### Build System Integration
Update `CMakeLists.txt` to include new test targets and add appropriate CTest labels.

## Documentation

- [`docs/conformance_guide.md`](docs/conformance_guide.md) - How to run conformance tests
- [`docs/hardware_testing.md`](docs/hardware_testing.md) - Hardware testing procedures
- [`docs/performance_analysis.md`](docs/performance_analysis.md) - Performance testing guide
- [`docs/ci_cd_integration.md`](docs/ci_cd_integration.md) - CI/CD testing documentation

## Dependencies

### Python Tools
```bash
cd testing/tools/python
pip install -r requirements.txt
```

### C++ Testing Framework
- **CppUTest** - Included in OpenAvnu thirdparty
- **Simple Test Framework** - Custom lightweight framework

### Hardware Requirements
- **Intel Ethernet Controllers** - I210, I219, I225, I226
- **AVB Network Setup** - For integration and interoperability testing
- **Precision Timing Hardware** - For timestamp analysis

## Contributing

1. **Follow the directory structure** - Place tests in appropriate categories
2. **Update CMakeLists.txt** - Include new tests in build system
3. **Add documentation** - Document test purpose and usage
4. **Include in CI/CD** - Ensure tests run in automated workflows
5. **Use consistent naming** - Follow existing test naming conventions

## Troubleshooting

### Build Issues
- Ensure all dependencies are installed
- Check CMake version (minimum 3.15 required)
- Verify Intel HAL is enabled: `-DOPENAVNU_BUILD_INTEL_HAL=ON`

### Test Failures
- Check hardware connectivity for hardware tests
- Verify network configuration for integration tests
- Review test logs in `results/` directory

### VS Code Integration
- Reload VS Code after CMake configuration changes
- Update `.vscode/tasks.json` for new test paths
- Check build directory is correctly set

## Support

For testing framework issues:
1. Check existing documentation in `docs/`
2. Review test logs in `results/`
3. Open GitHub issue with test output and configuration details
