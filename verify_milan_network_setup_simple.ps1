# Milan Network Setup Verification Script
# Helps users prepare their network for Milan compliance testing

Write-Host "=== MILAN NETWORK SETUP VERIFICATION ===" -ForegroundColor Green
Write-Host "This script helps prepare your network for Milan compliance testing with real devices" -ForegroundColor Cyan
Write-Host ""

# Check Intel adapter status
Write-Host "[1] INTEL ADAPTER STATUS" -ForegroundColor Yellow
$intelAdapters = Get-NetAdapter | Where-Object { 
    $_.InterfaceDescription -like '*I210*' -or 
    $_.InterfaceDescription -like '*I219*' -or 
    $_.InterfaceDescription -like '*I225*' -or 
    $_.InterfaceDescription -like '*I226*' 
}

if ($intelAdapters) {
    foreach ($adapter in $intelAdapters) {
        Write-Host "  Found: $($adapter.Name) - $($adapter.InterfaceDescription)" -ForegroundColor Green
        Write-Host "    Status: $($adapter.Status)" -ForegroundColor $(if ($adapter.Status -eq "Up") { "Green" } else { "Red" })
        Write-Host "    Link Speed: $($adapter.LinkSpeed)" -ForegroundColor White
        Write-Host "    MAC: $($adapter.MacAddress)" -ForegroundColor Gray
        
        if ($adapter.Status -eq "Up") {
            # Get IP configuration
            $ipConfig = Get-NetIPAddress -InterfaceIndex $adapter.InterfaceIndex -AddressFamily IPv4 -ErrorAction SilentlyContinue
            if ($ipConfig) {
                Write-Host "    IP Address: $($ipConfig.IPAddress)" -ForegroundColor Green
                if ($ipConfig.IPAddress -like "169.254.*") {
                    Write-Host "    WARNING: Using APIPA address - consider static IP for Milan network" -ForegroundColor Yellow
                }
            } else {
                Write-Host "    ERROR: No IP address configured" -ForegroundColor Red
            }
        }
        Write-Host ""
    }
} else {
    Write-Host "  ERROR: No Intel AVB-capable adapters found" -ForegroundColor Red
    Write-Host "     Required: Intel I210, I219, I225, or I226 adapter" -ForegroundColor Gray
    Write-Host ""
}

# Check network connectivity
Write-Host "[2] NETWORK CONNECTIVITY" -ForegroundColor Yellow
if ($intelAdapters | Where-Object { $_.Status -eq "Up" }) {
    $activeAdapter = $intelAdapters | Where-Object { $_.Status -eq "Up" } | Select-Object -First 1
    Write-Host "  Testing connectivity on $($activeAdapter.Name)..." -ForegroundColor Cyan
    
    try {
        # Try to ping common network addresses
        $pingResults = @()
        $testAddresses = @("192.168.1.1", "192.168.0.1", "10.0.0.1", "172.16.0.1")
        
        foreach ($addr in $testAddresses) {
            $ping = Test-Connection -ComputerName $addr -Count 1 -Quiet -ErrorAction SilentlyContinue
            if ($ping) {
                $pingResults += $addr
                Write-Host "  Gateway reachable: $addr" -ForegroundColor Green
            }
        }
        
        if ($pingResults.Count -eq 0) {
            Write-Host "  WARNING: No common gateways reachable - check network connection" -ForegroundColor Yellow
        }
    } catch {
        Write-Host "  WARNING: Network connectivity test failed" -ForegroundColor Yellow
    }
} else {
    Write-Host "  ERROR: No active network adapters" -ForegroundColor Red
}
Write-Host ""

# Check for potential Milan devices
Write-Host "[3] MILAN DEVICE DISCOVERY" -ForegroundColor Yellow
Write-Host "  Scanning for potential Milan-compatible devices..." -ForegroundColor Cyan

