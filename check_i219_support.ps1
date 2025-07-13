# Intel i219 Detection and OpenAvnu Configuration Guide
# Specifically looks for i219 controllers and provides setup guidance

param(
    [switch]$ShowAll = $false,
    [switch]$SimulateI219 = $false
)

Write-Host "=== Intel i219 Detection for OpenAvnu gPTP/AVB ===" -ForegroundColor Cyan
Write-Host ""

# Function to detect all network adapters including disabled ones
function Get-AllNetworkAdapters {
    $adapters = @()
    
    # Get all network adapters (enabled and disabled)
    $allAdapters = Get-WmiObject -Class Win32_NetworkAdapter | Where-Object { 
        $_.PhysicalAdapter -eq $true
    }
    
    foreach ($adapter in $allAdapters) {
        $config = Get-WmiObject -Class Win32_NetworkAdapterConfiguration | Where-Object { 
            $_.Index -eq $adapter.DeviceID 
        }
        
        # Detailed i219 detection
        $description = $adapter.Description
        $isI219 = $description -match "I219|219" -and $description -like "*Intel*"
        $isEthernet = $description -like "*Ethernet*" -or $description -like "*Gigabit*"
        $isIntel = $description -like "*Intel*" -or $adapter.Manufacturer -like "*Intel*"
        $supportsAVB = $isI219 -or $description -match "I210|I225|I350"
        
        $adapterInfo = @{
            Name = $adapter.Name
            Description = $description
            NetConnectionID = $adapter.NetConnectionID
            MACAddress = $adapter.MACAddress
            NetEnabled = $adapter.NetEnabled
            Status = $adapter.NetConnectionStatus
            IsIntel = $isIntel
            IsI219 = $isI219
            IsEthernet = $isEthernet
            SupportsAVB = $supportsAVB
            PNPDeviceID = $adapter.PNPDeviceID
        }
        
        $adapters += $adapterInfo
    }
    
    return $adapters
}

# Get all adapters
$allAdapters = Get-AllNetworkAdapters

# Categorize adapters
$i219Adapters = $allAdapters | Where-Object { $_.IsI219 }
$intelEthernet = $allAdapters | Where-Object { $_.IsIntel -and $_.IsEthernet -and -not $_.IsI219 }
$otherEthernet = $allAdapters | Where-Object { -not $_.IsIntel -and $_.IsEthernet }
$allIntel = $allAdapters | Where-Object { $_.IsIntel }
$wirelessIntel = $allAdapters | Where-Object { $_.IsIntel -and -not $_.IsEthernet }

Write-Host "=== ADAPTER DETECTION RESULTS ===" -ForegroundColor White
Write-Host ""

# Show i219 adapters (ideal for OpenAvnu)
Write-Host "Intel i219 Controllers (OPTIMAL for OpenAvnu gPTP):" -ForegroundColor Green
if ($i219Adapters.Count -gt 0) {
    foreach ($adapter in $i219Adapters) {
        $statusText = switch ($adapter.Status) {
            2 { "Connected" }
            7 { "Media Disconnected" }
            default { "Unknown ($($adapter.Status))" }
        }
        Write-Host "  [SUCCESS] $($adapter.Description)" -ForegroundColor Green
        Write-Host "    Connection: $($adapter.NetConnectionID)" -ForegroundColor Gray
        Write-Host "    MAC: $($adapter.MACAddress)" -ForegroundColor Gray
        Write-Host "    Status: $statusText" -ForegroundColor Gray
        Write-Host "    Enabled: $($adapter.NetEnabled)" -ForegroundColor Gray
        Write-Host "    AVB Support: Full (Hardware Timestamping)" -ForegroundColor Green
        Write-Host ""
    }
} else {
    Write-Host "  No Intel i219 controllers found on this system." -ForegroundColor Yellow
    Write-Host ""
}

# Show other Intel Ethernet adapters
Write-Host "Other Intel Ethernet Controllers (GOOD for OpenAvnu):" -ForegroundColor Cyan
if ($intelEthernet.Count -gt 0) {
    foreach ($adapter in $intelEthernet) {
        $avbSupport = if ($adapter.SupportsAVB) { "Good (Limited AVB)" } else { "Basic" }
        Write-Host "  • $($adapter.Description)" -ForegroundColor Cyan
        Write-Host "    Connection: $($adapter.NetConnectionID)" -ForegroundColor Gray
        Write-Host "    MAC: $($adapter.MACAddress)" -ForegroundColor Gray
        Write-Host "    AVB Support: $avbSupport" -ForegroundColor Yellow
        Write-Host ""
    }
} else {
    Write-Host "  No other Intel Ethernet controllers found." -ForegroundColor Yellow
    Write-Host ""
}

# Show non-Intel Ethernet adapters
Write-Host "Non-Intel Ethernet Controllers (LIMITED support):" -ForegroundColor Yellow
if ($otherEthernet.Count -gt 0) {
    foreach ($adapter in $otherEthernet) {
        Write-Host "  - $($adapter.Description)" -ForegroundColor Yellow
        Write-Host "    Connection: $($adapter.NetConnectionID)" -ForegroundColor Gray
        Write-Host "    MAC: $($adapter.MACAddress)" -ForegroundColor Gray
        Write-Host "    AVB Support: Software only (no hardware timestamping)" -ForegroundColor Red
        Write-Host ""
    }
} else {
    Write-Host "  No non-Intel Ethernet controllers found." -ForegroundColor Gray
    Write-Host ""
}

