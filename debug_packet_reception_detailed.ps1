#!/usr/bin/env pwsh

<#
.SYNOPSIS
    Detailed gPTP packet reception debugging
.DESCRIPTION
    Tests exactly what happens when gPTP tries to receive network packets
#>

param(
    [switch]$AdminMode = $false,
    [switch]$VerboseLogging = $false
)

Write-Host "🔍 Detailed gPTP Packet Reception Analysis" -ForegroundColor Magenta
Write-Host "==========================================" -ForegroundColor Magenta

# Check if running as admin
$isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")
if (-not $isAdmin -and $AdminMode) {
    Write-Host "❌ Not running as Administrator! Some tests may fail." -ForegroundColor Red
    Write-Host "   Restart PowerShell as Administrator for full diagnostics." -ForegroundColor Yellow
}

# Get network adapter info
Write-Host "`n1. Network Adapter Analysis:" -ForegroundColor Cyan
$adapters = Get-NetAdapter | Where-Object { $_.Status -eq "Up" -and $_.Virtual -eq $false }
$intelAdapter = $adapters | Where-Object { $_.InterfaceDescription -like "*Intel*" }

if ($intelAdapter) {
    Write-Host "   ✅ Intel adapter found: $($intelAdapter.InterfaceDescription)" -ForegroundColor Green
    Write-Host "   📍 MAC: $($intelAdapter.MacAddress)" -ForegroundColor White
    Write-Host "   🔌 Link Speed: $($intelAdapter.LinkSpeed)" -ForegroundColor White
    
    # Get detailed adapter properties
    $adapterAdvanced = Get-NetAdapterAdvancedProperty -Name $intelAdapter.Name -ErrorAction SilentlyContinue
    $ptpProps = $adapterAdvanced | Where-Object { $_.DisplayName -like "*PTP*" -or $_.DisplayName -like "*1588*" -or $_.RegistryKeyword -like "*PTP*" }
    
    if ($ptpProps) {
        Write-Host "   🕐 PTP/1588 Properties:" -ForegroundColor Yellow
        foreach ($prop in $ptpProps) {
            Write-Host "      $($prop.DisplayName): $($prop.DisplayValue)" -ForegroundColor Gray
        }
    } else {
        Write-Host "   ⚠️  No PTP/1588 properties found" -ForegroundColor Yellow
    }
} else {
    Write-Host "   ❌ No Intel adapter found!" -ForegroundColor Red
    return
}

# Test PCAP functionality
Write-Host "`n2. PCAP Library Test:" -ForegroundColor Cyan

# Check if Npcap/WinPcap is installed
$npcapService = Get-Service -Name "npcap" -ErrorAction SilentlyContinue
$winpcapService = Get-Service -Name "NPF" -ErrorAction SilentlyContinue

if ($npcapService) {
    Write-Host "   ✅ Npcap service found: $($npcapService.Status)" -ForegroundColor Green
} elseif ($winpcapService) {
    Write-Host "   ✅ WinPcap service found: $($winpcapService.Status)" -ForegroundColor Green
} else {
    Write-Host "   ❌ No PCAP service found! Install Npcap or WinPcap." -ForegroundColor Red
    return
}

# Test if we can create a simple packet capture handle
Write-Host "`n3. Interface Opening Test:" -ForegroundColor Cyan

$macFormatted = $intelAdapter.MacAddress.Replace("-", ":")
Write-Host "   📝 Testing with MAC: $macFormatted" -ForegroundColor White

# Change to the correct gPTP directory
Push-Location "thirdparty\gptp\build_gptp\Release"

