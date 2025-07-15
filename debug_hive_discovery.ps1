#!/usr/bin/env powershell
<#
.SYNOPSIS
    Hive-AVDECC Discovery Debug Script
    
.DESCRIPTION
    Comprehensive diagnostics to identify why Hive-AVDECC is not recognizing
    the OpenAvnu AVDECC entity despite successful packet transmission.
    
.AUTHOR
    OpenAvnu Development Team - Hive Integration Success
    
.DATE
    July 14, 2025
#>

[CmdletBinding()]
param(
    [string]$Interface = "3DC822E6-8C68-424C-9798-63CFBF52994B",
    [int]$TestDuration = 30,
    [switch]$Verbose
)

# Colors for output
function Write-Success { param($Message) Write-Host "✅ $Message" -ForegroundColor Green }
function Write-Info { param($Message) Write-Host "ℹ️  $Message" -ForegroundColor Cyan }
function Write-Warning { param($Message) Write-Host "⚠️  $Message" -ForegroundColor Yellow }
function Write-Error { param($Message) Write-Host "❌ $Message" -ForegroundColor Red }
function Write-Debug { param($Message) if ($Verbose) { Write-Host "🔍 $Message" -ForegroundColor Gray } }

Write-Host "═══════════════════════════════════════════════════════════" -ForegroundColor Magenta
Write-Host "    HIVE-AVDECC DISCOVERY DIAGNOSTIC SCRIPT" -ForegroundColor Magenta
Write-Host "    Comprehensive Analysis for Entity Recognition Issues" -ForegroundColor Magenta
Write-Host "═══════════════════════════════════════════════════════════" -ForegroundColor Magenta
Write-Host ""

$entityPath = ".\build\examples\Release\raw_ethernet_avdecc_entity.exe"
$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$logFile = "hive_discovery_debug_$timestamp.log"

# Function to log and display
function Write-Log {
    param($Message, $Color = "White")
    $logMessage = "$(Get-Date -Format 'HH:mm:ss') - $Message"
    Write-Host $logMessage -ForegroundColor $Color
    Add-Content -Path $logFile -Value $logMessage
}

# Check prerequisites
Write-Info "Phase 1: Prerequisites and Environment Check"
Write-Host "═════════════════════════════════════════════════" -ForegroundColor Yellow

# Check if entity executable exists
if (-not (Test-Path $entityPath)) {
    Write-Error "AVDECC entity executable not found: $entityPath"
    Write-Info "Please build the project first with: cmake --build build --config Release"
    exit 1
}
Write-Success "OpenAvnu AVDECC entity executable found"

# Check admin privileges
$isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")
if (-not $isAdmin) {
    Write-Warning "Not running as Administrator - this may cause Npcap issues"
    Write-Info "For best results, run PowerShell as Administrator"
} else {
    Write-Success "Running with Administrator privileges"
}

# Check Npcap installation
$npcapService = Get-Service -Name "npcap" -ErrorAction SilentlyContinue
if ($npcapService) {
    if ($npcapService.Status -eq "Running") {
        Write-Success "Npcap service is running"
    } else {
        Write-Warning "Npcap service exists but is not running: $($npcapService.Status)"
    }
} else {
    Write-Warning "Npcap service not found - check Npcap installation"
}

Write-Host ""
Write-Info "Phase 2: Network Interface Analysis"
Write-Host "═════════════════════════════════════════════════" -ForegroundColor Yellow

# List network interfaces with detailed info
Write-Log "Analyzing network interfaces..." "Cyan"

try {
    $interfaces = Get-NetAdapter | Where-Object { $_.Status -eq "Up" }
    
    Write-Host "Active Network Interfaces:" -ForegroundColor White
    foreach ($adapter in $interfaces) {
        $guidMatch = $adapter.InterfaceGuid -eq $Interface
        $color = if ($guidMatch) { "Green" } else { "Gray" }
        $marker = if ($guidMatch) { "🎯 [TARGET]" } else { "  " }
        
        Write-Host "$marker Interface: $($adapter.Name)" -ForegroundColor $color
        Write-Host "    GUID: $($adapter.InterfaceGuid)" -ForegroundColor $color
        Write-Host "    Description: $($adapter.InterfaceDescription)" -ForegroundColor $color
        Write-Host "    Link Speed: $($adapter.LinkSpeed)" -ForegroundColor $color
        Write-Host "    MAC Address: $($adapter.MacAddress)" -ForegroundColor $color
        
        # Check if this is an Intel AVB-capable adapter
        if ($adapter.InterfaceDescription -match "Intel.*I(210|219|225)") {
            Write-Host "    ⭐ AVB-Capable Intel Adapter (Priority: High)" -ForegroundColor Green
        } elseif ($adapter.InterfaceDescription -match "RME") {
            Write-Host "    🎵 Professional Audio Interface (Priority: High)" -ForegroundColor Green
        } elseif ($adapter.InterfaceDescription -match "Wi-Fi|Wireless") {
            Write-Host "    📶 Wireless Interface (Not recommended for AVB)" -ForegroundColor Yellow
        }
        Write-Host ""
    }
} catch {
    Write-Error "Failed to analyze network interfaces: $_"
}

