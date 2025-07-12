# OpenAvnu Testing Framework Migration Guide

**Date**: January 11, 2025  
**Updated**: July 11, 2025  
**Status**: ✅ **MIGRATION COMPLETE** - Repository cleanup finished  
**For**: Users of legacy testing scripts  
**Target**: Unified Testing Framework (`tests/hardware/unified_test_framework.ps1`)

---

## 🎉 Migration Status: COMPLETE

**Completed on July 11, 2025:**
- ✅ All legacy scripts moved to `docs/tests/legacy/scripts/`
- ✅ All scattered test results organized in `docs/tests/results/`
- ✅ Configuration files centralized in `docs/tests/config/`
- ✅ GitHub Actions workflows updated to use unified framework
- ✅ 33 total files organized (30 moved + 3 copied)

See the [Cleanup Summary](cleanup_summary.md) for complete details.

---

## 🔄 Migration Overview

The OpenAvnu project has consolidated multiple testing approaches into a single, unified framework. This guide helps you migrate from legacy scripts to the new unified testing system.

---

## Legacy Script Mapping

### Phase 1 Scripts → Unified Framework

| **Legacy Script** | **Unified Framework Equivalent** | **Migration Notes** |
|------------------|-----------------------------------|-------------------|
| `phase1_interface_detection.ps1` | `-Phase 1` | All functionality preserved |
| `phase1_network_communication.ps1` | `-Phase 1` | Connectivity testing included |
| `phase1_driver_interface.ps1` | `-Phase 1` | PTP validation enhanced |
| `run_phase1_simple.ps1` | `-Phase all` | More comprehensive |

### Register Testing Scripts → Unified Framework

| **Legacy Script** | **Unified Framework Equivalent** | **Migration Notes** |
|------------------|-----------------------------------|-------------------|
| `test_i225_register_access.ps1` | `-Phase 2 -AdapterFilter I225` | I225 specific testing |
| Manual register tests | `-Phase 2 -Verbose` | Hardware property validation |

### Python Analysis Tools → Hybrid Approach

| **Legacy Tool** | **New Approach** | **Migration Notes** |
|----------------|------------------|-------------------|
| `automated_test_suite.py` | Unified Framework + Python | Use PowerShell for detection, Python for analysis |
| `timestamp_analyzer.py` | Keep as specialized tool | Called by GitHub workflow after hardware validation |
| `performance_benchmark.py` | Keep as specialized tool | Future integration planned |

---

## Command Migration Examples

### Before (Legacy Scripts)
```powershell
# Old approach - multiple scripts
./phase1_interface_detection.ps1
./phase1_network_communication.ps1 -SkipConnectivity  
./phase1_driver_interface.ps1

# I225 specific testing
./test_i225_register_access.ps1 -Verbose
```

### After (Unified Framework)
```powershell
# New approach - single command
./tests/hardware/unified_test_framework.ps1 -Phase all -SkipConnectivity -Verbose -SaveResults

# I225 specific testing
./tests/hardware/unified_test_framework.ps1 -Phase 2 -AdapterFilter "I225" -Verbose
```

---

## GitHub Actions Migration

### Before (Legacy Workflow)
```yaml
- name: I225 Register Hardware Test
  shell: pwsh
  run: |
    ./test_i225_register_access.ps1 -Verbose -SaveLog

- name: Timestamp Automated Test Suite  
  run: |
    python tools/timestamp_analysis/automated_test_suite.py --verbose
```

### After (Unified Workflow)
```yaml
- name: Unified Hardware Testing Framework
  shell: pwsh
  run: |
    ./tests/hardware/unified_test_framework.ps1 -Phase all -Verbose -SaveResults

- name: Legacy I225 Test (if needed)
  shell: pwsh  
  run: |
    # Optional fallback for I225 specific validation
    if (Test-Path "./tests/hardware/legacy/test_i225_register_access.ps1") {
      ./tests/hardware/legacy/test_i225_register_access.ps1 -Verbose
    }
  continue-on-error: true
```

---

## Benefits of Migration

### ✅ Immediate Benefits
- **Single Command**: One script replaces multiple legacy scripts
- **Better Detection**: Enhanced adapter detection and capability validation
- **Consistent Results**: Standardized JSON output format
- **Cross-System**: Same commands work on different machines (DEV01, DAW02)

### 📈 Long-term Benefits  
- **Easier Maintenance**: One codebase instead of multiple scripts
- **Enhanced Coverage**: More comprehensive testing with same effort
- **Future Proof**: New adapter support added to single framework
- **CI/CD Integration**: Better GitHub Actions integration

---

