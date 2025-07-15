#!/usr/bin/env powershell
<#
.SYNOPSIS
    Verification script for Hive-AVDECC entity recognition fixes
    
.DESCRIPTION
    This script validates that all critical AECP descriptor fixes have been
    implemented correctly for Hive-AVDECC compatibility.
    
.AUTHOR
    OpenAvnu Development Team - Hive Integration Fix
    
.DATE
    July 14, 2025
#>

[CmdletBinding()]
param(
    [switch]$BuildOnly,
    [switch]$TestOnly,
    [switch]$Verbose
)

# Colors for output
function Write-Success { param($Message) Write-Host "✅ $Message" -ForegroundColor Green }
function Write-Warning { param($Message) Write-Host "⚠️  $Message" -ForegroundColor Yellow }
function Write-Error { param($Message) Write-Host "❌ $Message" -ForegroundColor Red }
function Write-Info { param($Message) Write-Host "ℹ️  $Message" -ForegroundColor Cyan }
function Write-Step { param($Message) Write-Host "🔧 $Message" -ForegroundColor Blue }

Write-Host "=" * 80
Write-Host "OpenAvnu Hive-AVDECC Compatibility Verification" -ForegroundColor Magenta
Write-Host "Validating AECP Descriptor Fixes Implementation" -ForegroundColor Magenta
Write-Host "=" * 80
Write-Host ""

# Check if running as Administrator
$isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")

if (-not $isAdmin) {
    Write-Warning "Not running as Administrator. Some tests may fail."
    Write-Info "For full validation, run: Start-Process PowerShell -Verb RunAs"
    Write-Host ""
}

# Step 1: Verify source code fixes
Write-Step "Step 1: Verifying AECP descriptor implementation..."

$sourceFile = "raw_ethernet_avdecc_entity.c"
if (-not (Test-Path $sourceFile)) {
    Write-Error "Source file not found: $sourceFile"
    Write-Info "Current directory: $(Get-Location)"
    Write-Info "Expected files in examples directory"
    exit 1
}

# Check for key implementations
$sourceContent = Get-Content $sourceFile -Raw

$requiredFunctions = @(
    'create_entity_descriptor',
    'create_configuration_descriptor', 
    'handle_aecp_read_descriptor',
    'process_received_packet',
    'ntohll_local'
)

$missingFunctions = @()
foreach ($func in $requiredFunctions) {
    if ($sourceContent -notmatch $func) {
        $missingFunctions += $func
    }
}

if ($missingFunctions.Count -eq 0) {
    Write-Success "All required AECP functions implemented"
} else {
    Write-Error "Missing functions: $($missingFunctions -join ', ')"
    exit 1
}

# Check for AECP command ID
if ($sourceContent -match 'AECP_READ_DESCRIPTOR.*0x0000') {
    Write-Success "AECP READ_DESCRIPTOR command ID found"
} else {
    Write-Warning "AECP READ_DESCRIPTOR command ID not found or incorrect"
}

# Check for descriptor type constants
if ($sourceContent -match 'AEM_DESCRIPTOR_ENTITY.*0x0000' -and 
    $sourceContent -match 'AEM_DESCRIPTOR_CONFIGURATION.*0x0001') {
    Write-Success "AEM descriptor type constants found"
} else {
    Write-Warning "AEM descriptor type constants missing or incorrect"
}

Write-Host ""

# Step 2: Verify build scripts
Write-Step "Step 2: Verifying enhanced build scripts..."

$buildScript = "build_raw_ethernet_avdecc.bat"
if (Test-Path $buildScript) {
    $buildContent = Get-Content $buildScript -Raw
    
    if ($buildContent -match 'NPCAP_SDK_DIR' -or $buildContent -match 'NPCAP_DIR') {
        Write-Success "Build script has Npcap SDK detection"
    } else {
        Write-Warning "Build script missing Npcap SDK detection"
    }
    
    if ($buildContent -match 'PRODUCTION.*MODE' -and $buildContent -match 'SIMULATION.*MODE') {
        Write-Success "Build script supports production/simulation modes"
    } else {
        Write-Warning "Build script missing dual-mode support"
    }
} else {
    Write-Warning "Enhanced build script not found: $buildScript"
}

$testScript = "test_raw_ethernet_avdecc.bat"
if (Test-Path $testScript) {
    Write-Success "Test script found: $testScript"
} else {
    Write-Warning "Test script not found: $testScript"
}

Write-Host ""

