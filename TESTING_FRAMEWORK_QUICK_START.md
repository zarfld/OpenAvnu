# OpenAvnu Testing Framework - Quick Start Guide

## Overview

The OpenAvnu testing framework has been consolidated into a unified directory structure for better organization, maintainability, and CI/CD integration.

## Quick Commands

### Build Configuration
```bash
# Configure with unified testing framework
cd build
cmake .. -G "Visual Studio 17 2022" -DOPENAVNU_BUILD_UNIFIED_TESTING=ON

# Build all tests
cmake --build . --config Release
```

### Running Tests

#### Individual Tests
```bash
# Clock Quality Tests (gPTP conformance)
./testing/conformance/avnu_alliance/gptp/clock_quality/Release/clock_quality_tests_consolidated.exe

# Intel HAL Tests (hardware validation)  
./tests/intel_hal_validation_test_consolidated.exe
```

#### CTest Integration
```bash
# Run tests by category
ctest -L conformance --output-on-failure
ctest -L hardware --output-on-failure
ctest -L integration --output-on-failure
ctest -L performance --output-on-failure

# Run all tests
ctest --output-on-failure
```

## Directory Structure

```
testing/
├── conformance/               # Standards compliance tests
│   └── avnu_alliance/gptp/
│       └── clock_quality/     # gPTP clock quality tests ✅
├── hardware/                  # Hardware-specific tests
│   └── intel/hal/             # Intel HAL validation tests ✅
├── integration/               # System integration tests
│   ├── phase2/                # Phase 2 integration tests
│   └── phase3/                # Phase 3 production tests
├── performance/               # Performance and timing tests
│   └── timestamp_analysis/    # Timestamp analysis suite
├── tools/                     # Testing utilities
│   └── python/                # Python testing tools
├── results/                   # Test result storage
└── docs/                      # Testing documentation
```

## VS Code Integration

### Available Tasks
- **Build Unified Testing Framework (Windows)** - Builds all test components
- **Run Clock Quality Tests (Consolidated)** - Executes gPTP conformance tests
- **Run Intel HAL Tests (Consolidated)** - Runs hardware validation tests
- **Run CTest Conformance Tests** - Category-based test execution
- **Run CTest Hardware Tests** - Hardware-specific test execution
- **CMake Configure with Unified Testing (Windows)** - Project configuration

### Task Execution
1. Open Command Palette (`Ctrl+Shift+P`)
2. Type "Tasks: Run Task"
3. Select desired testing task

## Test Status

### ✅ Working Tests
- **Clock Quality Tests**: 25/25 passing - Complete gPTP conformance validation
- **Intel HAL Tests**: 6/6 passing - I210/I219/I225/I226 hardware support validation

### 🔄 In Progress
- **Integration Tests**: CMake configured, dependency resolution in progress
- **Performance Tests**: Python-based, requires Python environment

## Configuration Options

### CMake Options
```cmake
# Enable/disable test categories
-DOPENAVNU_BUILD_CONFORMANCE_TESTS=ON    # Avnu Alliance conformance tests
-DOPENAVNU_BUILD_HARDWARE_TESTS=ON       # Hardware validation tests  
-DOPENAVNU_BUILD_INTEGRATION_TESTS=ON    # System integration tests
-DOPENAVNU_BUILD_PERFORMANCE_TESTS=ON    # Performance and timing tests

# Legacy support (deprecated)
-DOPENAVNU_BUILD_LEGACY_TESTS=OFF        # Use old directory structure
```

### Main Control
```cmake
# Primary control option (recommended)
-DOPENAVNU_BUILD_UNIFIED_TESTING=ON      # Enable new unified framework
```

## Development Workflow

### Adding New Tests
1. Choose appropriate category under `testing/`
2. Create subdirectory following naming convention
3. Add CMakeLists.txt with unique target name (add `_consolidated` suffix)
4. Register test with CTest using appropriate labels
5. Update documentation

### Example Test Addition
```cmake
# In testing/new_category/CMakeLists.txt
add_executable(my_new_test_consolidated my_test.cpp)

# In testing/CMakeLists.txt
add_test(NAME MyNewTest 
         COMMAND my_new_test_consolidated
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/new_category)
set_tests_properties(MyNewTest PROPERTIES 
                    LABELS "new_category;feature"
                    TIMEOUT 120)
```

## Troubleshooting

### Common Issues

#### CMake Configuration Fails
```bash
# Clean and reconfigure
rm -rf build
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -DOPENAVNU_BUILD_UNIFIED_TESTING=ON
```

#### Tests Not Found
```bash
# Verify test targets exist
cmake --build . --config Release --target clock_quality_tests_consolidated
cmake --build . --config Release --target intel_hal_validation_test_consolidated
```

#### Path Issues
- Use absolute paths in CMakeLists.txt: `${CMAKE_SOURCE_DIR}/path/to/source`
- Include directories set via `target_include_directories()` or `include_directories()`
- Avoid relative paths in source code includes

## Documentation

- **`docs/TESTING_CONSOLIDATION_COMPLETION_REPORT.md`** - Complete consolidation report
- **`testing/docs/MIGRATION_GUIDE.md`** - Developer migration guide  
- **`testing/README.md`** - Detailed testing framework documentation
- **`docs/TESTING_DIRECTORY_CONSOLIDATION_PLAN.md`** - Original planning document

## Migration Notes

### For Contributors
- Old test directories still exist but are deprecated
- New tests should use unified structure under `testing/`
- Existing VS Code tasks continue to work
- Target names have `_consolidated` suffix to avoid conflicts

### Legacy Support
- `OPENAVNU_BUILD_LEGACY_TESTS=ON` enables old structure
- Automatic conflict prevention when unified testing enabled
- Gradual migration path supported

## Success Metrics

- ✅ 25/25 clock quality tests passing
- ✅ 6/6 Intel HAL tests passing  
- ✅ Zero build conflicts
- ✅ Complete CMake integration
- ✅ VS Code task integration
- ✅ CTest categorization working

---

*For detailed implementation information, see the complete documentation in the `docs/` directory.*
