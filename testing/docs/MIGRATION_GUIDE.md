# OpenAvnu Testing Directory Migration Guide

## Overview

The OpenAvnu testing structure has been consolidated from scattered directories into a unified `testing/` framework for better organization and maintainability.

## Directory Changes

### Old Structure → New Structure

| Old Location | New Location | Status |
|--------------|-------------|---------|
| `test/clock_quality/` | `testing/conformance/avnu_alliance/gptp/clock_quality/` | ✅ Migrated |
| `tests/intel_hal/` | `testing/hardware/intel/hal/` | ✅ Migrated |
| `test_phase2/` | `testing/integration/phase2/` | ✅ Migrated |
| `phase3_demo/` | `testing/integration/phase3/` | ✅ Migrated |
| `tools/timestamp_analysis/` | `testing/performance/timestamp_analysis/automated_suite/` | ✅ Migrated |
| `test/*.py` | `testing/tools/python/` | ✅ Migrated |
| `tests/results/` | `testing/results/hardware/` | ✅ Migrated |

## Build System Changes

### CMake Configuration

#### New Unified Option (Recommended)
```cmake
# Enable all testing
cmake .. -DOPENAVNU_BUILD_TESTING=ON
```

#### Legacy Options (Still Supported)
```cmake
# Individual test categories (deprecated)
cmake .. -DOPENAVNU_BUILD_CLOCK_QUALITY_TESTS=ON
cmake .. -DOPENAVNU_BUILD_INTEL_HAL=ON
```

### New Test Targets
```bash
# Build specific test categories
make test_conformance
make test_hardware  
make test_integration
make test_performance

# Build all tests
make test_all

# Run with CTest
ctest -L conformance
ctest -L hardware
ctest --output-on-failure
```

## VS Code Integration Updates

### Updated Task Names
- `Build Clock Quality Tests` → Works with both old and new locations
- `Run Intel HAL Tests` → Works with both old and new locations
- `Run All OpenAvnu Tests` → Includes unified testing framework

### Updated Keyboard Shortcuts
- **Ctrl+Shift+T Q** - Clock quality tests (any location)
- **Ctrl+Shift+T H** - Intel HAL tests (any location)
- **Ctrl+Shift+T Shift+A** - All tests (unified framework)

## CI/CD Integration Updates

### GitHub Actions Workflow Changes
The workflow now supports both old and new test locations:
```yaml
# Tries new location first, falls back to old location
- name: Run Clock Quality Tests
  run: |
    if (Test-Path "build/testing/conformance/avnu_alliance/gptp/clock_quality/Release/clock_quality_tests.exe") {
      & "build/testing/conformance/avnu_alliance/gptp/clock_quality/Release/clock_quality_tests.exe"
    } else {
      & "build/test/clock_quality/Release/clock_quality_tests.exe"
    }
```

## Developer Migration Steps

### For New Development
1. **Use the `testing/` directory** for all new tests
2. **Follow the category structure**:
   - Conformance tests → `testing/conformance/`
   - Hardware tests → `testing/hardware/`
   - Integration tests → `testing/integration/`
   - Performance tests → `testing/performance/`

### For Existing Development
1. **Update local paths** in scripts and documentation
2. **Use new CMake options** when configuring builds
3. **Update IDE configurations** to reference new paths

### Path Updates Required

#### Build Scripts
```bash
# Old
./build/test/clock_quality/Release/clock_quality_tests.exe

# New
./build/testing/conformance/avnu_alliance/gptp/clock_quality/Release/clock_quality_tests.exe
```

#### CMake References
```cmake
# Old
add_subdirectory("test/clock_quality")

# New
add_subdirectory("testing/conformance/avnu_alliance/gptp/clock_quality")
```

#### Documentation Links
```markdown
# Old
[Clock Quality Tests](test/clock_quality/README.md)

# New  
[Clock Quality Tests](testing/conformance/avnu_alliance/gptp/clock_quality/README.md)
```

## Backward Compatibility

### Transition Period
- **Old directories remain** with deprecation notices
- **Both old and new paths work** in build system
- **CI/CD supports both locations** automatically

### Deprecation Timeline
1. **Phase 1 (Current)** - Both old and new structures supported
2. **Phase 2 (2 weeks)** - Old structure marked deprecated
3. **Phase 3 (4 weeks)** - Old structure removed (optional)

## Benefits of Migration

### For Developers
- **Clear organization** - Know exactly where to find/add tests
- **Unified build system** - Single CMake configuration for all tests
- **Better documentation** - Centralized testing guides
- **Simplified workflows** - Consistent test execution

### For CI/CD
- **Single test path** - Easier workflow configuration
- **Category-based testing** - Run specific test types
- **Better reporting** - Organized result storage
- **Easier maintenance** - Fewer paths to manage

## Troubleshooting

### Build Issues
```bash
# Clear CMake cache after migration
rm -rf build/CMakeCache.txt
cmake .. -DOPENAVNU_BUILD_TESTING=ON
```

### Path Not Found Errors
```bash
# Check if using old paths
grep -r "test/clock_quality" .
grep -r "tests/intel_hal" .

# Update to new paths
sed -i 's|test/clock_quality|testing/conformance/avnu_alliance/gptp/clock_quality|g' file.txt
```

### VS Code Issues
1. Reload VS Code after CMake changes
2. Update `.vscode/settings.json` if using custom paths
3. Clear VS Code workspace cache if needed

## Migration Validation

### Verify Migration Success
```bash
# Check new structure exists
ls -la testing/conformance/avnu_alliance/gptp/clock_quality/
ls -la testing/hardware/intel/hal/

# Test builds work
cmake --build build --target test_conformance
cmake --build build --target test_hardware

# Test execution works
ctest -L conformance
ctest -L hardware
```

### Rollback if Needed
```bash
# Disable unified testing
cmake .. -DOPENAVNU_BUILD_TESTING=OFF -DOPENAVNU_BUILD_CLOCK_QUALITY_TESTS=ON

# Use old build targets
cmake --build build --target clock_quality_tests
```

## Support

For migration issues:
1. Check this guide for common problems
2. Verify CMake configuration with `cmake .. -L`
3. Test with both old and new options
4. Open GitHub issue if problems persist

The migration improves OpenAvnu's testing organization while maintaining full backward compatibility during the transition period.
