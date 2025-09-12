# OpenAvnu DAW02 Testing Package for Windows

**Created:** July 13, 2025  
**Ready for:** Windows DAW02 Testing

## Quick Start

### Option 1: Simple Start
1. Double-click `QUICK_START.bat`
2. Follow the prompts

### Option 2: PowerShell
1. Open PowerShell in this folder
2. Run: `.\run_daw02_tests.ps1`

### Option 3: Individual Tests
- Build test only: `.\test_01_build_verification.ps1`
- Intel HAL test only: `.\test_02_intel_hal_detection.ps1`

## Prerequisites

- **Windows 10/11**
- **Visual Studio 2022** or Visual Studio Build Tools
- **CMake 3.16+** (available in VS or standalone)
- **Npcap SDK** or WinPcap Developer Pack

## What the Tests Do

### Build Verification Test
- Tests that all 3 daemons compile successfully
- MRPD (MRP/STP Protocol), MAAP (MAC Address Allocation), Windows Shaper
- Measures build performance
- Verifies basic executable functionality

### Intel HAL Detection Test  
- Tests build without Intel HAL (software fallback)
- Tests build with Intel HAL (if available)
- Detects Intel network hardware
- Verifies hardware independence

## Expected Results

- **✅ Success:** All tests pass, ready for production
- **⚠️ Partial:** Some tests fail, review detailed logs  
- **❌ Failure:** Major issues, requires investigation

## Output

Results saved in `test_results_*` directory:
- Master report with summary
- Detailed logs for each test
- System information

## Transfer Results Back

After testing:
```powershell
Compress-Archive -Path test_results_* -DestinationPath daw02_results.zip
```

Then send `daw02_results.zip` back for analysis.

---
*OpenAvnu DAW02 Testing Package*