Write-Host ""
Write-Info "Phase 3: AVDECC Entity Test with Packet Analysis"
Write-Host "═════════════════════════════════════════════════" -ForegroundColor Yellow

Write-Log "Starting OpenAvnu AVDECC entity for $TestDuration seconds..." "Cyan"
Write-Log "Interface GUID: $Interface" "Cyan"

# Start the entity in the background to capture its output
$entityArgs = @("--interface", $Interface, "--duration", $TestDuration)
Write-Log "Command: $entityPath $($entityArgs -join ' ')" "Gray"

try {
    $entityProcess = Start-Process -FilePath $entityPath -ArgumentList $entityArgs -PassThru -NoNewWindow -RedirectStandardOutput "entity_output_$timestamp.log" -RedirectStandardError "entity_error_$timestamp.log"
    
    Write-Success "AVDECC entity started (PID: $($entityProcess.Id))"
    Write-Info "Monitoring for $TestDuration seconds..."
    
    # Monitor the process and log key events
    $startTime = Get-Date
    $packetCount = 0
    
    while (-not $entityProcess.HasExited -and ((Get-Date) - $startTime).TotalSeconds -lt ($TestDuration + 5)) {
        Start-Sleep -Milliseconds 500
        
        # Check if entity output file has new content
        if (Test-Path "entity_output_$timestamp.log") {
            $content = Get-Content "entity_output_$timestamp.log" -Tail 10 -ErrorAction SilentlyContinue
            if ($content) {
                foreach ($line in $content) {
                    if ($line -match "Raw Ethernet TX.*bytes") {
                        $packetCount++
                        Write-Log "📤 TX: $line" "Green"
                    } elseif ($line -match "Raw Ethernet RX.*bytes") {
                        Write-Log "📥 RX: $line" "Blue"
                    } elseif ($line -match "ADP Entity Available sent") {
                        Write-Log "📢 ADP Advertisement: $line" "Magenta"
                    } elseif ($line -match "ERROR|Failed|❌") {
                        Write-Log "⚠️  Error: $line" "Red"
                    }
                }
            }
        }
    }
    
    # Wait for entity to complete
    if (-not $entityProcess.HasExited) {
        Write-Log "Waiting for entity to complete..." "Yellow"
        $entityProcess.WaitForExit(10000)  # 10 second timeout
    }
    
    Write-Host ""
    Write-Info "Phase 4: Results Analysis"
    Write-Host "═════════════════════════════════════════════════" -ForegroundColor Yellow
    
    # Analyze entity output
    if (Test-Path "entity_output_$timestamp.log") {
        $outputContent = Get-Content "entity_output_$timestamp.log"
        
        # Check for successful initialization
        $initSuccess = $outputContent | Where-Object { $_ -match "Raw Ethernet interface initialized successfully" }
        if ($initSuccess) {
            Write-Success "Entity initialized successfully"
        } else {
            Write-Error "Entity failed to initialize properly"
        }
        
        # Count transmitted packets
        $txPackets = $outputContent | Where-Object { $_ -match "Raw Ethernet TX.*bytes" }
        $rxPackets = $outputContent | Where-Object { $_ -match "Raw Ethernet RX.*bytes" }
        $adpAdverts = $outputContent | Where-Object { $_ -match "ADP Entity Available sent" }
        
        Write-Log "📊 Packet Statistics:" "White"
        Write-Log "   Transmitted: $($txPackets.Count) packets" "Green"
        Write-Log "   Received: $($rxPackets.Count) packets" "Blue"
        Write-Log "   ADP Advertisements: $($adpAdverts.Count)" "Magenta"
        
        if ($txPackets.Count -eq 0) {
            Write-Error "NO PACKETS TRANSMITTED - This is the problem!"
            Write-Log "❌ Entity is not sending any packets on the network" "Red"
            Write-Log "   This explains why Hive cannot discover the entity" "Red"
        } else {
            Write-Success "Entity is transmitting packets correctly"
        }
        
        # Check for errors
        $errorLines = $outputContent | Where-Object { $_ -match "ERROR|Failed|❌" }
        if ($errorLines) {
            Write-Warning "Errors detected in entity output:"
            foreach ($errorLine in $errorLines) {
                Write-Log "   ⚠️  $errorLine" "Red"
            }
        }
    }
    
} catch {
    Write-Error "Failed to start AVDECC entity: $_"
}

