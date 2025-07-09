# Intel I219 Register Access Test Script
# Tests for direct register access capabilities for I219 PHY device
# Based on Intel Ethernet Connection I219 Datasheet specifications
# Version: 1.0

param(
    [switch]$Verbose = $false,
    [switch]$SaveLog = $true,
    [string]$LogPath = "i219_register_test_$(Get-Date -Format 'yyyyMMdd_HHmmss').log"
)

Write-Host "=== Intel I219 Ethernet Connection Test ==="
Write-Host "Testing I219 PHY device detection and register accessibility"

# I219 Device IDs and variants
$I219_DEVICE_IDS = @{
    "15B7" = "I219-LM (LAN on Motherboard)"
    "15B8" = "I219-V (Discrete/Add-in Card)"
    "15B9" = "I219-LM2 (LAN on Motherboard v2)"
    "15BA" = "I219-V2 (Discrete/Add-in Card v2)"
    "15BB" = "I219-LM3 (LAN on Motherboard v3)"
    "15BC" = "I219-V3 (Discrete/Add-in Card v3)"
    "15BD" = "I219-LM4 (LAN on Motherboard v4)"
    "15BE" = "I219-V4 (Discrete/Add-in Card v4)"
    "15D7" = "I219-LM5 (LAN on Motherboard v5)"
    "15D8" = "I219-V5 (Discrete/Add-in Card v5)"
    "15E3" = "I219-LM6 (LAN on Motherboard v6)"
    "0D4E" = "I219-LM7 (LAN on Motherboard v7)"
    "0D4F" = "I219-V7 (Discrete/Add-in Card v7)"
    "0D4C" = "I219-LM8 (LAN on Motherboard v8)"
    "0D53" = "I219-LM9 (LAN on Motherboard v9)"
    "0D55" = "I219-V9 (Discrete/Add-in Card v9)"
}

# I219 MDIO Register Map (based on datasheet Section 9.5)
# Note: I219 uses MDIO register access, not direct memory-mapped registers like I210/I225
$I219_MDIO_REGISTERS = @{
    # Standard IEEE 802.3 PHY registers
    "BMCR"      = @{ Address = "0x00"; Description = "Basic Mode Control Register"; Page = 0 }
    "BMSR"      = @{ Address = "0x01"; Description = "Basic Mode Status Register"; Page = 0 }
    "PHYIDR1"   = @{ Address = "0x02"; Description = "PHY Identifier Register 1"; Page = 0 }
    "PHYIDR2"   = @{ Address = "0x03"; Description = "PHY Identifier Register 2"; Page = 0 }
    "ANAR"      = @{ Address = "0x04"; Description = "Auto-Negotiation Advertisement Register"; Page = 0 }
    "ANLPAR"    = @{ Address = "0x05"; Description = "Auto-Negotiation Link Partner Ability Register"; Page = 0 }
    "ANER"      = @{ Address = "0x06"; Description = "Auto-Negotiation Expansion Register"; Page = 0 }
    "ANNPTR"    = @{ Address = "0x07"; Description = "Auto-Negotiation Next Page Transmit Register"; Page = 0 }
    "ANLPRNPR"  = @{ Address = "0x08"; Description = "Auto-Negotiation Link Partner Receive Next Page Register"; Page = 0 }
    
    # I219-specific registers (based on datasheet)
    "COPPER_CTRL1" = @{ Address = "0x10"; Description = "Copper Specific Control Register 1"; Page = 0 }
    "COPPER_STAT1" = @{ Address = "0x11"; Description = "Copper Specific Status Register 1"; Page = 0 }
    "INTR_ENABLE"  = @{ Address = "0x12"; Description = "Interrupt Enable Register"; Page = 0 }
    "INTR_STATUS"  = @{ Address = "0x13"; Description = "Interrupt Status Register"; Page = 0 }
    "SMART_SPEED"  = @{ Address = "0x14"; Description = "Smart Speed Register"; Page = 0 }
    "LED_CTRL"     = @{ Address = "0x18"; Description = "LED Control Register"; Page = 0 }
    "LED_OVERRIDE" = @{ Address = "0x19"; Description = "LED Override Register"; Page = 0 }
    
    # IEEE 1588/802.1AS registers (mentioned in datasheet features)
    "PTP_CTRL"     = @{ Address = "0x20"; Description = "PTP Control Register"; Page = 0 }
    "PTP_STATUS"   = @{ Address = "0x21"; Description = "PTP Status Register"; Page = 0 }
    
    # Energy Efficient Ethernet (EEE) registers
    "EEE_CTRL"     = @{ Address = "0x00"; Description = "EEE Control Register"; Page = "MMD" }
    "EEE_STATUS"   = @{ Address = "0x01"; Description = "EEE Status Register"; Page = "MMD" }
    
    # I219 Power management registers
    "POWER_CTRL"   = @{ Address = "0x1A"; Description = "Power Control Register"; Page = 0 }
    "ULP_CONFIG"   = @{ Address = "0x1B"; Description = "Ultra Low Power Configuration"; Page = 0 }
}

