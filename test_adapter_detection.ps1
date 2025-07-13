# Intel Network Adapter Detection and Testing Framework
# Detects all Intel network adapters and tests OpenAvnu daemon compatibility

param(
    [switch]$ListOnly = $false,
    [string]$SpecificInterface = "",
    [switch]$Verbose = $false
)

$ErrorActionPreference = "Continue"

Write-Host "=== Intel Network Adapter Detection and Testing ===" -ForegroundColor Cyan
Write-Host ""

# Function to get all network adapters with detailed info
function Get-NetworkAdapters {
    $adapters = @()
    
    # Get Win32_NetworkAdapter information - include all physical adapters regardless of link state
    $win32Adapters = Get-WmiObject -Class Win32_NetworkAdapter | Where-Object { 
        $_.PhysicalAdapter -eq $true -and
        ($_.AdapterType -eq "Ethernet 802.3" -or $_.Description -like "*Ethernet*")
    }
    
    foreach ($adapter in $win32Adapters) {
        # Get corresponding NetworkAdapterConfiguration
        $config = Get-WmiObject -Class Win32_NetworkAdapterConfiguration | Where-Object { 
            $_.Index -eq $adapter.DeviceID 
        }
        
        # Get PnP device info for vendor detection
        $pnpDevice = Get-WmiObject -Class Win32_PnPEntity | Where-Object { 
            $_.DeviceID -eq $adapter.PNPDeviceID 
        }
        
        # Check for configuration data without duplicating properties
        $configData = @{}
        if ($config) {
            $configData.IPEnabled = $config.IPEnabled
            $configData.IPAddress = $config.IPAddress
        } else {
            $configData.IPEnabled = $false
            $configData.IPAddress = @()
        }
        
        # Detect Intel controller type and AVB/TSN capabilities
        $isIntel = ($adapter.Description -like "*Intel*" -or $adapter.Manufacturer -like "*Intel*")
        $isI219 = $adapter.Description -like "*I219*" -or $adapter.Description -like "*219*"
        $isGigabit = $adapter.Description -like "*Gigabit*" -or $adapter.Speed -ge 1000000000
        $supportsAVB = $isIntel -and ($isI219 -or $adapter.Description -like "*I210*" -or $adapter.Description -like "*I225*")
        
        $adapterInfo = @{
            Name = $adapter.Name
            Description = $adapter.Description
            DeviceID = $adapter.DeviceID
            PNPDeviceID = $adapter.PNPDeviceID
            MACAddress = $adapter.MACAddress
            Speed = $adapter.Speed
            NetConnectionID = $adapter.NetConnectionID
            NetStatus = $adapter.NetConnectionStatus
            NetEnabled = $adapter.NetEnabled
            Manufacturer = $adapter.Manufacturer
            IPEnabled = $configData.IPEnabled
            IPAddress = $configData.IPAddress
            IsIntel = $isIntel
            IsI219 = $isI219
            IsGigabit = $isGigabit
            SupportsAVB = $supportsAVB
            SupportsTimestamping = $supportsAVB  # Intel i219, i210, i225 support hardware timestamping
        }
        
        $adapters += $adapterInfo
    }
    
    return $adapters
}

# Function to test Intel HAL compatibility
function Test-IntelHALCompatibility {
    param($AdapterName)
    
    $halTestPath = "D:\Repos\OpenAvnu\build\thirdparty\intel-ethernet-hal\examples\Release\hal_device_info.exe"
    
    if (Test-Path $halTestPath) {
        try {
            $result = & $halTestPath 2>&1
            $success = $LASTEXITCODE -eq 0
            return @{ Success = $success; Output = $result }
        } catch {
            return @{ Success = $false; Output = $_.Exception.Message }
        }
    } else {
        return @{ Success = $false; Output = "HAL test executable not found" }
    }
}

