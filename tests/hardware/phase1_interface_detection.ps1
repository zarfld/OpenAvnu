# OpenAvnu Hardware Testing - Phase 1: Network Interface Detection
# Test Case 1: Detect Intel I210/I219/I225/I226 adapters
# Date: 2025-01-11
# System: DEV01

Write-Host "=== OpenAvnu Phase 1 Hardware Testing ===" -ForegroundColor Green
Write-Host "Test Case: Network Interface Detection" -ForegroundColor Yellow
Write-Host "Machine: $env:COMPUTERNAME" -ForegroundColor Cyan
Write-Host "Date: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')" -ForegroundColor Cyan
Write-Host ""

# Test Case 1.1: Detect all Intel network adapters
Write-Host "1.1 Detecting Intel Network Adapters..." -ForegroundColor Blue
Write-Host "=" * 50

$intelAdapters = Get-PnpDevice | Where-Object { 
    $_.FriendlyName -like "*Intel*" -and 
    $_.Class -eq "Net" -and
    $_.Status -eq "OK"
}

if ($intelAdapters.Count -eq 0) {
    Write-Host "❌ FAIL: No Intel network adapters found" -ForegroundColor Red
    exit 1
}

Write-Host "✅ Found $($intelAdapters.Count) Intel network adapter(s)" -ForegroundColor Green
Write-Host ""

foreach ($adapter in $intelAdapters) {
    Write-Host "Adapter: $($adapter.FriendlyName)" -ForegroundColor White
    Write-Host "  PCI ID: $($adapter.InstanceId)" -ForegroundColor Gray
    Write-Host "  Status: $($adapter.Status)" -ForegroundColor Gray
    
    # Extract device ID for specific model detection
    if ($adapter.InstanceId -match "DEV_([0-9A-F]{4})") {
        $deviceId = $matches[1]
        $model = switch ($deviceId) {
            "1533" { "I210" }
            "1539" { "I211" }
            "15F3" { "I219" }
            "0D4E" { "I219-LM" }
            "0DC7" { "I219-LM (22)" }
            "15F5" { "I219-V" }
            "550A" { "I225" }
            "550B" { "I225-LM" }
            "550C" { "I225-V" }
            "15BD" { "I226" }
            "15BE" { "I226-LM" }
            "15BF" { "I226-V" }
            default { "Unknown ($deviceId)" }
        }
        Write-Host "  Model: $model" -ForegroundColor Yellow
        
        # Check if this is a supported TSN model
        $tsnSupported = $deviceId -in @("550A", "550B", "550C", "15BD", "15BE", "15BF")
        $ptpSupported = $deviceId -in @("1533", "1539", "15F3", "0D4E", "0DC7", "15F5", "550A", "550B", "550C", "15BD", "15BE", "15BF")
        
        Write-Host "  TSN Support: $(if ($tsnSupported) { "✅ Yes" } else { "❌ No" })" -ForegroundColor $(if ($tsnSupported) { "Green" } else { "Red" })
        Write-Host "  PTP Support: $(if ($ptpSupported) { "✅ Yes" } else { "❌ No" })" -ForegroundColor $(if ($ptpSupported) { "Green" } else { "Red" })
    }
    Write-Host ""
}

# Test Case 1.2: Verify adapter status and get detailed info
Write-Host "1.2 Network Adapter Status and Configuration..." -ForegroundColor Blue
Write-Host "=" * 50

$netAdapters = Get-NetAdapter | Where-Object { $_.InterfaceDescription -like "*Intel*" }

foreach ($adapter in $netAdapters) {
    Write-Host "Interface: $($adapter.Name) ($($adapter.InterfaceDescription))" -ForegroundColor White
    Write-Host "  Status: $($adapter.Status)" -ForegroundColor $(if ($adapter.Status -eq "Up") { "Green" } else { "Yellow" })
    Write-Host "  Speed: $($adapter.LinkSpeed)" -ForegroundColor Gray
    Write-Host "  Media Type: $($adapter.MediaType)" -ForegroundColor Gray
    Write-Host "  MAC Address: $($adapter.MacAddress)" -ForegroundColor Gray
    
    # Check for PTP capabilities
    try {
        $ptpProps = Get-NetAdapterAdvancedProperty -Name $adapter.Name | Where-Object { 
            $_.DisplayName -like "*PTP*" -or 
            $_.DisplayName -like "*Timestamp*" -or 
            $_.RegistryKeyword -like "*ptp*" -or 
            $_.RegistryKeyword -like "*timestamp*"
        }
        
        if ($ptpProps) {
            Write-Host "  PTP/Timestamp Properties:" -ForegroundColor Green
            foreach ($prop in $ptpProps) {
                Write-Host "    $($prop.DisplayName): $($prop.DisplayValue)" -ForegroundColor Gray
            }
        } else {
            Write-Host "  PTP/Timestamp Properties: None found" -ForegroundColor Red
        }
    } catch {
        Write-Host "  PTP/Timestamp Properties: Error accessing ($($_.Exception.Message))" -ForegroundColor Red
    }
    Write-Host ""
}

# Test Case 1.3: Driver version verification
Write-Host "1.3 Driver Version Information..." -ForegroundColor Blue
Write-Host "=" * 50

foreach ($adapter in $intelAdapters) {
    try {
        $driver = Get-PnpDeviceProperty -InstanceId $adapter.InstanceId -KeyName "DEVPKEY_Device_DriverVersion" -ErrorAction SilentlyContinue
        $driverDate = Get-PnpDeviceProperty -InstanceId $adapter.InstanceId -KeyName "DEVPKEY_Device_DriverDate" -ErrorAction SilentlyContinue
        
        Write-Host "Driver for $($adapter.FriendlyName):" -ForegroundColor White
        if ($driver) {
            Write-Host "  Version: $($driver.Data)" -ForegroundColor Gray
        } else {
            Write-Host "  Version: Unable to retrieve" -ForegroundColor Red
        }
        
        if ($driverDate) {
            Write-Host "  Date: $($driverDate.Data)" -ForegroundColor Gray
        } else {
            Write-Host "  Date: Unable to retrieve" -ForegroundColor Red
        }
    } catch {
        Write-Host "  Error retrieving driver info: $($_.Exception.Message)" -ForegroundColor Red
    }
    Write-Host ""
}

# Summary
Write-Host "=== Test Summary ===" -ForegroundColor Green

# Count PTP-capable adapters
$ptpCapableCount = 0
foreach ($adapter in $intelAdapters) {
    if ($adapter.InstanceId -match "DEV_([0-9A-F]{4})") {
        $deviceId = $matches[1]
        if ($deviceId -in @("1533", "1539", "15F3", "0D4E", "0DC7", "15F5", "550A", "550B", "550C", "15BD", "15BE", "15BF")) {
            $ptpCapableCount++
        }
    }
}

Write-Host "Total Intel Adapters: $($intelAdapters.Count)" -ForegroundColor White
Write-Host "PTP-Capable Adapters: $ptpCapableCount" -ForegroundColor White

if ($ptpCapableCount -gt 0) {
    Write-Host "Test Status: ✅ PASS" -ForegroundColor Green
    Write-Host ""
    Write-Host "✅ Phase 1 Test Case 1 completed successfully!" -ForegroundColor Green
    Write-Host "Ready to proceed to Test Case 2 (Basic Network Communication)" -ForegroundColor Yellow
} else {
    Write-Host "Test Status: ❌ FAIL" -ForegroundColor Red
    Write-Host ""
    Write-Host "❌ No PTP-capable Intel adapters found. Hardware testing cannot proceed." -ForegroundColor Red
    exit 1
}