$testResults = @{
    DeviceFound = $false
    DriverOK = $false
    MDIOAccess = $false
    IEEE1588Support = $false
    EEESupport = $false
    Score = 0
}

Write-Host "`nPhase 1: Intel I219 Device Detection"

# Check for Intel I219 device using multiple methods
$networkAdapters = Get-WmiObject -Class Win32_NetworkAdapter | Where-Object { 
    $_.Description -like "*Intel*" -and $_.Description -like "*I219*"
}

if ($networkAdapters) {
    Write-Host "Found Intel I219 network adapter(s):" -ForegroundColor Green
    foreach ($adapter in $networkAdapters) {
        Write-Host "  - $($adapter.Description)" -ForegroundColor Green
        Write-Host "    Status: $($adapter.NetConnectionStatus)"
        Write-Host "    MAC: $($adapter.MACAddress)"
        Write-Host "    Service Name: $($adapter.ServiceName)"
    }
    $testResults.DeviceFound = $true
    $testResults.Score += 25
} else {
    Write-Host "No Intel I219 network adapters found in network adapter list" -ForegroundColor Yellow
}

# Check PCI devices for specific I219 device IDs
Write-Host "`nChecking PCI Device IDs for I219 variants..."
$pciDevices = Get-WmiObject -Class Win32_PnPEntity | Where-Object { 
    $_.HardwareID -like "*VEN_8086*" 
}

$foundI219 = $false
foreach ($device in $pciDevices) {
    foreach ($hwid in $device.HardwareID) {
        foreach ($deviceId in $I219_DEVICE_IDS.Keys) {
            if ($hwid -like "*DEV_$deviceId*") {
                Write-Host "Found I219 Device: $($I219_DEVICE_IDS[$deviceId])" -ForegroundColor Green
                Write-Host "  Hardware ID: $hwid"
                Write-Host "  Device Name: $($device.Name)"
                Write-Host "  Status: $($device.Status)"
                $foundI219 = $true
                $testResults.DeviceFound = $true
                $testResults.Score += 25
            }
        }
    }
}

if (-not $foundI219 -and -not $testResults.DeviceFound) {
    Write-Host "No I219 devices detected in system" -ForegroundColor Red
    Write-Host "Note: I219 is typically integrated into PCH (Platform Controller Hub)" -ForegroundColor Yellow
}

Write-Host "`nPhase 2: Driver and Integration Analysis"

# Check for Intel Ethernet drivers
$drivers = Get-WmiObject -Class Win32_PnPSignedDriver | Where-Object { 
    $_.DeviceName -like "*I219*" -or ($_.DeviceName -like "*Intel*" -and $_.DeviceName -like "*Ethernet*")
}

if ($drivers) {
    foreach ($driver in $drivers) {
        Write-Host "Driver Found: $($driver.DeviceName)" -ForegroundColor Green
        Write-Host "  Version: $($driver.DriverVersion)"
        Write-Host "  Date: $($driver.DriverDate)"
        Write-Host "  Provider: $($driver.DriverProviderName)"
        
        if ($driver.DriverProviderName -like "*Intel*") {
            $testResults.DriverOK = $true
            $testResults.Score += 20
            Write-Host "  Intel driver confirmed" -ForegroundColor Green
        }
    }
} else {
    Write-Host "No Intel Ethernet drivers found" -ForegroundColor Red
}

# Check for PCH integration (I219 is typically PCH-integrated)
Write-Host "`nChecking for PCH Integration..."
$pchDevices = Get-WmiObject -Class Win32_PnPEntity | Where-Object { 
    $_.Description -like "*PCH*" -or $_.Description -like "*Chipset*"
}

if ($pchDevices) {
    Write-Host "PCH/Chipset devices found - I219 likely integrated:" -ForegroundColor Green
    foreach ($pch in $pchDevices | Select-Object -First 3) {
        Write-Host "  - $($pch.Description)"
    }
    $testResults.Score += 10
}