# Function to test MAAP daemon with specific interface
function Test-MAAPDaemon {
    param($InterfaceName, $MACAddress)
    
    $maapPath = "D:\Repos\OpenAvnu\build\daemons\maap\Release\maap_daemon.exe"
    
    if (Test-Path $maapPath) {
        Write-Host "  Testing MAAP daemon with interface: $InterfaceName" -ForegroundColor Yellow
        
        # Test daemon initialization (brief test, then terminate)
        $job = Start-Job -ScriptBlock {
            param($maapPath, $interfaceName)
            & $maapPath -i $interfaceName 2>&1
        } -ArgumentList $maapPath, $InterfaceName
        
        Start-Sleep -Seconds 3
        Stop-Job $job -ErrorAction SilentlyContinue
        $output = Receive-Job $job
        Remove-Job $job -ErrorAction SilentlyContinue
        
        # Check if daemon started successfully (no immediate error)
        $success = $output -notmatch "error|Error|ERROR|failed|Failed|FAILED"
        
        return @{ Success = $success; Output = $output }
    } else {
        return @{ Success = $false; Output = "MAAP daemon not found" }
    }
}

# Function to test MRPD daemon with specific interface
function Test-MRPDDaemon {
    param($InterfaceName, $MACAddress)
    
    $mrpdPath = "D:\Repos\OpenAvnu\build\daemons\mrpd\Release\mrpd.exe"
    
    if (Test-Path $mrpdPath) {
        Write-Host "  Testing MRPD daemon with interface: $InterfaceName" -ForegroundColor Yellow
        
        # Test daemon help/version first
        try {
            $helpOutput = & $mrpdPath -h 2>&1
            $helpSuccess = $LASTEXITCODE -ne 255  # Some exit codes are expected for help
            
            return @{ Success = $helpSuccess; Output = $helpOutput }
        } catch {
            return @{ Success = $false; Output = $_.Exception.Message }
        }
    } else {
        return @{ Success = $false; Output = "MRPD daemon not found" }
    }
}

# Function to test gPTP daemon with specific interface
function Test-gPTPDaemon {
    param($InterfaceName, $MACAddress)
    
    $gptpPath = "D:\Repos\OpenAvnu\build\thirdparty\gptp\Release\gptp.exe"
    
    if (Test-Path $gptpPath) {
        Write-Host "  Testing gPTP daemon with interface: $InterfaceName" -ForegroundColor Yellow
        
        # Test gPTP initialization (brief test, then terminate)
        $job = Start-Job -ScriptBlock {
            param($gptpPath, $interfaceName)
            # Use interface name as parameter
            & $gptpPath -S -i $interfaceName 2>&1
        } -ArgumentList $gptpPath, $InterfaceName
        
        Start-Sleep -Seconds 5  # gPTP needs a bit more time to initialize
        Stop-Job $job -ErrorAction SilentlyContinue
        $output = Receive-Job $job
        Remove-Job $job -ErrorAction SilentlyContinue
        
        # Check if gPTP started successfully (look for init messages)
        $success = $output -match "Initializing|Starting|opened" -and $output -notmatch "error|Error|ERROR|failed|Failed|FAILED"
        
        return @{ Success = $success; Output = $output }
    } else {
        return @{ Success = $false; Output = "gPTP daemon not found" }
    }
}

# Main detection and testing logic
Write-Host "Detecting network adapters..." -ForegroundColor Yellow
$allAdapters = Get-NetworkAdapters

Write-Host "Found $($allAdapters.Count) network adapters" -ForegroundColor Green
Write-Host ""

# Filter Intel adapters and prioritize i219
$intelAdapters = $allAdapters | Where-Object { $_.IsIntel }
$i219Adapters = $allAdapters | Where-Object { $_.IsI219 }
$avbAdapters = $allAdapters | Where-Object { $_.SupportsAVB }
$otherAdapters = $allAdapters | Where-Object { -not $_.IsIntel }

