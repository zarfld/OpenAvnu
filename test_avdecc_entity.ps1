#!/usr/bin/env powershell
<#
.SYNOPSIS
    OpenAvnu AVDECC Entity Test Script for Hive Integration
    
.DESCRIPTION
    This script makes it easy to test the OpenAvnu AVDECC entity with proper
    interface selection for Hive-AVDECC compatibility.
    
.AUTHOR
    OpenAvnu Development Team - Hive Integration Success
    
.DATE
    July 14, 2025
#>

[CmdletBinding()]
param(
    [int]$Duration = 60,
    [switch]$ListInterfaces,
    [string]$Interface,
    [switch]$Help
)

# Colors for output
function Write-Success { param($Message) Write-Host "✅ $Message" -ForegroundColor Green }
function Write-Info { param($Message) Write-Host "ℹ️  $Message" -ForegroundColor Cyan }
function Write-Warning { param($Message) Write-Host "⚠️  $Message" -ForegroundColor Yellow }
function Write-Error { param($Message) Write-Host "❌ $Message" -ForegroundColor Red }

$exePath = ".\build\examples\Release\raw_ethernet_avdecc_entity.exe"

if ($Help) {
    Write-Host "OpenAvnu AVDECC Entity Test Script" -ForegroundColor Magenta
    Write-Host "===================================" -ForegroundColor Magenta
    Write-Host ""
    Write-Host "This script helps test the OpenAvnu AVDECC entity for Hive-AVDECC compatibility."
    Write-Host ""
    Write-Host "Parameters:"
    Write-Host "  -Duration <seconds>     Run entity for specified duration (default: 60)"
    Write-Host "  -ListInterfaces         List all available network interfaces"
    Write-Host "  -Interface <guid>       Use specific interface GUID"
    Write-Host "  -Help                   Show this help message"
    Write-Host ""
    Write-Host "Examples:"
    Write-Host "  .\test_avdecc_entity.ps1 -ListInterfaces"
    Write-Host "  .\test_avdecc_entity.ps1 -Duration 120"
    Write-Host "  .\test_avdecc_entity.ps1 -Interface '3DC822E6-8C68-424C-9798-63CFBF52994B'"
    Write-Host ""
    Write-Host "Quick Start for Hive Testing:"
    Write-Host "1. Run: .\test_avdecc_entity.ps1 -ListInterfaces"
    Write-Host "2. Find Intel I219/I210/I225 or RME interface GUID"
    Write-Host "3. Run: .\test_avdecc_entity.ps1 -Interface '<GUID>'"
    Write-Host "4. Start Hive-AVDECC and select same interface"
    Write-Host "5. Look for 'OpenAvnu AVDECC Entity' in device list"
    return
}

# Check if executable exists
if (-not (Test-Path $exePath)) {
    Write-Error "AVDECC entity executable not found: $exePath"
    Write-Info "Build the project first with: cmake --build build --config Release"
    return
}

# Check if running as Administrator
$isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")
if (-not $isAdmin) {
    Write-Warning "Not running as Administrator"
    Write-Info "For best results, run PowerShell as Administrator"
    Write-Host ""
}

if ($ListInterfaces) {
    Write-Info "Listing available network interfaces..."
    & $exePath --list
    Write-Host ""
    Write-Success "For Hive-AVDECC compatibility, choose:"
    Write-Host "  🎯 Intel I210/I219/I225 interfaces (best AVB support)" -ForegroundColor Yellow
    Write-Host "  🎵 RME audio interfaces (professional AVB support)" -ForegroundColor Yellow
    Write-Host "  🔗 Physical Ethernet interfaces (basic support)" -ForegroundColor Yellow
    Write-Host ""
    Write-Info "Copy the GUID from your chosen interface and use:"
    Write-Host "  .\test_avdecc_entity.ps1 -Interface '<GUID>'" -ForegroundColor Cyan
    return
}

# Build command arguments
$entityArgs = @("--duration", $Duration)

if ($Interface) {
    $entityArgs += @("--interface", $Interface)
    Write-Info "Using specified interface: $Interface"
} else {
    Write-Info "Using automatic interface selection"
}

Write-Host "=" * 60
Write-Host "OpenAvnu AVDECC Entity Test" -ForegroundColor Magenta
Write-Host "Duration: $Duration seconds" -ForegroundColor Magenta
Write-Host "=" * 60
Write-Host ""

Write-Success "Starting OpenAvnu AVDECC Entity..."
Write-Info "Entity will run for $Duration seconds"
Write-Info "Press Ctrl+C to stop early"
Write-Host ""

try {
    # Start the AVDECC entity
    & $exePath @entityArgs
    
    Write-Host ""
    Write-Success "AVDECC entity completed successfully!"
    
} catch {
    Write-Error "Error running AVDECC entity: $_"
    return
}

Write-Host ""
Write-Host "Hive-AVDECC Testing Checklist:" -ForegroundColor Yellow
Write-Host "==============================" -ForegroundColor Yellow
Write-Host "1. ✅ AVDECC entity ran successfully"
if ($Interface) {
    Write-Host "2. 🔧 Start Hive-AVDECC and select interface with GUID: $Interface"
} else {
    Write-Host "2. 🔧 Start Hive-AVDECC and select the auto-detected interface"
}
Write-Host "3. 👀 Look for 'OpenAvnu AVDECC Entity' in Hive device list"
Write-Host "4. 📖 Click entity to verify descriptor information loads"
Write-Host "5. ✅ Confirm no enumeration timeouts or errors"
Write-Host ""
Write-Info "If entity doesn't appear in Hive:"
Write-Host "  - Ensure both use the same network interface"
Write-Host "  - Check that interface has active network connection"
Write-Host "  - Verify Hive is running as Administrator"
Write-Host "  - Try running on separate machines to avoid loopback issues"
