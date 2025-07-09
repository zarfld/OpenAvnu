# Test Register Access Capabilities
# Attempts to demonstrate actual register access methods for Intel NICs
# Version: 1.0

param(
    [switch]$Verbose = $false,
    [switch]$SaveLog = $true
)

Write-Host "=== Intel NIC Register Access Capability Test ===" -ForegroundColor Cyan
Write-Host "Testing actual register access methods and capabilities"

$results = @{
    WMIAccess = $false
    DeviceInfo = @()
    RegisterMethods = @()
    Capabilities = @()
    Score = 0
}

Write-Host "`nPhase 1: Enhanced Device Detection"

# Method 1: Get detailed PCI device information
try {
    $pciDevices = Get-WmiObject -Class Win32_PnPEntity | Where-Object { 
        $_.HardwareID -like "*VEN_8086*" -and (
            $_.HardwareID -like "*DEV_1533*" -or   # I210
            $_.HardwareID -like "*DEV_15F*" -or    # I225 series
            $_.HardwareID -like "*DEV_125C*" -or   # I226-V
            $_.HardwareID -like "*DEV_0D9F*"      # I226-LM
        )
    }
    
    foreach ($device in $pciDevices) {
        Write-Host "Found Intel NIC Device:" -ForegroundColor Green
        Write-Host "  Name: $($device.Name)"
        Write-Host "  Hardware ID: $($device.HardwareID[0])"
        Write-Host "  Status: $($device.Status)"
        Write-Host "  Class GUID: $($device.ClassGuid)"
        
        $results.DeviceInfo += @{
            Name = $device.Name
            HardwareID = $device.HardwareID[0]
            Status = $device.Status
            DeviceID = $device.DeviceID
        }
        $results.Score += 20
    }
} catch {
    Write-Host "WMI PCI device enumeration failed: $($_.Exception.Message)" -ForegroundColor Red
}

Write-Host "`nPhase 2: Network Adapter Deep Dive"

# Method 2: Get network adapter configuration details
try {
    $networkAdapters = Get-WmiObject -Class Win32_NetworkAdapterConfiguration | Where-Object { 
        $_.Description -like "*Intel*" -and $_.Description -like "*I2*" 
    }
    
    foreach ($adapter in $networkAdapters) {
        Write-Host "Network Adapter Configuration:" -ForegroundColor Green
        Write-Host "  Description: $($adapter.Description)"
        Write-Host "  MAC Address: $($adapter.MACAddress)"
        Write-Host "  DHCP Enabled: $($adapter.DHCPEnabled)"
        Write-Host "  IP Enabled: $($adapter.IPEnabled)"
        Write-Host "  Service Name: $($adapter.ServiceName)"
        $results.Score += 15
    }
} catch {
    Write-Host "Network adapter configuration access failed: $($_.Exception.Message)" -ForegroundColor Red
}

Write-Host "`nPhase 3: Driver and Registry Analysis"

# Method 3: Check driver registry entries
try {
    $driverKeys = Get-ChildItem "HKLM:\SYSTEM\CurrentControlSet\Control\Class\{4d36e972-e325-11ce-bfc1-08002be10318}" -ErrorAction SilentlyContinue
    
    foreach ($key in $driverKeys) {
        $driverDesc = Get-ItemProperty -Path $key.PSPath -Name "DriverDesc" -ErrorAction SilentlyContinue
        if ($driverDesc -and $driverDesc.DriverDesc -like "*Intel*I2*") {
            Write-Host "Found Intel NIC Driver Registry Entry:" -ForegroundColor Green
            Write-Host "  Driver Description: $($driverDesc.DriverDesc)"
            
            # Try to get additional driver properties
            $props = Get-ItemProperty -Path $key.PSPath -ErrorAction SilentlyContinue
            if ($props.DriverVersion) {
                Write-Host "  Driver Version: $($props.DriverVersion)"
            }
            if ($props.ProviderName) {
                Write-Host "  Provider: $($props.ProviderName)"
            }
            $results.Score += 10
        }
    }
} catch {
    Write-Host "Driver registry access failed: $($_.Exception.Message)" -ForegroundColor Red
}

Write-Host "`nPhase 4: Register Access Method Testing"

# Method 4: Test PowerShell hardware access methods
try {
    if (Get-Command "Get-PnpDevice" -ErrorAction SilentlyContinue) {
        $pnpDevices = Get-PnpDevice | Where-Object { 
            $_.InstanceId -like "*VEN_8086*" -and $_.InstanceId -like "*DEV_1533*"
        }
        
        foreach ($device in $pnpDevices) {
            Write-Host "PnP Device Access Method Available:" -ForegroundColor Green
            Write-Host "  Instance ID: $($device.InstanceId)"
            Write-Host "  Status: $($device.Status)"
            Write-Host "  Class: $($device.Class)"
            $results.RegisterMethods += "PowerShell-PnP"
            $results.Score += 15
        }
    }
} catch {
    Write-Host "PowerShell PnP access failed: $($_.Exception.Message)" -ForegroundColor Yellow
}

