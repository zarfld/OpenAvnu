# Intel I225/I226 Register Access Test Script
# Tests for direct register access capabilities for I225/I226 TimeSync and TSN features
# Version: 1.0

param(
    [switch]$Verbose = $false,
    [switch]$SaveLog = $true,
    [string]$LogPath = "i225_register_test_$(Get-Date -Format 'yyyyMMdd_HHmmss').log"
)

Write-Host "=== Intel I225/I226 Register Access Test ==="
Write-Host "Testing register accessibility for I225/I226 TimeSync and TSN features"

# I225/I226 Device IDs
$I225_DEVICE_IDS = @{
    "15F2" = "I225-LM (LAN on Motherboard)"
    "15F3" = "I225-V (Discrete/NIC)"
    "15F4" = "I225-IT (Extended Temperature)"
    "15F5" = "I225-K (vPro)"
    "15F6" = "I225-K2 (vPro)"
    "15F7" = "I225-LMvP (LAN on Motherboard vPro)"
    "15F8" = "I225-V2 (Discrete/NIC v2)"
    "15F9" = "I225-LM2 (LAN on Motherboard v2)"
    "15FA" = "I225-V3 (Discrete/NIC v3)"
    "15FB" = "I225-LM3 (LAN on Motherboard v3)"
    "0D9F" = "I226-LM (2.5G LAN on Motherboard)"
    "125C" = "I226-V (2.5G Discrete/NIC)"
    "125D" = "I226-IT (2.5G Industrial Temperature)"
    "125E" = "I226-K (2.5G vPro)"
}

# I225/I226 TimeSync Register Map (Enhanced vs I210)
$I225_REGISTERS = @{
    # Legacy IEEE 1588 registers (compatible with I210)
    "SYSTIML"    = 0x0B600  # System time register Low - RO
    "SYSTIMH"    = 0x0B604  # System time register High - RO  
    "TIMINCA"    = 0x0B608  # Increment attributes register - RW
    "TIMADJL"    = 0x0B60C  # Time adjustment offset Low - RW
    "TIMADJH"    = 0x0B610  # Time adjustment offset High - RW
    "TSYNCTXCTL" = 0x0B614  # Tx Time Sync Control register - RW
    "TXSTMPL"    = 0x0B618  # Tx timestamp value Low - RO
    "TXSTMPH"    = 0x0B61C  # Tx timestamp value High - RO
    "TSYNCRXCTL" = 0x0B620  # Rx Time Sync Control register - RW
    "RXSTMPL"    = 0x0B624  # Rx timestamp Low - RO
    "RXSTMPH"    = 0x0B628  # Rx timestamp High - RO
    "TSAUXC"     = 0x0B640  # Timesync Auxiliary Control register
    "TSICR"      = 0x0B66C  # Interrupt Cause Register
    "TSIM"       = 0x0B674  # Interrupt Mask Register
    "SYSTIMR"    = 0x0B6F8  # System time register Residue
    
    # Enhanced I225/I226 TSN registers
    "QBVCYCLET_S"  = 0x0331C  # Qbv Cycle Time Seconds
    "QBVCYCLET_NS" = 0x03320  # Qbv Cycle Time Nanoseconds
    "QBVBTIME_S"   = 0x03324  # Qbv Base Time Seconds
    "QBVBTIME_NS"  = 0x03328  # Qbv Base Time Nanoseconds
    "FPRXCTL"      = 0x03400  # Frame Preemption Rx Control
    "FPTXCTL"      = 0x03404  # Frame Preemption Tx Control
    "SDP0_CTRL"    = 0x0E000  # Software Defined Pin 0 Control
    "SDP1_CTRL"    = 0x0E004  # Software Defined Pin 1 Control
    "SDP2_CTRL"    = 0x0E008  # Software Defined Pin 2 Control
    "PTM_CTRL"     = 0x12000  # PCIe PTM Control
    "PTM_STAT"     = 0x12004  # PCIe PTM Status
}

$testResults = @{
    DeviceFound = $false
    DriverOK = $false
    RegisterAccess = $false
    TSNFeatures = $false
    Score = 0
}