try {
    # Check if gPTP executable exists
    if (-not (Test-Path "gptp2.exe")) {
        Write-Host "   ❌ gPTP executable not found! Build first." -ForegroundColor Red
        return
    }
    
    Write-Host "   ✅ Found gPTP executable" -ForegroundColor Green
    
    # Test packet reception with timeout
    Write-Host "`n4. Packet Reception Test (5 seconds):" -ForegroundColor Cyan
    
    $gptpArgs = @(
        $macFormatted
        "-f", "test_milan_config.ini"
    )
    
    if ($VerboseLogging) {
        $gptpArgs += "-debug-packets"
    }
    
    Write-Host "   🚀 Starting gPTP with args: $($gptpArgs -join ' ')" -ForegroundColor White
    Write-Host "   ⏱️  Will timeout after 5 seconds to check reception..." -ForegroundColor Yellow
    
    # Start gPTP process
    $process = Start-Process -FilePath ".\gptp2.exe" -ArgumentList $gptpArgs -NoNewWindow -PassThru -RedirectStandardOutput "packet_test.log" -RedirectStandardError "packet_test_err.log"
    
    # Wait 5 seconds
    Start-Sleep -Seconds 5
    
    # Stop the process
    if (-not $process.HasExited) {
        $process.Kill()
        Write-Host "   ⏹️  Stopped gPTP after 5 seconds" -ForegroundColor Yellow
    }
    
    # Analyze the output
    Write-Host "`n5. Packet Reception Analysis:" -ForegroundColor Cyan
    
    if (Test-Path "packet_test.log") {
        $output = Get-Content "packet_test.log" -Raw
        Write-Host "   📄 gPTP output (first 500 chars):" -ForegroundColor Gray
        Write-Host ($output.Substring(0, [Math]::Min(500, $output.Length))) -ForegroundColor Gray
        
        # Look for specific packet reception indicators
        $pcapOpenSuccess = $output -match "Successfully opened interface"
        $packetsReceived = $output -match "PTP PACKET.*DETECTED"
        $interfaceErrors = $output -match "Failed to.*interface|pcap_open failed"
        $timeouts = $output -match "Timeout occurred|consecutive timeouts"
        
        Write-Host "`n   📊 Analysis Results:" -ForegroundColor Yellow
        Write-Host "      Interface opened: $(if($pcapOpenSuccess){'✅ YES'}else{'❌ NO'})" -ForegroundColor $(if($pcapOpenSuccess){'Green'}else{'Red'})
        Write-Host "      Packets received: $(if($packetsReceived){'✅ YES'}else{'❌ NO'})" -ForegroundColor $(if($packetsReceived){'Green'}else{'Red'})
        Write-Host "      Interface errors: $(if($interfaceErrors){'❌ YES'}else{'✅ NO'})" -ForegroundColor $(if($interfaceErrors){'Red'}else{'Green'})
        Write-Host "      Timeouts occurred: $(if($timeouts){'⚠️  YES'}else{'✅ NO'})" -ForegroundColor $(if($timeouts){'Yellow'}else{'Green'})
        
        # Count specific events
        $timeoutMatches = [regex]::Matches($output, "consecutive timeouts")
        $packetMatches = [regex]::Matches($output, "PTP PACKET")
        
        Write-Host "`n   📈 Event Counts:" -ForegroundColor Yellow
        Write-Host "      Timeout events: $($timeoutMatches.Count)" -ForegroundColor Gray
        Write-Host "      PTP packets: $($packetMatches.Count)" -ForegroundColor Gray
        
    } else {
        Write-Host "   ❌ No output log generated!" -ForegroundColor Red
    }
    
    if (Test-Path "packet_test_err.log") {
        $errors = Get-Content "packet_test_err.log" -Raw
        if ($errors.Trim()) {
            Write-Host "`n   ⚠️  Error output:" -ForegroundColor Red
            Write-Host $errors -ForegroundColor Red
        }
    }
    
    # Check for network traffic during test
    Write-Host "`n6. Network Traffic Analysis:" -ForegroundColor Cyan
    
    # Get network statistics
    $beforeStats = Get-NetAdapterStatistics -Name $intelAdapter.Name -ErrorAction SilentlyContinue
    if ($beforeStats) {
        Write-Host "   📊 Adapter Statistics:" -ForegroundColor White
        Write-Host "      Received Packets: $($beforeStats.ReceivedPackets)" -ForegroundColor Gray
        Write-Host "      Received Bytes: $($beforeStats.ReceivedBytes)" -ForegroundColor Gray
        Write-Host "      Sent Packets: $($beforeStats.SentPackets)" -ForegroundColor Gray
        Write-Host "      Sent Bytes: $($beforeStats.SentBytes)" -ForegroundColor Gray
    }
    
} catch {
    Write-Host "   ❌ Error during testing: $($_.Exception.Message)" -ForegroundColor Red
} finally {
    Pop-Location
}

Write-Host "`n7. Diagnosis Summary:" -ForegroundColor Magenta

# Provide diagnosis based on findings
$logPath = "thirdparty\gptp\build_gptp\Release\packet_test.log"
if (Test-Path $logPath) {
    $fullOutput = Get-Content $logPath -Raw
    
    if ($fullOutput -match "Successfully opened interface") {
        if ($fullOutput -match "PTP PACKET.*DETECTED") {
            Write-Host "   ✅ WORKING: gPTP can receive packets!" -ForegroundColor Green
        } elseif ($fullOutput -match "consecutive timeouts.*100") {
            Write-Host "   ⚠️  WAITING: Interface working but no PTP traffic detected" -ForegroundColor Yellow
            Write-Host "      This is normal if no other gPTP devices are transmitting" -ForegroundColor Gray
        } else {
            Write-Host "   ❓ UNCLEAR: Interface opened but uncertain about packet flow" -ForegroundColor Yellow
        }
    } else {
        Write-Host "   ❌ BROKEN: gPTP cannot open network interface!" -ForegroundColor Red
        Write-Host "      Check PCAP installation and permissions" -ForegroundColor Red
    }
} else {
    Write-Host "   ❌ BROKEN: gPTP failed to generate any output!" -ForegroundColor Red
}

Write-Host "`n🔍 Diagnosis complete. Check logs for details." -ForegroundColor Magenta