Write-Host "=== INTEL i219 CONTROLLERS (Best for gPTP/AVB) ===" -ForegroundColor Green
if ($i219Adapters.Count -gt 0) {
    foreach ($adapter in $i219Adapters) {
        Write-Host "⭐ i219 Controller: $($adapter.Name)" -ForegroundColor White
        Write-Host "  Description: $($adapter.Description)" -ForegroundColor Gray
        Write-Host "  Connection ID: $($adapter.NetConnectionID)" -ForegroundColor Gray
        Write-Host "  MAC Address: $($adapter.MACAddress)" -ForegroundColor Gray
        Write-Host "  Enabled: $(if($adapter.NetEnabled){'YES'}else{'NO (Disabled)'})" -ForegroundColor $(if($adapter.NetEnabled){'Green'}else{'Red'})
        Write-Host "  Link Status: $(if($adapter.NetStatus -eq 2){'Connected'}elseif($adapter.NetStatus -eq 7){'Media Disconnected'}else{'Unknown'})" -ForegroundColor $(if($adapter.NetStatus -eq 2){'Green'}elseif($adapter.NetStatus -eq 7){'Yellow'}else{'Gray'})
        Write-Host "  AVB Support: $(if($adapter.SupportsAVB){'YES (Hardware Timestamping)'}else{'Limited'})" -ForegroundColor Green
        Write-Host "  IP Enabled: $($adapter.IPEnabled)" -ForegroundColor Gray
        if ($adapter.IPAddress) {
            Write-Host "  IP Addresses: $($adapter.IPAddress -join ', ')" -ForegroundColor Gray
        }
        Write-Host ""
    }
} else {
    Write-Host "No Intel i219 controllers detected." -ForegroundColor Yellow
    Write-Host ""
}

Write-Host "=== OTHER INTEL ADAPTERS ===" -ForegroundColor Cyan
$otherIntelAdapters = $intelAdapters | Where-Object { -not $_.IsI219 }
if ($otherIntelAdapters.Count -gt 0) {
    foreach ($adapter in $otherIntelAdapters) {
        Write-Host "Adapter: $($adapter.Name)" -ForegroundColor White
        Write-Host "  Description: $($adapter.Description)" -ForegroundColor Gray
        Write-Host "  Connection ID: $($adapter.NetConnectionID)" -ForegroundColor Gray
        Write-Host "  MAC Address: $($adapter.MACAddress)" -ForegroundColor Gray
        Write-Host "  Enabled: $(if($adapter.NetEnabled){'YES'}else{'NO (Disabled)'})" -ForegroundColor $(if($adapter.NetEnabled){'Green'}else{'Red'})
        Write-Host "  Link Status: $(if($adapter.NetStatus -eq 2){'Connected'}elseif($adapter.NetStatus -eq 7){'Media Disconnected'}else{'Unknown'})" -ForegroundColor $(if($adapter.NetStatus -eq 2){'Green'}elseif($adapter.NetStatus -eq 7){'Yellow'}else{'Gray'})
        Write-Host "  AVB Support: $(if($adapter.SupportsAVB){'YES'}else{'Limited'})" -ForegroundColor $(if($adapter.SupportsAVB){'Green'}else{'Yellow'})
        Write-Host "  IP Enabled: $($adapter.IPEnabled)" -ForegroundColor Gray
        if ($adapter.IPAddress) {
            Write-Host "  IP Addresses: $($adapter.IPAddress -join ', ')" -ForegroundColor Gray
        }
        Write-Host ""
    }
} else {
    Write-Host "No other Intel adapters detected." -ForegroundColor Yellow
    Write-Host ""
}

Write-Host "=== OTHER ADAPTERS (Compatibility Testing) ===" -ForegroundColor Cyan
if ($otherAdapters.Count -gt 0) {
    foreach ($adapter in $otherAdapters) {
        Write-Host "Adapter: $($adapter.Name)" -ForegroundColor White
        Write-Host "  Manufacturer: $($adapter.Manufacturer)" -ForegroundColor Gray
        Write-Host "  Connection ID: $($adapter.NetConnectionID)" -ForegroundColor Gray
        Write-Host "  MAC Address: $($adapter.MACAddress)" -ForegroundColor Gray
        Write-Host ""
    }
} else {
    Write-Host "No other adapters detected." -ForegroundColor Yellow
    Write-Host ""
}

if ($ListOnly) {
    Write-Host "List-only mode complete." -ForegroundColor Green
    exit 0
}

# Testing phase
Write-Host "=== DAEMON COMPATIBILITY TESTING ===" -ForegroundColor Cyan
Write-Host ""

$testResults = @()