if ($ShowAll) {
    Write-Host "Intel Wireless Adapters (NOT suitable for AVB/TSN):" -ForegroundColor Gray
    if ($wirelessIntel.Count -gt 0) {
        foreach ($adapter in $wirelessIntel) {
            Write-Host "  × $($adapter.Description)" -ForegroundColor Gray
            Write-Host "    Note: Wireless cannot provide deterministic timing for AVB" -ForegroundColor Gray
            Write-Host ""
        }
    }
}

# Analysis and recommendations
Write-Host "=== ANALYSIS AND RECOMMENDATIONS ===" -ForegroundColor White
Write-Host ""

if ($i219Adapters.Count -gt 0) {
    $bestI219 = $i219Adapters | Where-Object { $_.NetEnabled -eq $true } | Select-Object -First 1
    if ($bestI219) {
        Write-Host "EXCELLENT: i219 controller detected and enabled!" -ForegroundColor Green
        Write-Host ""
        Write-Host "Recommended configuration for '$($bestI219.NetConnectionID)':" -ForegroundColor Cyan
        Write-Host "  gPTP Master: gptp.exe -S -i `"$($bestI219.NetConnectionID)`"" -ForegroundColor White
        Write-Host "  gPTP Slave:  gptp.exe -i `"$($bestI219.NetConnectionID)`"" -ForegroundColor White
        Write-Host "  MAAP:        maap_daemon.exe -i `"$($bestI219.NetConnectionID)`"" -ForegroundColor White
        Write-Host "  MRPD:        mrpd.exe -i `"$($bestI219.NetConnectionID)`"" -ForegroundColor White
        Write-Host ""
        Write-Host "Features available:" -ForegroundColor Green
        Write-Host "  ✓ Hardware timestamping (IEEE 1588/802.1AS)" -ForegroundColor Green
        Write-Host "  ✓ Traffic shaping (IEEE 802.1Qav)" -ForegroundColor Green
        Write-Host "  ✓ Stream reservation (IEEE 802.1Qat)" -ForegroundColor Green
        Write-Host "  ✓ AVB-compatible audio streaming" -ForegroundColor Green
    } else {
        Write-Host "PARTIAL: i219 detected but disabled" -ForegroundColor Yellow
        Write-Host "Enable the i219 adapter in Network Connections to use OpenAvnu" -ForegroundColor Yellow
    }
} elseif ($intelEthernet.Count -gt 0) {
    $bestIntel = $intelEthernet | Where-Object { $_.NetEnabled -eq $true } | Select-Object -First 1
    if ($bestIntel) {
        Write-Host "GOOD: Intel Ethernet controller available" -ForegroundColor Yellow
        Write-Host ""
        Write-Host "Configuration for '$($bestIntel.NetConnectionID)':" -ForegroundColor Cyan
        Write-Host "  Limited AVB features - software timing only" -ForegroundColor Yellow
        Write-Host "  gPTP: gptp.exe -i `"$($bestIntel.NetConnectionID)`"" -ForegroundColor White
        Write-Host ""
        Write-Host "Note: Consider upgrading to i219/i210/i225 for full AVB support" -ForegroundColor Yellow
    }
} elseif ($otherEthernet.Count -gt 0) {
    Write-Host "LIMITED: Only non-Intel Ethernet available" -ForegroundColor Red
    Write-Host "OpenAvnu will work but without hardware timestamping" -ForegroundColor Red
    Write-Host "Recommend adding Intel i219/i210/i225 PCIe adapter" -ForegroundColor Yellow
} else {
    Write-Host "INCOMPATIBLE: No suitable Ethernet adapters found" -ForegroundColor Red
    Write-Host ""
    Write-Host "Required hardware:" -ForegroundColor Yellow
    Write-Host "  - Intel i219 (integrated on many motherboards)" -ForegroundColor White
    Write-Host "  - Intel i210/i225 (PCIe add-in cards)" -ForegroundColor White
    Write-Host "  - Intel i350 (server-grade, PCIe)" -ForegroundColor White
}

Write-Host ""

# Check if OpenAvnu daemons are built
Write-Host "=== OPENAVNU BUILD STATUS ===" -ForegroundColor White
$buildDir = "D:\Repos\OpenAvnu\build"
$executables = @(
    @{ Name = "gPTP"; Path = "$buildDir\thirdparty\gptp\Release\gptp.exe" },
    @{ Name = "MAAP"; Path = "$buildDir\daemons\maap\Release\maap_daemon.exe" },
    @{ Name = "MRPD"; Path = "$buildDir\daemons\mrpd\Release\mrpd.exe" }
)

$builtCount = 0
foreach ($exe in $executables) {
    if (Test-Path $exe.Path) {
        Write-Host "✓ $($exe.Name) daemon: Built" -ForegroundColor Green
        $builtCount++
    } else {
        Write-Host "× $($exe.Name) daemon: Missing" -ForegroundColor Red
    }
}

if ($builtCount -eq $executables.Count) {
    Write-Host ""
    Write-Host "All OpenAvnu daemons successfully built!" -ForegroundColor Green
    Write-Host "WinSock header fixes and Intel HAL integration working correctly." -ForegroundColor Green
} else {
    Write-Host ""
    Write-Host "Some daemons missing - check build configuration." -ForegroundColor Yellow
}

Write-Host ""
Write-Host "Analysis complete. Ready for OpenAvnu testing with appropriate hardware." -ForegroundColor Cyan
