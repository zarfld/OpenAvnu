# Directory Structure Fix Summary for OpenAvnu gPTP Tests

## CRITICAL: Running gPTP from Correct Directory

### The Problem
- gPTP configuration files (*.ini) are copied by CMake to the **Release build directory**
- Previous scripts ran gPTP from the **root project directory** where config files don't exist
- This caused gPTP to load default configuration instead of Milan specification

### The Solution
All gPTP tests must run from: `thirdparty\gptp\build_gptp\Release\`

### Updated Scripts

#### 1. test_milan_compliance_validation.ps1 ✅
- **FIXED**: Now uses `$gptpBuildPath = "build_gptp\Release"`
- **FIXED**: All Start-Job blocks use correct working directory
- **FIXED**: Command syntax: `gptp.exe MAC_ADDRESS -f test_milan_config.ini`

#### 2. test_milan_admin_privileges.ps1 ✅
- **FIXED**: Uses Push-Location/Pop-Location to change to Release directory
- **FIXED**: Runs gPTP with correct config file path
- **FIXED**: Includes error handling with directory restoration

#### 3. diagnose_pdelay_pcap.ps1 ✅
- **Status**: Already analyzes logs correctly (reads from any location)
- **No changes needed**: Diagnostic tool works from any directory

### Command Structure (CRITICAL)
```powershell
# CORRECT (from Release directory):
cd "thirdparty\gptp\build_gptp\Release"
.\gptp.exe 00:A0:C9:26:12:43 -f test_milan_config.ini

# WRONG (from root directory):
.\thirdparty\gptp\build_gptp\Release\gptp.exe -f .\thirdparty\gptp\test_milan_config.ini 00:A0:C9:26:12:43
```

### Why This Matters
1. **Milan Profile Loading**: Config file must be in working directory
2. **Log File Location**: Logs are created in working directory
3. **Relative Path Resolution**: gPTP resolves paths relative to working directory

### Verification Steps
1. Check config file exists: `Test-Path "thirdparty\gptp\build_gptp\Release\test_milan_config.ini"`
2. Verify Milan profile loads: Look for "MILAN PROFILE CREATED" in logs
3. Confirm MAC address parsing: Should not parse config filename as MAC
4. Validate PDelay exchanges: Should see "PDelay Request" and "PDelay Response" messages

### Build Requirements
- Run CMake configuration: Creates Release directory
- Run CMake build: Copies config files to Release directory
- All *.ini files are automatically copied by CMakeLists.txt:
  - gptp_cfg.ini (default)
  - test_milan_config.ini (Milan specification)
  - avnu_base_config.ini
  - avnu_presonus_config.ini
  - test_unified_profiles.ini

### Other Scripts That Need Updates
Any script that runs gPTP should be reviewed for:
- **Working directory**: Must run from Release build directory
- **Config file paths**: Use relative paths from Release directory
- **Log file handling**: Logs will be created in Release directory
- **MAC address order**: MAC first, then -f config_file.ini

### Testing Validation
Run `diagnose_pdelay_pcap.ps1` after any gPTP execution to verify:
- Fresh timestamps (not stale log data)
- Milan profile loaded correctly
- PDelay packet transmission/reception
- Hardware timestamping status