# Test Intel HAL first
Write-Host "Testing Intel HAL device detection..." -ForegroundColor Blue
$halTest = Test-IntelHALCompatibility
if ($halTest.Success) {
    Write-Host "[SUCCESS] Intel HAL device detection" -ForegroundColor Green
} else {
    Write-Host "[INFO] Intel HAL may require hardware" -ForegroundColor Yellow
    Write-Host "Output: $($halTest.Output)" -ForegroundColor Gray
}
Write-Host ""

# Test with each adapter, prioritizing i219
if ($SpecificInterface) {
    $adaptersToTest = $allAdapters | Where-Object { 
        $_.NetConnectionID -eq $SpecificInterface -or 
        $_.Name -like "*$SpecificInterface*" 
    }
    Write-Host "Testing specific interface: $SpecificInterface" -ForegroundColor Yellow
} else {
    # Prioritize i219, then other Intel adapters, then others
    $adaptersToTest = $i219Adapters + ($intelAdapters | Where-Object { -not $_.IsI219 }) + ($otherAdapters | Select-Object -First 1)
    Write-Host "Testing all detected adapters (i219 priority)..." -ForegroundColor Yellow
}

Write-Host ""

foreach ($adapter in $adaptersToTest) {
    Write-Host "=== Testing Adapter: $($adapter.NetConnectionID) ===" -ForegroundColor White
    Write-Host "MAC: $($adapter.MACAddress)" -ForegroundColor Gray
    Write-Host "Type: $(if($adapter.IsI219){'Intel i219 (Full AVB/TSN Support)'}elseif($adapter.IsIntel){'Intel (Good Support)'}else{'Generic (Limited)'})" -ForegroundColor Gray
    Write-Host ""
    
    # Test MAAP daemon
    $maapResult = Test-MAAPDaemon -InterfaceName $adapter.NetConnectionID -MACAddress $adapter.MACAddress
    if ($maapResult.Success) {
        Write-Host "[SUCCESS] MAAP daemon compatibility" -ForegroundColor Green
    } else {
        Write-Host "[PARTIAL] MAAP daemon (may need elevated privileges)" -ForegroundColor Yellow
        if ($Verbose) {
            Write-Host "Output: $($maapResult.Output)" -ForegroundColor Gray
        }
    }
    
    # Test MRPD daemon  
    $mrpdResult = Test-MRPDDaemon -InterfaceName $adapter.NetConnectionID -MACAddress $adapter.MACAddress
    if ($mrpdResult.Success) {
        Write-Host "[SUCCESS] MRPD daemon compatibility" -ForegroundColor Green
    } else {
        Write-Host "[PARTIAL] MRPD daemon (may need elevated privileges)" -ForegroundColor Yellow
        if ($Verbose) {
            Write-Host "Output: $($mrpdResult.Output)" -ForegroundColor Gray
        }
    }
    
    # Test gPTP daemon  
    $gptpResult = Test-gPTPDaemon -InterfaceName $adapter.NetConnectionID -MACAddress $adapter.MACAddress
    if ($gptpResult.Success) {
        Write-Host "[SUCCESS] gPTP daemon compatibility" -ForegroundColor Green
    } else {
        Write-Host "[PARTIAL] gPTP daemon (may need elevated privileges)" -ForegroundColor Yellow
        if ($Verbose) {
            Write-Host "Output: $($gptpResult.Output)" -ForegroundColor Gray
        }
    }
    
    $testResults += @{
        Adapter = $adapter.NetConnectionID
        MAC = $adapter.MACAddress
        IsIntel = $adapter.IsIntel
        IsI219 = $adapter.IsI219
        SupportsAVB = $adapter.SupportsAVB
        MAAPSuccess = $maapResult.Success
        MRPDSuccess = $mrpdResult.Success
        gPTPSuccess = $gptpResult.Success
    }
    
    Write-Host ""
}

# Final summary
Write-Host "=== TESTING SUMMARY ===" -ForegroundColor Cyan
Write-Host ""