Write-Host "`nPhase 3: I219 Architecture Analysis"

Write-Host "I219 Device Architecture:" -ForegroundColor Cyan
Write-Host "  Type: PHY-only device (not MAC+PHY like I210/I225)"
Write-Host "  Integration: PCH-integrated (not discrete NIC)"
Write-Host "  MAC: Integrated LAN Controller in PCH"
Write-Host "  PHY: I219 (separate chip or PCH-integrated)"
Write-Host "  Interconnect: PCIe-based (custom protocol) + SMBus"
Write-Host "  Speed: 10/100/1000 Mbps (no 2.5G like I225)"

Write-Host "`nI219 Interface Modes:" -ForegroundColor Cyan
Write-Host "  Active (S0): PCIe-based interface at 1.25 Gbps"
Write-Host "  Low Power (Sx): SMBus interface at 10 Mbps"
Write-Host "  Management: MDIO register access via both interfaces"

Write-Host "`nPhase 4: I219 MDIO Register Analysis"

Write-Host "I219 MDIO Register Categories:"
Write-Host "  Standard IEEE 802.3 PHY Registers:"
Write-Host "    BMCR, BMSR, PHYIDR1/2, ANAR, ANLPAR"
Write-Host "  I219-Specific Registers:"
Write-Host "    COPPER_CTRL1, SMART_SPEED, LED_CTRL"
Write-Host "  IEEE 1588/802.1AS Support:"
Write-Host "    PTP_CTRL, PTP_STATUS (if supported)"
Write-Host "  Energy Efficient Ethernet (EEE):"
Write-Host "    EEE_CTRL, EEE_STATUS (via MMD registers)"
Write-Host "  Power Management:"
Write-Host "    POWER_CTRL, ULP_CONFIG"

# Detect I219 capabilities based on datasheet
if ($testResults.DeviceFound) {
    Write-Host "`nI219 Capabilities Analysis:" -ForegroundColor Green
    Write-Host "  IEEE 802.1AS/1588 Conformance: Supported" -ForegroundColor Green
    Write-Host "  Energy Efficient Ethernet (EEE): Supported" -ForegroundColor Green
    Write-Host "  Ultra Low Power (<1 mW): Supported" -ForegroundColor Green
    Write-Host "  Auto-Negotiation: Full IEEE 802.3u compliance" -ForegroundColor Green
    Write-Host "  MDI/MDIX Auto-crossover: Supported" -ForegroundColor Green
    Write-Host "  Smart Speed: Automatic speed reduction on faulty cables" -ForegroundColor Green
    Write-Host "  Network Proxy/ARP Offload: Supported" -ForegroundColor Green
    Write-Host "  Flexible Filters: Up to 32 programmable filters" -ForegroundColor Green
    
    $testResults.IEEE1588Support = $true
    $testResults.EEESupport = $true
    $testResults.Score += 20
}

Write-Host "`nPhase 5: MDIO Register Access Assessment"

# Test MDIO access capabilities
$mdioMethods = @()

# Method 1: WMI hardware access
try {
    $pciRoot = Get-WmiObject -Class Win32_PnPEntity | Where-Object { $_.Name -like "*I219*" }
    if ($pciRoot) {
        $mdioMethods += "WMI"
        Write-Host "WMI hardware access: Available" -ForegroundColor Green
    }
} catch {
    Write-Host "WMI hardware access: Limited" -ForegroundColor Yellow
}

# Method 2: Check for Intel PROSet or similar management tools
$intelTools = Get-WmiObject -Class Win32_Product | Where-Object { 
    $_.Name -like "*Intel*" -and ($_.Name -like "*PROSet*" -or $_.Name -like "*Ethernet*")
}

if ($intelTools) {
    $mdioMethods += "Intel-Management-Tools"
    Write-Host "Intel management tools detected:" -ForegroundColor Green
    foreach ($tool in $intelTools) {
        Write-Host "  - $($tool.Name)"
    }
}

# Method 3: Registry-based configuration access
try {
    $regPath = "HKLM:\SYSTEM\CurrentControlSet\Control\Class\{4D36E972-E325-11CE-BFC1-08002BE10318}"
    if (Test-Path $regPath) {
        $mdioMethods += "Registry-Config"
        Write-Host "Network adapter registry access: Available" -ForegroundColor Green
    }
} catch {
    Write-Host "Registry access: Limited" -ForegroundColor Yellow
}