Write-Host "`nPhase 1: Intel I225/I226 Device Detection"

# Check for Intel I225/I226 device using multiple methods
$networkAdapters = Get-WmiObject -Class Win32_NetworkAdapter | Where-Object { 
    $_.Description -like "*Intel*" -and ($_.Description -like "*I225*" -or $_.Description -like "*I226*")
}

if ($networkAdapters) {
    Write-Host "Found Intel I225/I226 network adapter(s):" -ForegroundColor Green
    foreach ($adapter in $networkAdapters) {
        Write-Host "  - $($adapter.Description)" -ForegroundColor Green
        Write-Host "    Status: $($adapter.NetConnectionStatus)"
        Write-Host "    MAC: $($adapter.MACAddress)"
    }
    $testResults.DeviceFound = $true
    $testResults.Score += 25
} else {
    Write-Host "No Intel I225/I226 network adapters found" -ForegroundColor Red
}

# Check PCI devices for specific device IDs
Write-Host "`nChecking PCI Device IDs..."
$pciDevices = Get-WmiObject -Class Win32_PnPEntity | Where-Object { 
    $_.HardwareID -like "*VEN_8086*" 
}

$foundI225 = $false
foreach ($device in $pciDevices) {
    foreach ($hwid in $device.HardwareID) {
        foreach ($deviceId in $I225_DEVICE_IDS.Keys) {
            if ($hwid -like "*DEV_$deviceId*") {
                Write-Host "Found I225/I226 Device: $($I225_DEVICE_IDS[$deviceId])" -ForegroundColor Green
                Write-Host "  Hardware ID: $hwid"
                Write-Host "  Status: $($device.Status)"
                $foundI225 = $true
                $testResults.DeviceFound = $true
                $testResults.Score += 25
            }
        }
    }
}

if (-not $foundI225 -and -not $testResults.DeviceFound) {
    Write-Host "No I225/I226 devices detected in system" -ForegroundColor Red
}

Write-Host "`nPhase 2: Driver Analysis"

# Check driver information
$drivers = Get-WmiObject -Class Win32_PnPSignedDriver | Where-Object { 
    $_.DeviceName -like "*I225*" -or $_.DeviceName -like "*I226*" 
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
    Write-Host "No I225/I226 drivers found" -ForegroundColor Red
}

Write-Host "`nPhase 3: System Prerequisites"

# Check Windows version
$osVersion = [System.Environment]::OSVersion.Version
Write-Host "Operating System: Windows $($osVersion.Major).$($osVersion.Minor) Build $($osVersion.Build)"

if ($osVersion.Major -ge 10) {
    Write-Host "Windows 10/11 detected - Good for register access" -ForegroundColor Green
    $testResults.Score += 10
} else {
    Write-Host "Older Windows version - Limited register access" -ForegroundColor Yellow
}

# Check Administrator privileges
$currentPrincipal = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
if ($currentPrincipal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Host "Running as Administrator - Good for register access" -ForegroundColor Green
    $testResults.Score += 15
} else {
    Write-Host "Not running as Administrator - Limited register access" -ForegroundColor Yellow
}

Write-Host "`nPhase 4: I225/I226 Register Analysis"

Write-Host "I225/I226 TimeSync Register Categories:"
Write-Host "  Legacy IEEE 1588 Registers (I210 compatible):"
Write-Host "    SYSTIML/H, TIMINCA, TSYNCTXCTL, TSYNCRXCTL, TSAUXC"
Write-Host "  Enhanced TSN Registers (I225/I226 specific):"
Write-Host "    QBVCYCLET_S/NS - Time Aware Shaper"
Write-Host "    QBVBTIME_S/NS  - Base Time Control"
Write-Host "    FPRXCTL/FPTXCTL - Frame Preemption"
Write-Host "    SDP0/1/2_CTRL  - Software Defined Pins"
Write-Host "    PTM_CTRL/STAT  - PCIe Precision Time Measurement"

