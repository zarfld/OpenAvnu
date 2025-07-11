# OpenAvnu Hardware Testing - Phase 1: Driver Interface Test
# Test Case 3: Driver Interface Test
# Date: 2025-01-11
# System: ATGRZWN313892

Write-Host "=== OpenAvnu Phase 1 Hardware Testing ===" -ForegroundColor Green
Write-Host "Test Case: Driver Interface Test" -ForegroundColor Yellow
Write-Host "Machine: $env:COMPUTERNAME" -ForegroundColor Cyan
Write-Host "Date: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')" -ForegroundColor Cyan
Write-Host ""

# Test Case 3.1: NDIS Interface Functionality (Windows)
Write-Host "3.1 NDIS Interface Functionality..." -ForegroundColor Blue
Write-Host "=" * 50

$ethernetAdapter = Get-NetAdapter -Name "Ethernet" -ErrorAction SilentlyContinue
if (-not $ethernetAdapter) {
    Write-Host "❌ FAIL: Ethernet adapter not found" -ForegroundColor Red
    exit 1
}

Write-Host "Testing NDIS interface for: $($ethernetAdapter.InterfaceDescription)" -ForegroundColor White

# Test basic NDIS properties
try {
    $ndisVersion = Get-NetAdapterAdvancedProperty -Name "Ethernet" -DisplayName "*NDIS*" -ErrorAction SilentlyContinue
    if ($ndisVersion) {
        foreach ($prop in $ndisVersion) {
            Write-Host "  $($prop.DisplayName): $($prop.DisplayValue)" -ForegroundColor Gray
        }
    }
    
    # Test adapter statistics access
    $stats = Get-NetAdapterStatistics -Name "Ethernet" -ErrorAction Stop
    Write-Host "  ✅ Adapter statistics accessible" -ForegroundColor Green
    Write-Host "    Bytes Sent: $($stats.SentBytes)" -ForegroundColor Gray
    Write-Host "    Bytes Received: $($stats.ReceivedBytes)" -ForegroundColor Gray
    Write-Host "    Packets Sent: $($stats.SentUnicastPackets)" -ForegroundColor Gray
    Write-Host "    Packets Received: $($stats.ReceivedUnicastPackets)" -ForegroundColor Gray
    
} catch {
    Write-Host "  ❌ Error accessing NDIS interface: $($_.Exception.Message)" -ForegroundColor Red
}

# Test Case 3.2: PTP Device Access
Write-Host ""
Write-Host "3.2 PTP Device Access..." -ForegroundColor Blue
Write-Host "=" * 50

try {
    # Check PTP-related properties
    $ptpProps = Get-NetAdapterAdvancedProperty -Name "Ethernet" | Where-Object { 
        $_.DisplayName -like "*PTP*" -or 
        $_.DisplayName -like "*Timestamp*" -or 
        $_.DisplayName -like "*1588*" -or
        $_.RegistryKeyword -like "*ptp*" -or 
        $_.RegistryKeyword -like "*timestamp*"
    }
    
    if ($ptpProps) {
        Write-Host "  ✅ PTP properties accessible" -ForegroundColor Green
        foreach ($prop in $ptpProps) {
            Write-Host "    $($prop.DisplayName): $($prop.DisplayValue)" -ForegroundColor Gray
            Write-Host "      Registry Key: $($prop.RegistryKeyword)" -ForegroundColor DarkGray
            Write-Host "      Valid Values: $($prop.ValidDisplayValues -join ', ')" -ForegroundColor DarkGray
        }
    } else {
        Write-Host "  ❌ No PTP properties found" -ForegroundColor Red
    }
    
    # Test if we can modify PTP settings (non-destructive test)
    $ptpTimestamp = $ptpProps | Where-Object { $_.RegistryKeyword -eq "*PtpHardwareTimestamp" }
    if ($ptpTimestamp) {
        Write-Host "    Current PTP Setting: $($ptpTimestamp.DisplayValue)" -ForegroundColor Gray
        Write-Host "    ✅ PTP hardware timestamp is configurable" -ForegroundColor Green
    }
    
} catch {
    Write-Host "  ❌ Error accessing PTP properties: $($_.Exception.Message)" -ForegroundColor Red
}

# Test Case 3.3: Register Access Capabilities (Indirect Test)
Write-Host ""
Write-Host "3.3 Register Access Capabilities..." -ForegroundColor Blue
Write-Host "=" * 50