try {
    # Get ARP table
    $arpEntries = arp -a | Where-Object { $_ -match "\d+\.\d+\.\d+\.\d+" }
    $deviceCount = ($arpEntries | Measure-Object).Count
    
    Write-Host "  Found $deviceCount devices on network" -ForegroundColor White
    
    # Look for known AVB vendor MAC prefixes
    $avbVendors = @(
        @{ Pattern = "00-1c-f0"; Vendor = "PreSonus"; Devices = @("StudioLive AI", "NSB Stage Box", "AVB Switch") },
        @{ Pattern = "00-07-7d"; Vendor = "MOTU"; Devices = @("AVB Audio Interface", "1248", "8M") },
        @{ Pattern = "00-50-c2"; Vendor = "L-Acoustics"; Devices = @("LA12X", "LA4X", "Milan Amplifier") },
        @{ Pattern = "00-80-92"; Vendor = "Focusrite"; Devices = @("RedNet", "AVB Interface") },
        @{ Pattern = "00-0c-cc"; Vendor = "Behringer"; Devices = @("X32", "Wing", "AVB Switch") }
    )
    
    $foundDevices = @()
    foreach ($vendor in $avbVendors) {
        $vendorMatches = $arpEntries | Where-Object { 
            $_ -match $vendor.Pattern.Replace("-", ".") 
        }
        if ($vendorMatches) {
            $foundDevices += @{
                Vendor = $vendor.Vendor
                Count = $vendorMatches.Count
                Devices = $vendor.Devices
            }
        }
    }
    
    if ($foundDevices.Count -gt 0) {
        Write-Host "  SUCCESS: Potential Milan/AVB devices detected:" -ForegroundColor Green
        foreach ($device in $foundDevices) {
            Write-Host "    $($device.Vendor): $($device.Count) device(s)" -ForegroundColor Green
            Write-Host "      Typical devices: $($device.Devices -join ', ')" -ForegroundColor Gray
        }
        Write-Host ""
        Write-Host "  READY FOR MILAN TESTING!" -ForegroundColor Green
        Write-Host "     Run test_milan_compliance_validation.ps1 to test synchronization" -ForegroundColor White
    } else {
        Write-Host "  INFO: No obvious AVB vendor devices detected" -ForegroundColor Yellow
        Write-Host "     This doesn't mean Milan devices aren't present" -ForegroundColor Gray
        Write-Host "     Many devices use generic MAC addresses" -ForegroundColor Gray
    }
    
} catch {
    Write-Host "  ERROR: Device discovery failed: $($_.Exception.Message)" -ForegroundColor Red
}
Write-Host ""

# Next steps
Write-Host "[4] NEXT STEPS" -ForegroundColor Yellow
if ($foundDevices.Count -gt 0) {
    Write-Host "  Your network appears ready for Milan testing!" -ForegroundColor Green
    Write-Host "  1. Run: .\test_milan_compliance_validation.ps1" -ForegroundColor White
    Write-Host "  2. Monitor for peer discovery and synchronization" -ForegroundColor White
    Write-Host "  3. Check network test results for Milan compliance" -ForegroundColor White
} else {
    Write-Host "  To prepare for Milan testing:" -ForegroundColor Yellow
    Write-Host "  1. Connect Milan-compatible devices to network" -ForegroundColor White
    Write-Host "  2. Power on devices and enable AVB/Milan mode" -ForegroundColor White
    Write-Host "  3. Verify device firmware supports Milan" -ForegroundColor White
    Write-Host "  4. Re-run this script to verify detection" -ForegroundColor White
    Write-Host "  5. Run test_milan_compliance_validation.ps1" -ForegroundColor White
}
Write-Host ""

Write-Host "=== SETUP VERIFICATION COMPLETE ===" -ForegroundColor Green
Write-Host "For Milan specification compliance documentation:" -ForegroundColor Gray
Write-Host "https://avnu.org/milan/" -ForegroundColor Gray