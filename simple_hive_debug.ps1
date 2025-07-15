#!/usr/bin/env powershell
<#
.SYNOPSIS
    Simple Hive Discovery Debug Script
.DESCRIPTION
    Debug why Hive-AVDECC is not recognizing the OpenAvnu entity
#>

param(
    [string]$Interface = "3DC822E6-8C68-424C-9798-63CFBF52994B",
    [int]$TestDuration = 20
)

Write-Host "=== HIVE-AVDECC DISCOVERY DEBUG ===" -ForegroundColor Magenta
Write-Host ""

$entityPath = ".\build\examples\Release\raw_ethernet_avdecc_entity.exe"
$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"

# Check if entity exists
if (-not (Test-Path $entityPath)) {
    Write-Host "ERROR: Entity executable not found: $entityPath" -ForegroundColor Red
    exit 1
}
Write-Host "✓ Entity executable found" -ForegroundColor Green

# Check admin privileges
$isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")
if ($isAdmin) {
    Write-Host "✓ Running as Administrator" -ForegroundColor Green
} else {
    Write-Host "⚠ Not running as Administrator" -ForegroundColor Yellow
}

# Check network interface
Write-Host ""
Write-Host "Network Interface Analysis:" -ForegroundColor Cyan
$adapter = Get-NetAdapter | Where-Object { $_.InterfaceGuid -eq $Interface }
if ($adapter) {
    Write-Host "✓ Target Interface: $($adapter.Name)" -ForegroundColor Green
    Write-Host "  Description: $($adapter.InterfaceDescription)" -ForegroundColor Gray
    Write-Host "  Status: $($adapter.Status)" -ForegroundColor Gray
} else {
    Write-Host "ERROR: Interface not found: $Interface" -ForegroundColor Red
    Write-Host ""
    Write-Host "Available interfaces:" -ForegroundColor Yellow
    Get-NetAdapter | Where-Object { $_.Status -eq "Up" } | ForEach-Object {
        Write-Host "  $($_.Name): $($_.InterfaceGuid)" -ForegroundColor Gray
    }
    exit 1
}

# Test entity transmission
Write-Host ""
Write-Host "Testing AVDECC Entity Transmission:" -ForegroundColor Cyan
Write-Host "Duration: $TestDuration seconds" -ForegroundColor Gray
Write-Host "Interface: $($adapter.Name)" -ForegroundColor Gray

$outputFile = "entity_test_$timestamp.log"
$entityArgs = @("--interface", $Interface, "--duration", $TestDuration)

Write-Host ""
Write-Host "Starting entity..." -ForegroundColor Yellow

try {
    # Run entity and capture output
    $output = & $entityPath @entityArgs 2>&1
    
    # Save full output to file
    $output | Out-File -FilePath $outputFile -Encoding UTF8
    
    # Analyze output
    $txCount = ($output | Where-Object { $_ -match "Raw Ethernet TX.*bytes" }).Count
    $rxCount = ($output | Where-Object { $_ -match "Raw Ethernet RX.*bytes" }).Count
    $adpCount = ($output | Where-Object { $_ -match "ADP Entity Available sent" }).Count
    $initSuccess = $output | Where-Object { $_ -match "Raw Ethernet interface initialized successfully" }
    $errors = $output | Where-Object { $_ -match "ERROR|Failed|❌" }
    
    Write-Host ""
    Write-Host "Results Analysis:" -ForegroundColor Cyan
    Write-Host "=================" -ForegroundColor Cyan
    
    if ($initSuccess) {
        Write-Host "✓ Entity initialization: SUCCESS" -ForegroundColor Green
    } else {
        Write-Host "✗ Entity initialization: FAILED" -ForegroundColor Red
    }
    
    Write-Host "Transmitted packets: $txCount" -ForegroundColor White
    Write-Host "Received packets: $rxCount" -ForegroundColor White
    Write-Host "ADP advertisements: $adpCount" -ForegroundColor White
    
    if ($txCount -eq 0) {
        Write-Host ""
        Write-Host "🚨 CRITICAL ISSUE: NO PACKETS TRANSMITTED" -ForegroundColor Red
        Write-Host "This is why Hive cannot discover the entity!" -ForegroundColor Red
        Write-Host ""
        Write-Host "Possible causes:" -ForegroundColor Yellow
        Write-Host "1. Npcap not properly installed or configured" -ForegroundColor White
        Write-Host "2. Wrong network interface selected" -ForegroundColor White
        Write-Host "3. Insufficient permissions" -ForegroundColor White
        Write-Host "4. Entity build issue" -ForegroundColor White
    } else {
        Write-Host ""
        Write-Host "✓ Entity is transmitting packets" -ForegroundColor Green
        if ($rxCount -gt 0) {
            Write-Host "✓ Entity is receiving AVDECC traffic" -ForegroundColor Green
            Write-Host "  This indicates other AVDECC devices are present" -ForegroundColor Gray
        }
    }
    
    if ($errors.Count -gt 0) {
        Write-Host ""
        Write-Host "Errors detected:" -ForegroundColor Red
        foreach ($err in $errors) {
            Write-Host "  $err" -ForegroundColor Red
        }
    }
    
} catch {
    Write-Host "ERROR running entity: $_" -ForegroundColor Red
}

Write-Host ""
Write-Host "Hive Configuration Checklist:" -ForegroundColor Cyan
Write-Host "=============================" -ForegroundColor Cyan
Write-Host "1. Start Hive-AVDECC as Administrator" -ForegroundColor White
Write-Host "2. In Hive settings, select interface: $($adapter.Name)" -ForegroundColor White
Write-Host "3. Click 'Refresh' or restart Hive" -ForegroundColor White
Write-Host "4. Look for 'OpenAvnu AVDECC Entity' in device list" -ForegroundColor White

if ($txCount -gt 0) {
    Write-Host ""
    Write-Host "Next Steps:" -ForegroundColor Yellow
    Write-Host "1. Verify Hive is using the same network interface" -ForegroundColor White
    Write-Host "2. Try running Hive and entity on separate machines" -ForegroundColor White
    Write-Host "3. Use Wireshark to capture AVDECC packets" -ForegroundColor White
} else {
    Write-Host ""
    Write-Host "Fix transmission issues first:" -ForegroundColor Yellow
    Write-Host "1. Reinstall Npcap SDK" -ForegroundColor White
    Write-Host "2. Run as Administrator" -ForegroundColor White
    Write-Host "3. Check entity build configuration" -ForegroundColor White
}

Write-Host ""
Write-Host "Full entity output saved to: $outputFile" -ForegroundColor Gray
Write-Host "=== DEBUG COMPLETE ===" -ForegroundColor Magenta