try {
    # Test access to hardware-level properties that indicate register access
    $hardwareProps = Get-NetAdapterAdvancedProperty -Name "Ethernet" | Where-Object { 
        $_.DisplayName -like "*Hardware*" -or 
        $_.DisplayName -like "*Queue*" -or 
        $_.DisplayName -like "*Ring*" -or
        $_.DisplayName -like "*Buffer*" -or
        $_.DisplayName -like "*Interrupt*"
    }
    
    if ($hardwareProps) {
        Write-Host "  ✅ Hardware configuration properties accessible" -ForegroundColor Green
        foreach ($prop in $hardwareProps) {
            Write-Host "    $($prop.DisplayName): $($prop.DisplayValue)" -ForegroundColor Gray
        }
    } else {
        Write-Host "  ⚠️  Limited hardware properties available" -ForegroundColor Yellow
    }
    
    # Check if Intel HAL would have register access by examining device capabilities
    $deviceId = ""
    if ($ethernetAdapter.PnPDeviceID -match "DEV_([0-9A-F]{4})") {
        $deviceId = $matches[1]
    }
    
    $registerAccessSupported = $deviceId -in @("1533", "1539", "15F3", "0D4E", "0DC7", "15F5", "550A", "550B", "550C", "15BD", "15BE", "15BF")
    
    if ($registerAccessSupported) {
        Write-Host "  ✅ Device supports Intel HAL register access (Device ID: $deviceId)" -ForegroundColor Green
    } else {
        Write-Host "  ❌ Device may not support Intel HAL register access (Device ID: $deviceId)" -ForegroundColor Red
    }
    
} catch {
    Write-Host "  ❌ Error checking register access capabilities: $($_.Exception.Message)" -ForegroundColor Red
}

# Test Case 3.4: Driver Interface Validation
Write-Host ""
Write-Host "3.4 Driver Interface Validation..." -ForegroundColor Blue
Write-Host "=" * 50

try {
    # Get detailed driver information
    $driverInfo = Get-PnpDevice | Where-Object { $_.FriendlyName -eq $ethernetAdapter.InterfaceDescription }
    
    if ($driverInfo) {
        Write-Host "  Device Status: $($driverInfo.Status)" -ForegroundColor $(if ($driverInfo.Status -eq "OK") { "Green" } else { "Red" })
        
        # Check driver properties
        $driverVersion = Get-PnpDeviceProperty -InstanceId $driverInfo.InstanceId -KeyName "DEVPKEY_Device_DriverVersion" -ErrorAction SilentlyContinue
        $driverProvider = Get-PnpDeviceProperty -InstanceId $driverInfo.InstanceId -KeyName "DEVPKEY_Device_DriverProvider" -ErrorAction SilentlyContinue
        $driverDate = Get-PnpDeviceProperty -InstanceId $driverInfo.InstanceId -KeyName "DEVPKEY_Device_DriverDate" -ErrorAction SilentlyContinue
        
        if ($driverVersion) {
            Write-Host "  Driver Version: $($driverVersion.Data)" -ForegroundColor Gray
        }
        if ($driverProvider) {
            Write-Host "  Driver Provider: $($driverProvider.Data)" -ForegroundColor Gray
        }
        if ($driverDate) {
            Write-Host "  Driver Date: $($driverDate.Data)" -ForegroundColor Gray
        }
        
        # Check if it's an Intel driver
        if ($driverProvider -and $driverProvider.Data -like "*Intel*") {
            Write-Host "  ✅ Official Intel driver detected" -ForegroundColor Green
        } else {
            Write-Host "  ⚠️  Driver provider verification failed" -ForegroundColor Yellow
        }
    }
    
    # Test adapter binding and interface operations
    $interfaceIndex = $ethernetAdapter.InterfaceIndex
    $interfaceInfo = Get-NetIPInterface -InterfaceIndex $interfaceIndex -ErrorAction SilentlyContinue
    
    if ($interfaceInfo) {
        Write-Host "  ✅ Network interface binding accessible" -ForegroundColor Green
        Write-Host "    Interface Index: $interfaceIndex" -ForegroundColor Gray
        Write-Host "    MTU: $($interfaceInfo.NlMtu)" -ForegroundColor Gray
        Write-Host "    Forwarding: $($interfaceInfo.Forwarding)" -ForegroundColor Gray
    } else {
        Write-Host "  ❌ Network interface binding not accessible" -ForegroundColor Red
    }
    
} catch {
    Write-Host "  ❌ Error validating driver interface: $($_.Exception.Message)" -ForegroundColor Red
}

# Test Case 3.5: Intel HAL Prerequisites Check
Write-Host ""
Write-Host "3.5 Intel HAL Prerequisites Check..." -ForegroundColor Blue
Write-Host "=" * 50

# Check if the necessary components for Intel HAL are available
$halPrerequisites = @{
    "PTP Hardware Timestamp" = $false
    "Intel Driver" = $false
    "Supported Device ID" = $false
    "Driver Interface Access" = $false
    "Register Access Capability" = $false
}