## Migration Checklist

### For Individual Users
- [ ] Replace manual script calls with unified framework
- [ ] Update any custom scripts to call unified framework
- [ ] Test new commands on your hardware
- [ ] Validate results format meets your needs
- [ ] Archive or remove old scripts from your workflow

### For CI/CD Pipelines
- [ ] Update GitHub Actions workflows to use unified framework
- [ ] Test new workflow on actual hardware
- [ ] Update artifact collection paths
- [ ] Validate test result reporting
- [ ] Keep legacy scripts as fallback during transition

### For Development Teams
- [ ] Update documentation to reference unified framework
- [ ] Train team members on new commands
- [ ] Update development procedures
- [ ] Create team-specific adapter filters if needed
- [ ] Plan phase-out timeline for legacy scripts

---

## Troubleshooting Migration Issues

### Common Issues and Solutions

#### Issue: "Legacy script not found"
**Solution**: Legacy scripts moved to `tests/hardware/legacy/`
```powershell
# Update paths
./tests/hardware/legacy/phase1_interface_detection.ps1  # Old location
./tests/hardware/unified_test_framework.ps1 -Phase 1   # New approach
```

#### Issue: "Different test results"
**Solution**: Unified framework provides more comprehensive testing
- Compare JSON output structure
- Check adapter filtering options
- Enable verbose output for detailed comparison

#### Issue: "Missing I225 specific tests"
**Solution**: Use adapter filtering and legacy fallback
```powershell
# I225 specific testing
./tests/hardware/unified_test_framework.ps1 -AdapterFilter "I225" -Phase 2 -Verbose

# Legacy fallback if needed
./tests/hardware/legacy/test_i225_register_access.ps1
```

#### Issue: "Python integration broken"
**Solution**: Use hybrid approach
```powershell
# Hardware detection with unified framework
./tests/hardware/unified_test_framework.ps1 -Phase 1 -SaveResults

# Specialized analysis with Python (if available)
python tools/timestamp_analysis/timestamp_analyzer.py --duration 60
```

---

## Rollback Plan

If migration causes issues, you can temporarily rollback:

### 1. Use Legacy Scripts (Short-term)
```powershell
# Restore from legacy directory
./tests/hardware/legacy/phase1_interface_detection.ps1
./tests/hardware/legacy/test_i225_register_access.ps1
```

### 2. Hybrid Approach (Medium-term)
```powershell
# Use unified for basic testing, legacy for specialized
./tests/hardware/unified_test_framework.ps1 -Phase 1
./tests/hardware/legacy/test_i225_register_access.ps1 -Verbose
```

### 3. Full Migration (Target)
```powershell
# Use unified framework exclusively
./tests/hardware/unified_test_framework.ps1 -Phase all -Verbose -SaveResults
```

---

## 📁 Legacy Script Locations (Post-Migration)

All legacy scripts have been preserved and can be found in:

```
docs/tests/legacy/scripts/
├── test_i210_register_access.ps1
├── test_i219_register_access.ps1  
├── test_i225_register_access.ps1
├── build_and_test_phase2.ps1
├── build_and_test_phase3.ps1
├── configure_i225_network.ps1
├── simple_i225_config.ps1
└── [16 more legacy scripts...]
```

**To run a legacy script for reference:**
```powershell
# Example: Run legacy I225 test for comparison
.\docs\tests\legacy\scripts\test_i225_register_access.ps1 -Verbose
```

**Note**: Legacy scripts are preserved for reference only. Use the unified framework for all new testing.

---

## Support and Feedback

### Getting Help
- **Documentation**: `docs/tests/setup.md` (updated for unified framework)
- **Examples**: Run with `-Verbose` flag for detailed output
- **Issues**: Report problems with specific adapter/system combinations

### Providing Feedback
- Test unified framework on your hardware
- Report any missing functionality from legacy scripts
- Suggest improvements for adapter-specific testing
- Share results from different system configurations

---

## Timeline

### Current Status
- ✅ **Unified Framework**: Functional on I219-LM (DEV01)
- 🔄 **Legacy Archive**: Scripts moved to `tests/hardware/legacy/`
- 🔄 **GitHub Workflow**: Updated for unified approach
- 📋 **DAW02 Testing**: Validation needed on I225/I210 hardware

### Migration Timeline
- **Week 1**: Documentation updates and workflow migration
- **Week 2**: Hardware validation on all systems  
- **Week 3**: Legacy script deprecation notices
- **Month 2+**: Full legacy script removal (with archived copies)

**Recommendation**: Start migration immediately for new testing, validate thoroughly before removing legacy fallbacks.