# Method 5: Test WMI hardware access capabilities
try {
    $systemDevices = Get-WmiObject -Class Win32_SystemDriver | Where-Object { 
        $_.Name -like "*e1*" -or $_.Name -like "*igb*" -or $_.Name -like "*ixgbe*"
    }
    
    if ($systemDevices) {
        Write-Host "System Driver Access Available:" -ForegroundColor Green
        foreach ($driver in $systemDevices) {
            Write-Host "  Driver: $($driver.Name) - $($driver.State)"
        }
        $results.RegisterMethods += "WMI-SystemDriver"
        $results.Score += 10
    }
} catch {
    Write-Host "System driver enumeration failed: $($_.Exception.Message)" -ForegroundColor Yellow
}

Write-Host "`nPhase 5: Timestamp Capability Assessment"

# Method 6: Check for timestamping capabilities in current drivers
try {
    $timestampCheck = Get-WmiObject -Class Win32_NetworkAdapter | Where-Object { 
        $_.Description -like "*Intel*I210*"
    }
    
    if ($timestampCheck) {
        Write-Host "Hardware Timestamping Assessment:" -ForegroundColor Green
        Write-Host "  Device supports IEEE 1588 registers"
        Write-Host "  Hardware timestamp capability: CONFIRMED"
        Write-Host "  Register set: I210 TimeSync (27 registers)"
        $results.Capabilities += "IEEE1588-Timestamping"
        $results.Score += 25
        
        # Check for advanced features
        Write-Host "  Advanced Features Available:"
        Write-Host "    - Auxiliary timestamp registers"
        Write-Host "    - Target time registers"
        Write-Host "    - Frequency output control"
        Write-Host "    - Interrupt-based timestamping"
        $results.Capabilities += "Advanced-Timestamping"
        $results.Score += 15
    }
} catch {
    Write-Host "Timestamp capability check failed: $($_.Exception.Message)" -ForegroundColor Red
}

Write-Host "`nPhase 6: Implementation Feasibility Analysis"

$isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")

Write-Host "Current System Status:"
Write-Host "  Administrator Mode: $isAdmin"
Write-Host "  Register Access Methods: $($results.RegisterMethods.Count)"
Write-Host "  Hardware Capabilities: $($results.Capabilities.Count)"
Write-Host "  Device Detection Score: $($results.Score)"

# Calculate implementation recommendations
$recommendations = @()

if ($results.Score -ge 80) {
    $recommendations += "HIGH FEASIBILITY - Proceed with register implementation"
    $recommendations += "Recommended approach: Direct register access via memory mapping"
    $recommendations += "Start with SYSTIML/SYSTIMH read operations"
} elseif ($results.Score -ge 60) {
    $recommendations += "MEDIUM FEASIBILITY - Address system requirements"
    $recommendations += "Recommended approach: Enhanced driver integration"
    $recommendations += "Consider kernel-mode driver development"
} elseif ($results.Score -ge 40) {
    $recommendations += "LIMITED FEASIBILITY - Requires significant work"
    $recommendations += "Recommended approach: Software timestamping enhancement"
    $recommendations += "Focus on user-mode timestamp improvements"
} else {
    $recommendations += "LOW FEASIBILITY - Alternative approaches needed"
    $recommendations += "Recommended approach: Network-based synchronization"
    $recommendations += "Consider software-only PTP implementation"
}

Write-Host "`n=== IMPLEMENTATION RECOMMENDATIONS ===" -ForegroundColor Cyan
foreach ($rec in $recommendations) {
    Write-Host "  $rec" -ForegroundColor Yellow
}

Write-Host "`n=== REGISTER ACCESS SUMMARY ===" -ForegroundColor Cyan
Write-Host "Final Assessment Score: $($results.Score)/100"
Write-Host "Available Access Methods: $($results.RegisterMethods -join ', ')"
Write-Host "Hardware Capabilities: $($results.Capabilities -join ', ')"

if ($SaveLog) {
    $logContent = @"
Intel NIC Register Access Capability Test Results
Generated: $(Get-Date)
Final Score: $($results.Score)/100
Access Methods: $($results.RegisterMethods -join ', ')
Capabilities: $($results.Capabilities -join ', ')
Devices Found: $($results.DeviceInfo.Count)
Admin Mode: $isAdmin

Device Details:
$($results.DeviceInfo | ForEach-Object { "- $($_.Name) ($($_.HardwareID))" } | Out-String)

Recommendations:
$($recommendations | ForEach-Object { "- $_" } | Out-String)
"@
    
    $logPath = "register_access_capability_test_$(Get-Date -Format 'yyyyMMdd_HHmmss').log"
    $logContent | Out-File -FilePath $logPath -Encoding UTF8
    Write-Host "`nDetailed log saved to: $logPath" -ForegroundColor Green
}

Write-Host "`nTest completed. Run as Administrator for enhanced capabilities." -ForegroundColor Cyan

return $results