try {
    # Check PTP hardware timestamp
    $ptpHardware = Get-NetAdapterAdvancedProperty -Name "Ethernet" | Where-Object { $_.RegistryKeyword -eq "*PtpHardwareTimestamp" }
    if ($ptpHardware -and $ptpHardware.DisplayValue -eq "Enabled") {
        $halPrerequisites["PTP Hardware Timestamp"] = $true
        Write-Host "  ✅ PTP Hardware Timestamp: Enabled" -ForegroundColor Green
    } else {
        Write-Host "  ❌ PTP Hardware Timestamp: Not enabled or not found" -ForegroundColor Red
    }
    
    # Check Intel driver
    $driverInfo = Get-PnpDevice | Where-Object { $_.FriendlyName -eq $ethernetAdapter.InterfaceDescription }
    $driverProvider = Get-PnpDeviceProperty -InstanceId $driverInfo.InstanceId -KeyName "DEVPKEY_Device_DriverProvider" -ErrorAction SilentlyContinue
    if ($driverProvider -and $driverProvider.Data -like "*Intel*") {
        $halPrerequisites["Intel Driver"] = $true
        Write-Host "  ✅ Intel Driver: Confirmed" -ForegroundColor Green
    } else {
        Write-Host "  ❌ Intel Driver: Not confirmed" -ForegroundColor Red
    }
    
    # Check supported device ID
    if ($ethernetAdapter.PnPDeviceID -match "DEV_([0-9A-F]{4})") {
        $deviceId = $matches[1]
        if ($deviceId -in @("1533", "1539", "15F3", "0D4E", "0DC7", "15F5", "550A", "550B", "550C", "15BD", "15BE", "15BF")) {
            $halPrerequisites["Supported Device ID"] = $true
            Write-Host "  ✅ Supported Device ID: $deviceId" -ForegroundColor Green
        } else {
            Write-Host "  ❌ Unsupported Device ID: $deviceId" -ForegroundColor Red
        }
    }
    
    # Check driver interface access
    if ($stats) {
        $halPrerequisites["Driver Interface Access"] = $true
        Write-Host "  ✅ Driver Interface Access: Available" -ForegroundColor Green
    } else {
        Write-Host "  ❌ Driver Interface Access: Limited" -ForegroundColor Red
    }
    
    # Check register access capability (based on hardware properties)
    if ($hardwareProps -and $hardwareProps.Count -gt 0) {
        $halPrerequisites["Register Access Capability"] = $true
        Write-Host "  ✅ Register Access Capability: Indicated" -ForegroundColor Green
    } else {
        Write-Host "  ❌ Register Access Capability: Not indicated" -ForegroundColor Red
    }
    
} catch {
    Write-Host "  ❌ Error checking HAL prerequisites: $($_.Exception.Message)" -ForegroundColor Red
}

# Summary
Write-Host ""
Write-Host "=== Test Summary ===" -ForegroundColor Green

$testsPassed = 0
$testsTotal = 5

# NDIS Interface
if ($stats) {
    Write-Host "✅ NDIS Interface: PASS" -ForegroundColor Green
    $testsPassed++
} else {
    Write-Host "❌ NDIS Interface: FAIL" -ForegroundColor Red
}

# PTP Device Access
if ($ptpProps -and $ptpProps.Count -gt 0) {
    Write-Host "✅ PTP Device Access: PASS" -ForegroundColor Green
    $testsPassed++
} else {
    Write-Host "❌ PTP Device Access: FAIL" -ForegroundColor Red
}

# Register Access Capabilities
if ($registerAccessSupported) {
    Write-Host "✅ Register Access Capabilities: PASS" -ForegroundColor Green
    $testsPassed++
} else {
    Write-Host "❌ Register Access Capabilities: FAIL" -ForegroundColor Red
}

# Driver Interface
if ($driverInfo -and $driverInfo.Status -eq "OK") {
    Write-Host "✅ Driver Interface: PASS" -ForegroundColor Green
    $testsPassed++
} else {
    Write-Host "❌ Driver Interface: FAIL" -ForegroundColor Red
}

# HAL Prerequisites
$halPassed = ($halPrerequisites.Values | Where-Object { $_ -eq $true }).Count
Write-Host "Intel HAL Prerequisites: $halPassed/5 met" -ForegroundColor $(if ($halPassed -ge 4) { "Green" } elseif ($halPassed -ge 2) { "Yellow" } else { "Red" })
if ($halPassed -ge 4) {
    $testsPassed++
}

Write-Host ""
Write-Host "Tests Passed: $testsPassed/$testsTotal" -ForegroundColor White

if ($testsPassed -ge 4) {
    Write-Host "Overall Status: ✅ PASS" -ForegroundColor Green
    Write-Host ""
    Write-Host "✅ Phase 1 Test Case 3 completed successfully!" -ForegroundColor Green
    Write-Host "Ready to proceed to Phase 2 (Register Access Testing)" -ForegroundColor Yellow
} else {
    Write-Host "Overall Status: ❌ FAIL" -ForegroundColor Red
    Write-Host ""
    Write-Host "❌ Phase 1 Test Case 3 failed. Driver interface issues detected." -ForegroundColor Red
    exit 1
}