Write-Host ""
Write-Info "Phase 5: Hive-AVDECC Configuration Recommendations"
Write-Host "═════════════════════════════════════════════════" -ForegroundColor Yellow

Write-Host "Hive-AVDECC Setup Checklist:" -ForegroundColor White
Write-Host "1. 🔧 Start Hive-AVDECC as Administrator" -ForegroundColor Cyan
Write-Host "2. 🌐 In Hive settings, select network interface:" -ForegroundColor Cyan

# Find the target interface name for Hive
$targetAdapter = Get-NetAdapter | Where-Object { $_.InterfaceGuid -eq $Interface }
if ($targetAdapter) {
    Write-Host "   📌 Interface: $($targetAdapter.Name)" -ForegroundColor Green
    Write-Host "   📝 Description: $($targetAdapter.InterfaceDescription)" -ForegroundColor Green
} else {
    Write-Warning "Could not find target interface details"
}

Write-Host "3. 🔄 Click 'Refresh' or restart Hive after selecting interface" -ForegroundColor Cyan
Write-Host "4. 👀 Look for 'OpenAvnu AVDECC Entity' in device list" -ForegroundColor Cyan
Write-Host "5. 🆔 Entity ID should start with: 0x001B21" -ForegroundColor Cyan

Write-Host ""
Write-Info "Phase 6: Troubleshooting Recommendations"
Write-Host "═════════════════════════════════════════════════" -ForegroundColor Yellow

if ($packetCount -eq 0) {
    Write-Host "🚨 PRIMARY ISSUE: No packet transmission detected" -ForegroundColor Red
    Write-Host ""
    Write-Host "Immediate Actions:" -ForegroundColor Yellow
    Write-Host "1. Verify Npcap SDK is properly installed" -ForegroundColor White
    Write-Host "2. Check that the entity has the correct interface GUID" -ForegroundColor White
    Write-Host "3. Ensure running as Administrator" -ForegroundColor White
    Write-Host "4. Try rebuilding with verbose CMake output" -ForegroundColor White
} else {
    Write-Host "✅ Entity is transmitting - focus on Hive configuration" -ForegroundColor Green
    Write-Host ""
    Write-Host "Hive Troubleshooting:" -ForegroundColor Yellow
    Write-Host "1. Verify Hive is using the same network interface" -ForegroundColor White
    Write-Host "2. Check Hive's network capture permissions" -ForegroundColor White
    Write-Host "3. Try running Hive and entity on separate machines" -ForegroundColor White
    Write-Host "4. Use Wireshark to verify AVDECC packets are visible" -ForegroundColor White
}

Write-Host ""
Write-Info "Next Steps for Debugging:"
Write-Host "• Review entity logs: entity_output_$timestamp.log" -ForegroundColor Cyan
Write-Host "• Check entity errors: entity_error_$timestamp.log" -ForegroundColor Cyan
Write-Host "• Full debug log: $logFile" -ForegroundColor Cyan

if ($rxPackets.Count -gt 0) {
    Write-Host ""
    Write-Success "🌟 POSITIVE SIGN: Entity is receiving AVDECC packets!"
    Write-Host "   This indicates there are other AVDECC devices on the network." -ForegroundColor Green
    Write-Host "   The issue is likely with Hive's interface selection or packet filtering." -ForegroundColor Yellow
}

Write-Host ""
Write-Host "═════════════════════════════════════════════════" -ForegroundColor Magenta
Write-Host "    DIAGNOSTIC COMPLETE" -ForegroundColor Magenta
Write-Host "═════════════════════════════════════════════════" -ForegroundColor Magenta
