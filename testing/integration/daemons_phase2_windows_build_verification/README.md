# OpenAvnu DAW02 Windows Build Verification & Intel HAL Integration Test

**Test Suite Location:** `testing/integration/daemons_phase2_windows_build_verification/`

## Purpose
- Verifies build and basic functionality of MRPD, MAAP, and Windows Shaper daemons on Windows
- Tests Intel HAL detection and software fallback
- Ensures repository compliance and cross-platform readiness

## Test Scripts
- `run_daw02_tests.ps1` — Master test runner
- `test_01_build_verification.ps1` — Build and basic functionality test for all daemons
- `test_02_intel_hal_detection.ps1` — Intel hardware/HAL detection and fallback test

## Usage
1. Open a PowerShell terminal in the repo root.
2. Run the master test script:
   ```powershell
   .\testing\integration\daemons_phase2_windows_build_verification\run_daw02_tests.ps1
   ```
3. Review the generated reports in the `test_results_*` directory.

## Test Coverage
- CMake configuration and build for all daemons
- Executable presence and help/version output
- Intel network adapter and HAL library detection
- Software fallback verification
- Network interface analysis

## Results
- Master report: `OpenAvnu_DAW02_Master_Report.md`
- System info: `system_info.txt`
- Individual logs: `*.log`

## Repository Compliance
- Test scripts and documentation are stored under `testing/integration/` as required by project rules.
- No build artifacts or ZIPs are versioned.

---

**Maintainer:** GitHub Copilot
**Date:** 2025-07-13