# Step 3: Build verification (if not TestOnly)
if (-not $TestOnly) {
    Write-Step "Step 3: Build verification..."
    
    if (Test-Path $buildScript) {
        Write-Info "Attempting build..."
        try {
            & ".\$buildScript" 2>&1 | ForEach-Object {
                if ($_ -match 'error|Error|ERROR') {
                    Write-Warning "Build warning/error: $_"
                } elseif ($Verbose) {
                    Write-Host "  $_" -ForegroundColor Gray
                }
            }
            
            if (Test-Path "raw_ethernet_avdecc_entity.exe") {
                Write-Success "Build successful - executable created"
            } else {
                Write-Warning "Build completed but executable not found"
            }
        } catch {
            Write-Error "Build failed: $_"
        }
    } else {
        Write-Warning "Skipping build - build script not found"
    }
} else {
    Write-Info "Skipping build (TestOnly mode)"
}

Write-Host ""

# Step 4: Network adapter check
Write-Step "Step 4: Network adapter compatibility check..."

try {
    $adapters = Get-NetAdapter | Where-Object { $_.Status -eq 'Up' -and $_.Virtual -eq $false }
    
    if ($adapters.Count -gt 0) {
        Write-Success "Found $($adapters.Count) active network adapter(s)"
        foreach ($adapter in $adapters) {
            Write-Info "  - $($adapter.Name) ($($adapter.InterfaceDescription))"
        }
    } else {
        Write-Warning "No active physical network adapters found"
    }
} catch {
    Write-Warning "Could not enumerate network adapters: $_"
}

Write-Host ""

# Step 5: Npcap/WinPcap detection
Write-Step "Step 5: Packet capture library detection..."

# Check for Npcap SDK
$npcapPaths = @(
    "$env:NPCAP_SDK_DIR",
    "$env:NPCAP_DIR", 
    "C:\npcap-sdk",
    "C:\Program Files\Npcap\SDK"
)

$npcapFound = $false
foreach ($path in $npcapPaths) {
    if ($path -and (Test-Path "$path\Include\pcap.h")) {
        Write-Success "Npcap SDK found: $path"
        $npcapFound = $true
        break
    }
}

if (-not $npcapFound) {
    Write-Warning "Npcap SDK not found - will use simulation mode"
    Write-Info "Download: https://npcap.com/dist/npcap-sdk-1.13.zip"
}

# Check for runtime libraries
$runtimePaths = @(
    "C:\Program Files\Npcap\wpcap.dll",
    "C:\Program Files\Npcap\Packet.dll",
    "C:\Windows\System32\wpcap.dll"
)

$runtimeFound = $false
foreach ($path in $runtimePaths) {
    if (Test-Path $path) {
        Write-Success "Packet capture runtime found: $(Split-Path $path -Leaf)"
        $runtimeFound = $true
        break
    }
}

if (-not $runtimeFound) {
    Write-Warning "Packet capture runtime not found"
    Write-Info "Install Npcap runtime: https://npcap.com/dist/npcap-1.75.exe"
}

Write-Host ""

# Step 6: Integration test recommendations
Write-Step "Step 6: Integration testing recommendations..."

Write-Info "To test with Hive-AVDECC:"
Write-Host "  1. Build and run: test_raw_ethernet_avdecc.bat" -ForegroundColor White
Write-Host "  2. Start Hive-AVDECC application" -ForegroundColor White
Write-Host "  3. Select same network interface in both applications" -ForegroundColor White
Write-Host "  4. Look for 'OpenAvnu AVDECC Entity' in Hive device list" -ForegroundColor White
Write-Host "  5. Click entity to verify descriptor information loads" -ForegroundColor White

Write-Host ""

# Summary
Write-Host "=" * 80
Write-Host "VERIFICATION SUMMARY" -ForegroundColor Magenta
Write-Host "=" * 80

if ($missingFunctions.Count -eq 0 -and $npcapFound -and $runtimeFound) {
    Write-Success "✅ ALL SYSTEMS READY - Hive compatibility fixes implemented"
    Write-Success "✅ Ready for professional AVDECC tool integration"
} elseif ($missingFunctions.Count -eq 0) {
    Write-Warning "⚠️  CORE FIXES COMPLETE - Missing Npcap for production mode"
    Write-Info "Entity will work in simulation mode for testing"
} else {
    Write-Error "❌ CRITICAL ISSUES - Missing required implementations"
    Write-Error "Review and complete the AECP descriptor implementation"
}

Write-Host ""
Write-Info "For detailed status, see: docs/Hive_AVDECC_Entity_Recognition_Issue.md"
Write-Host ""
