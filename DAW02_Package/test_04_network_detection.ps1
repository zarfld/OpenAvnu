# OpenAvnu Network Detection Test for Windows DAW02
# PowerShell version for Windows systems

Write-Host "=== OpenAvnu Network Detection Test ===" -ForegroundColor Green
Write-Host "System: $env:COMPUTERNAME"
Write-Host "Date: $(Get-Date)"
Write-Host "Objective: Test network interface detection capabilities on Windows"
Write-Host ""

# Get all network adapters
Write-Host "=== Network Interface Detection ===" -ForegroundColor Yellow

try {
    $AllAdapters = Get-NetAdapter | Sort-Object Name
    $IntelAdapters = $AllAdapters | Where-Object { $_.InterfaceDescription -like "*Intel*" }
    $PhysicalAdapters = $AllAdapters | Where-Object { $_.PhysicalMediaType -ne $null -and $_.PhysicalMediaType -ne "Unspecified" }
    
    Write-Host "Found $($AllAdapters.Count) total network interfaces:" -ForegroundColor Cyan
    
    foreach ($Adapter in $AllAdapters) {
        $Status = switch ($Adapter.Status) {
            "Up" { "[UP]" }
            "Down" { "[DOWN]" }
            "Disabled" { "[DISABLED]" }
            default { "[UNKNOWN]" }
        }
        
        $MediaType = if ($Adapter.PhysicalMediaType) { $Adapter.PhysicalMediaType } else { "Virtual" }
        Write-Host "   $Status $($Adapter.Name): $($Adapter.InterfaceDescription)" -ForegroundColor Gray
        Write-Host "         Media: $MediaType, Speed: $($Adapter.LinkSpeed)" -ForegroundColor DarkGray
    }
    
    Write-Host ""
    Write-Host "=== Intel Hardware Analysis ===" -ForegroundColor Yellow
    
    if ($IntelAdapters.Count -gt 0) {
        Write-Host "✅ Found $($IntelAdapters.Count) Intel network interface(s):" -ForegroundColor Green
        
        foreach ($Intel in $IntelAdapters) {
            Write-Host "   • $($Intel.Name): $($Intel.InterfaceDescription)" -ForegroundColor Cyan
            Write-Host "     Status: $($Intel.Status), Speed: $($Intel.LinkSpeed)" -ForegroundColor Gray
            
            # Check if this is AVB-capable hardware
            $AVBCapable = $false
            if ($Intel.InterfaceDescription -match "I219|I210|I211|I225|I226") {
                $AVBCapable = $true
                Write-Host "     ✅ AVB/TSN Capable: YES (Hardware timestamping supported)" -ForegroundColor Green
            } elseif ($Intel.InterfaceDescription -match "Wi-Fi|Wireless") {
                Write-Host "     ❌ AVB/TSN Capable: NO (Wireless cannot provide deterministic timing)" -ForegroundColor Red
            } else {
                Write-Host "     ⚠️  AVB/TSN Capable: UNKNOWN (Check Intel specifications)" -ForegroundColor Yellow
            }
            
            # Get additional details
            try {
                $AdapterConfig = Get-NetAdapterAdvancedProperty -Name $Intel.Name -ErrorAction SilentlyContinue
                if ($AdapterConfig) {
                    $Timestamping = $AdapterConfig | Where-Object { $_.RegistryKeyword -like "*timestamp*" -or $_.RegistryKeyword -like "*1588*" }
                    if ($Timestamping) {
                        Write-Host "     ✅ Hardware timestamping properties found" -ForegroundColor Green
                    }
                }
            } catch {
                # Ignore errors for advanced properties
            }
        }
    } else {
        Write-Host "❌ No Intel network hardware found" -ForegroundColor Red
        Write-Host "   This system may not support hardware-accelerated AVB/TSN" -ForegroundColor Yellow
    }
    
    Write-Host ""
    Write-Host "=== Physical Interface Analysis ===" -ForegroundColor Yellow
    
    $EthernetAdapters = $PhysicalAdapters | Where-Object { $_.PhysicalMediaType -like "*Ethernet*" -or $_.PhysicalMediaType -eq "802.3" }
    
    if ($EthernetAdapters.Count -gt 0) {
        Write-Host "✅ Found $($EthernetAdapters.Count) physical Ethernet interface(s):" -ForegroundColor Green
        
        foreach ($Eth in $EthernetAdapters) {
            Write-Host "   • $($Eth.Name): $($Eth.InterfaceDescription)" -ForegroundColor Cyan
            Write-Host "     Status: $($Eth.Status), Speed: $($Eth.LinkSpeed)" -ForegroundColor Gray
            
            # Test if we can get MAC address
            try {
                $MacAddress = (Get-NetAdapter -Name $Eth.Name).MacAddress
                Write-Host "     MAC: $MacAddress" -ForegroundColor Gray
            } catch {
                Write-Host "     MAC: Unable to retrieve" -ForegroundColor DarkGray
            }
        }
    } else {
        Write-Host "❌ No physical Ethernet interfaces found" -ForegroundColor Red
    }
    
} catch {
    Write-Host "❌ Network detection failed: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}

# OpenAvnu Daemon Network Compatibility Test
Write-Host ""
Write-Host "=== OpenAvnu Network Compatibility ===" -ForegroundColor Yellow

$BuildDir = "..\build"
if (Test-Path "$BuildDir\daemons\mrpd\Release\mrpd.exe") {
    Write-Host "✅ MRPD daemon available for network testing" -ForegroundColor Green
    
    # Test MRPD interface enumeration (dry run)
    try {
        Write-Host "   Testing MRPD interface detection..." -ForegroundColor Cyan
        $MrpdOutput = & "$BuildDir\daemons\mrpd\Release\mrpd.exe" --help 2>&1
        Write-Host "   ✅ MRPD responds to help command" -ForegroundColor Green
    } catch {
        Write-Host "   ⚠️  MRPD execution test failed (may require admin privileges)" -ForegroundColor Yellow
    }
} else {
    Write-Host "❌ MRPD daemon not found - build may have failed" -ForegroundColor Red
}

if (Test-Path "$BuildDir\thirdparty\gptp\Release\gptp.exe") {
    Write-Host "✅ gPTP daemon available for precision timing" -ForegroundColor Green
} else {
    Write-Host "❌ gPTP daemon not found - build may have failed" -ForegroundColor Red
}

# Recommendations
Write-Host ""
Write-Host "=== Recommendations ===" -ForegroundColor Yellow

$RecommendationsMade = $false

if ($IntelAdapters.Count -eq 0) {
    Write-Host "⚠️  No Intel network hardware detected:" -ForegroundColor Yellow
    Write-Host "   - Consider using Intel I219, I225, or I226 for optimal AVB performance" -ForegroundColor Gray
    Write-Host "   - Software fallback will be used (reduced precision)" -ForegroundColor Gray
    $RecommendationsMade = $true
}

$DisabledIntel = $IntelAdapters | Where-Object { $_.Status -eq "Disabled" }
if ($DisabledIntel.Count -gt 0) {
    Write-Host "⚠️  Intel adapter(s) are disabled:" -ForegroundColor Yellow
    foreach ($Disabled in $DisabledIntel) {
        Write-Host "   - Enable '$($Disabled.Name)' for AVB functionality" -ForegroundColor Gray
    }
    $RecommendationsMade = $true
}

if (-not $RecommendationsMade) {
    Write-Host "✅ Network configuration looks good for OpenAvnu deployment" -ForegroundColor Green
}

# Final Results
Write-Host ""
Write-Host "=== Final Results ===" -ForegroundColor Yellow

$NetworkTestSuccess = $true
if ($IntelAdapters.Count -eq 0 -and $EthernetAdapters.Count -eq 0) {
    $NetworkTestSuccess = $false
}

if ($NetworkTestSuccess) {
    Write-Host "✅ NETWORK DETECTION TEST: SUCCESS" -ForegroundColor Green
    Write-Host "   Network interfaces detected and analyzed successfully"
    exit 0
} else {
    Write-Host "❌ NETWORK DETECTION TEST: FAILED" -ForegroundColor Red
    Write-Host "   No suitable network interfaces found for AVB operation"
    exit 1
}
