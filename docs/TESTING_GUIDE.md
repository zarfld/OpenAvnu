# OpenAvnu Testing Guide

## Overview

OpenAvnu includes a comprehensive testing framework that covers different aspects of the AVB/TSN implementation:

1. **Software Tests** - Unit tests, protocol compliance, integration tests
2. **Hardware Tests** - Tests that require real Intel network adapters
3. **Performance Tests** - Timing and performance validation tests

## Test Categories

### ✅ Software Tests (Always Run)
- **CppUTest Suite** - Unit tests for core functionality
- **Protocol Compliance** - AVDECC/AVTP protocol validation
- **Integration Tests** - Component interaction testing
- **AVTP Profile Validation** - Profile implementation verification

### 🔧 Hardware Tests (Require Real Hardware)
- **Intel HAL Validation** - Tests for Intel I210/I219/I225/I226 adapters
- **Timestamp Analysis** - Hardware timestamp precision validation
- **Performance Benchmarks** - Real-world performance measurements

## Running Tests

### Local Development (Full Test Suite)
```bash
# Build and run all tests (including hardware tests)
cd build
cmake --build . --config Release
ctest -C Release --output-on-failure
```

### CI/CD Environment (Software Tests Only)
```bash
# Automatically skips hardware tests in CI environments
cd build
ctest -C Release --output-on-failure
```

### Selective Test Execution
```bash
# Run only software tests
ctest -C Release -E "(IntelHalValidation|TimestampAnalysisSuite|IntelHALValidation)"

# Run only hardware tests (requires Intel adapters)
ctest -C Release -R "(IntelHalValidation|TimestampAnalysisSuite|IntelHALValidation)"

# Run by category
ctest -C Release -L conformance    # Conformance tests only
ctest -C Release -L integration    # Integration tests only
ctest -C Release -L performance    # Performance tests only
```

## CI Environment Behavior

The testing framework automatically detects CI environments (GitHub Actions, Travis, AppVeyor) and:

- ✅ **Runs all software tests** that don't require hardware
- ⏭️ **Skips hardware-dependent tests** that would fail on virtual machines
- 📝 **Provides clear feedback** about which tests are skipped and why

### Environment Variables Detected
- `CI=true`
- `GITHUB_ACTIONS=true`
- `TRAVIS=true`
- `APPVEYOR=true`

## Hardware Requirements

### Intel Ethernet Adapters
Hardware tests require one of these Intel adapters:
- Intel I210 Gigabit Network Connection
- Intel I219 Gigabit Network Connection  
- Intel I225 2.5G Network Connection
- Intel I226 2.5G Network Connection

### Driver Requirements
- **Windows**: Intel Ethernet adapter drivers with hardware timestamping support
- **Linux**: igb or e1000e kernel modules with PTP support

## Test Results

### Success Indicators
- **96%+ pass rate** is normal (some hardware tests may fail without real adapters)
- **100% pass rate** for software-only tests in CI environments
- **All categories passing** indicates full system health

### Common Issues
- **Hardware tests failing**: Check if Intel adapters are present and drivers installed
- **Timestamp tests failing**: Verify hardware timestamping capability
- **Permission errors**: Some tests may require administrator/root privileges

## Configuration Options

### CMake Options
```bash
# Disable specific test categories
cmake -DOPENAVNU_BUILD_HARDWARE_TESTS=OFF ..
cmake -DOPENAVNU_BUILD_PERFORMANCE_TESTS=OFF ..
cmake -DOPENAVNU_BUILD_CONFORMANCE_TESTS=OFF ..
cmake -DOPENAVNU_BUILD_INTEGRATION_TESTS=OFF ..
```

### Environment Override
```bash
# Force hardware test execution in CI (not recommended)
export OPENAVNU_FORCE_HARDWARE_TESTS=1

# Explicitly mark as non-CI environment  
unset CI GITHUB_ACTIONS TRAVIS APPVEYOR
```

## Troubleshooting

### Build Issues
1. Ensure all submodules are initialized: `git submodule update --init --recursive`
2. Verify PCAP library installation (Npcap on Windows)
3. Check Intel HAL dependencies for hardware tests

### Test Failures
1. **Software test failures**: Usually indicate real bugs that need fixing
2. **Hardware test failures**: Check adapter presence and driver status
3. **Timeout issues**: Increase test timeout values in CMakeLists.txt

### Debug Mode
```bash
# Run tests with verbose output
ctest -C Release --output-on-failure --verbose

# Run specific test with debug info
ctest -C Release -R "TestName" --verbose
```

## Integration with IDEs

### VS Code
Use the provided tasks:
- `CMake Test (Windows)` - Run all tests
- `CMake Test Debug (Windows)` - Run tests in debug mode

### Command Line
```bash
# Quick test after build
make test              # Linux/WSL
cmake --build . --target RUN_TESTS  # Windows
```

## Reporting Issues

When reporting test failures, include:
1. **Test environment**: OS, hardware, CI vs local
2. **Failed test names**: Specific tests that failed
3. **Hardware configuration**: Intel adapter models if relevant
4. **Build configuration**: CMake options used
5. **Full test output**: Complete ctest output with `--verbose`

This ensures proper categorization of issues as software bugs vs hardware environment problems.
