# Quick Python Setup for I219-LM Analysis Tools

## Problem
The current system has a Windows Store Python stub that prevents proper tool execution.

## Solution Options

### Option 1: Install Python from python.org (Recommended)
1. Go to https://www.python.org/downloads/
2. Download Python 3.11 or later
3. **IMPORTANT**: During installation, check "Add Python to PATH"
4. Restart PowerShell
5. Verify: `python --version`
6. Install dependencies: `pip install -r requirements.txt`

### Option 2: Use Anaconda
1. Download Anaconda from https://www.anaconda.com/
2. Install with default settings
3. Open "Anaconda Prompt"
4. Create environment: `conda create -n gptp python=3.11`
5. Activate: `conda activate gptp`
6. Install: `pip install -r requirements.txt`

### Option 3: Disable Windows Store Python (Advanced)
1. Open Settings > Apps > Advanced app settings > App execution aliases
2. Turn OFF "python.exe" and "python3.exe"
3. Install Python from python.org
4. Restart system

## Quick Verification
After Python installation:
```powershell
cd "d:\Repos\OpenAvnu\tools\timestamp_analysis"
python validate_setup.py
```

## Immediate Testing Without Full Python
Use the PowerShell validation script for immediate I219-LM testing:
```powershell
.\validate_windows.ps1
```

This confirms hardware functionality while Python environment is being set up.