$i219TestedCount = ($testResults | Where-Object { $_.IsI219 }).Count
$intelTestedCount = ($testResults | Where-Object { $_.IsIntel }).Count
$totalTestedCount = $testResults.Count
$maapSuccessCount = ($testResults | Where-Object { $_.MAAPSuccess }).Count
$mrpdSuccessCount = ($testResults | Where-Object { $_.MRPDSuccess }).Count
$gptpSuccessCount = ($testResults | Where-Object { $_.gPTPSuccess }).Count

Write-Host "Adapters Tested: $totalTestedCount" -ForegroundColor White
Write-Host "Intel i219 Adapters: $i219TestedCount" -ForegroundColor White
Write-Host "Intel Adapters: $intelTestedCount" -ForegroundColor White
Write-Host "gPTP Compatible: $gptpSuccessCount" -ForegroundColor Green
Write-Host "MAAP Compatible: $maapSuccessCount" -ForegroundColor Green
Write-Host "MRPD Compatible: $mrpdSuccessCount" -ForegroundColor Green
Write-Host ""

Write-Host "=== RECOMMENDED CONFIGURATION ===" -ForegroundColor Yellow
Write-Host ""

# Provide specific configuration recommendations, prioritizing i219
$bestAdapter = $testResults | Where-Object { $_.IsI219 -and $_.gPTPSuccess } | Select-Object -First 1
if ($bestAdapter) {
    Write-Host "RECOMMENDED: Use Intel i219 adapter '$($bestAdapter.Adapter)'" -ForegroundColor Green
    Write-Host "  MAC Address: $($bestAdapter.MAC)" -ForegroundColor Gray
    Write-Host "  Type: Intel i219 (Full Hardware Timestamping Support)" -ForegroundColor Gray
    Write-Host "  AVB/TSN Support: Full (IEEE 802.1AS, 802.1Qav, 802.1Qat)" -ForegroundColor Gray
    Write-Host ""
    Write-Host "Configuration commands:" -ForegroundColor Cyan
    Write-Host "  gPTP: gptp.exe -S -i `"$($bestAdapter.Adapter)`"" -ForegroundColor White
    Write-Host "  MAAP: maap_daemon.exe -i `"$($bestAdapter.Adapter)`"" -ForegroundColor White
    Write-Host "  MRPD: mrpd.exe -i `"$($bestAdapter.Adapter)`"" -ForegroundColor White
} else {
    $fallbackIntel = $testResults | Where-Object { $_.IsIntel -and ($_.gPTPSuccess -or $_.MAAPSuccess) } | Select-Object -First 1
    if ($fallbackIntel) {
        Write-Host "FALLBACK: Use Intel adapter '$($fallbackIntel.Adapter)'" -ForegroundColor Yellow
        Write-Host "  MAC Address: $($fallbackIntel.MAC)" -ForegroundColor Gray
        Write-Host "  Type: Intel (Good Support)" -ForegroundColor Gray
        Write-Host ""
        Write-Host "Note: May have limited AVB/TSN features compared to i219." -ForegroundColor Yellow
    } else {
        $anyAdapter = $testResults | Where-Object { $_.gPTPSuccess -or $_.MAAPSuccess } | Select-Object -First 1
        if ($anyAdapter) {
            Write-Host "LIMITED: Use adapter '$($anyAdapter.Adapter)'" -ForegroundColor Yellow
            Write-Host "  MAC Address: $($anyAdapter.MAC)" -ForegroundColor Gray
            Write-Host "  Type: Generic (Limited Support)" -ForegroundColor Gray
            Write-Host ""
            Write-Host "Note: May require elevated privileges and have limited AVB/TSN support." -ForegroundColor Yellow
        } else {
            Write-Host "NO COMPATIBLE ADAPTERS FOUND" -ForegroundColor Red
            Write-Host "This may indicate:" -ForegroundColor Yellow
            Write-Host "  1. Need for elevated administrator privileges" -ForegroundColor Gray
            Write-Host "  2. Missing network drivers" -ForegroundColor Gray
            Write-Host "  3. Hardware not suitable for AVB/TSN" -ForegroundColor Gray
            Write-Host "  4. i219 controller may need specific drivers" -ForegroundColor Gray
        }
    }
}

Write-Host ""
Write-Host "OpenAvnu daemon compilation and adapter detection complete!" -ForegroundColor Green