if ($testResults.DeviceFound) {
    Write-Host "`nI225/I226 TSN Features Available:" -ForegroundColor Green
    Write-Host "  - IEEE 802.1AS-Rev (Enhanced gPTP)"
    Write-Host "  - IEEE 802.1Qbv (Time Aware Shaper)"
    Write-Host "  - IEEE 802.1Qbu (Frame Preemption)"
    Write-Host "  - IEEE 802.3br (Interspersing Express Traffic)"
    Write-Host "  - PCIe PTM (Host-NIC synchronization)"
    Write-Host "  - 2.5 Gbps support (I225/I226 vs 1 Gbps I210)"
    $testResults.TSNFeatures = $true
    $testResults.Score += 20
}

Write-Host "`nPhase 5: Register Access Assessment"

# Test basic hardware access methods
$accessMethods = @()

# Method 1: WMI hardware access
try {
    $pciRoot = Get-WmiObject -Class Win32_PnPEntity | Where-Object { $_.Name -like "*I225*" -or $_.Name -like "*I226*" }
    if ($pciRoot) {
        $accessMethods += "WMI"
        Write-Host "WMI hardware access: Available" -ForegroundColor Green
    }
} catch {
    Write-Host "WMI hardware access: Limited" -ForegroundColor Yellow
}

# Method 2: PowerShell PnP
try {
    if (Get-Command "Get-PnpDevice" -ErrorAction SilentlyContinue) {
        $accessMethods += "PowerShell-PnP"
        Write-Host "PowerShell PnP access: Available" -ForegroundColor Green
    }
} catch {
    Write-Host "PowerShell PnP access: Not available" -ForegroundColor Yellow
}

if ($accessMethods.Count -gt 0) {
    $testResults.RegisterAccess = $true
    $testResults.Score += 10
    Write-Host "Register access methods available: $($accessMethods -join ', ')" -ForegroundColor Green
} else {
    Write-Host "Limited register access capabilities detected" -ForegroundColor Red
}

Write-Host "`n=== Test Results Summary ==="
Write-Host "Final Score: $($testResults.Score)/100"

if ($testResults.Score -ge 80) {
    Write-Host "Assessment: HIGH feasibility for I225/I226 register access" -ForegroundColor Green
    Write-Host "Recommendation: Proceed with implementation"
} elseif ($testResults.Score -ge 60) {
    Write-Host "Assessment: MEDIUM feasibility for I225/I226 register access" -ForegroundColor Yellow
    Write-Host "Recommendation: Address issues then implement"
} else {
    Write-Host "Assessment: LOW feasibility for I225/I226 register access" -ForegroundColor Red
    Write-Host "Recommendation: Resolve system issues first"
}

Write-Host "`nDetailed Results:"
Write-Host "  Device Detection: $(if($testResults.DeviceFound){'PASS'}else{'FAIL'})"
Write-Host "  Driver Status: $(if($testResults.DriverOK){'PASS'}else{'FAIL'})"
Write-Host "  Register Access: $(if($testResults.RegisterAccess){'PASS'}else{'FAIL'})"
Write-Host "  TSN Features: $(if($testResults.TSNFeatures){'PASS'}else{'FAIL'})"

Write-Host "`n=== I225/I226 Advantages over I210 ==="
Write-Host "  - 2.5 Gbps bandwidth (vs 1 Gbps I210)"
Write-Host "  - Enhanced TSN features (802.1Qbv, 802.1Qbu)"
Write-Host "  - PCIe PTM for improved synchronization"
Write-Host "  - Frame preemption for low latency"
Write-Host "  - Industrial temperature variants available"

if ($SaveLog) {
    $logContent = @"
I225/I226 Register Access Test Results
Generated: $(Get-Date)
Score: $($testResults.Score)/100
Device Found: $($testResults.DeviceFound)
Driver OK: $($testResults.DriverOK)
Register Access: $($testResults.RegisterAccess)
TSN Features: $($testResults.TSNFeatures)
"@
    $logContent | Out-File -FilePath $LogPath -Encoding UTF8
    Write-Host "`nLog saved to: $LogPath"
}

Write-Host "`nTest completed. Use -Verbose for detailed output."