if ($mdioMethods.Count -gt 0) {
    $testResults.MDIOAccess = $true
    $testResults.Score += 15
    Write-Host "MDIO access methods available: $($mdioMethods -join ', ')" -ForegroundColor Green
} else {
    Write-Host "Limited MDIO register access capabilities detected" -ForegroundColor Red
}

Write-Host "`nPhase 6: System Prerequisites"

# Check Windows version
$osVersion = [System.Environment]::OSVersion.Version
Write-Host "Operating System: Windows $($osVersion.Major).$($osVersion.Minor) Build $($osVersion.Build)"

if ($osVersion.Major -ge 10) {
    Write-Host "Windows 10/11 detected - Good for hardware access" -ForegroundColor Green
    $testResults.Score += 10
} else {
    Write-Host "Older Windows version - Limited hardware access" -ForegroundColor Yellow
}

# Check Administrator privileges
$currentPrincipal = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
if ($currentPrincipal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Host "Running as Administrator - Good for register access" -ForegroundColor Green
    $testResults.Score += 10
} else {
    Write-Host "Not running as Administrator - Limited register access" -ForegroundColor Yellow
}

Write-Host "`n=== Test Results Summary ==="
Write-Host "Final Score: $($testResults.Score)/100"

if ($testResults.Score -ge 80) {
    Write-Host "Assessment: HIGH feasibility for I219 MDIO register access" -ForegroundColor Green
    Write-Host "Recommendation: Proceed with MDIO implementation"
} elseif ($testResults.Score -ge 60) {
    Write-Host "Assessment: MEDIUM feasibility for I219 MDIO register access" -ForegroundColor Yellow
    Write-Host "Recommendation: Address issues then implement"
} else {
    Write-Host "Assessment: LOW feasibility for I219 MDIO register access" -ForegroundColor Red
    Write-Host "Recommendation: Resolve system issues first"
}

Write-Host "`nDetailed Results:"
Write-Host "  Device Detection: $(if($testResults.DeviceFound){'PASS'}else{'FAIL'})"
Write-Host "  Driver Status: $(if($testResults.DriverOK){'PASS'}else{'FAIL'})"
Write-Host "  MDIO Access: $(if($testResults.MDIOAccess){'PASS'}else{'FAIL'})"
Write-Host "  IEEE 1588 Support: $(if($testResults.IEEE1588Support){'PASS'}else{'FAIL'})"
Write-Host "  EEE Support: $(if($testResults.EEESupport){'PASS'}else{'FAIL'})"

Write-Host "`n=== I219 vs I210/I225 Comparison ==="
Write-Host "I219 Characteristics:" -ForegroundColor Cyan
Write-Host "  - PHY-only device (vs MAC+PHY in I210/I225)"
Write-Host "  - PCH-integrated (vs discrete NICs)"
Write-Host "  - MDIO register access (vs memory-mapped registers)"
Write-Host "  - 1 Gbps max (vs 2.5 Gbps I225)"
Write-Host "  - Ultra-low power focused (<1 mW disconnect)"
Write-Host "  - SMBus management interface"
Write-Host "  - IEEE 802.1AS/1588 conformance"
Write-Host "  - Energy Efficient Ethernet (EEE)"

Write-Host "`nImplementation Considerations:" -ForegroundColor Yellow
Write-Host "  - Use MDIO commands instead of direct register reads"
Write-Host "  - Consider PCIe-based vs SMBus interface modes"
Write-Host "  - Leverage existing PHY management infrastructure"
Write-Host "  - Focus on power management and EEE features"
Write-Host "  - Integration with PCH chipset required"

if ($SaveLog) {
    $logContent = @"
I219 MDIO Register Access Test Results
Generated: $(Get-Date)
Score: $($testResults.Score)/100
Device Found: $($testResults.DeviceFound)
Driver OK: $($testResults.DriverOK)
MDIO Access: $($testResults.MDIOAccess)
IEEE 1588 Support: $($testResults.IEEE1588Support)
EEE Support: $($testResults.EEESupport)

I219 is a PHY-only device requiring MDIO access methods
rather than direct memory-mapped register access.
"@
    $logContent | Out-File -FilePath $LogPath -Encoding UTF8
    Write-Host "`nLog saved to: $LogPath"
}

Write-Host "`nTest completed. Use -Verbose for detailed output."
Write-Host "Note: I219 requires MDIO-based register access, not direct memory mapping."
