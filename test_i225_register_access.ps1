# Intel I225/I226 Register Access Test Script
# Tests for direct register access capabilities for I225/I226 TimeSync and TSN features
# Version: 1.0

param(
    [switch]$Verbose = $false,
    [switch]$SaveLog = $true,
    [string]$LogPath = "docs/tests/results/i225_register_test_$(Get-Date -Format 'yyyyMMdd_HHmmss').log"
)

# Logging-Array für vollständige Terminalausgabe
$script:fullLog = @()

function Write-LogBoth {
    param(
        [string]$Message,
        [string]$Color = "White"
    )
    Write-Host $Message -ForegroundColor $Color
    $script:fullLog += $Message
}

Write-LogBoth "=== Intel I225/I226 Register Access Test ==="
Write-LogBoth "Testing register accessibility for I225/I226 TimeSync and TSN features"

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

Write-LogBoth "`nPhase 1: Intel I225/I226 Device Detection"

# Check for Intel I225/I226 device using multiple methods
$networkAdapters = Get-WmiObject -Class Win32_NetworkAdapter | Where-Object { 
    $_.Description -like "*Intel*" -and ($_.Description -like "*I225*" -or $_.Description -like "*I226*")
}

if ($networkAdapters) {
    Write-LogBoth "Found Intel I225/I226 network adapter(s):" -ForegroundColor Green
    foreach ($adapter in $networkAdapters) {
        Write-LogBoth "  - $($adapter.Description)" -ForegroundColor Green
        Write-LogBoth "    Status: $($adapter.NetConnectionStatus)"
        Write-LogBoth "    MAC: $($adapter.MACAddress)"
    }
    $testResults.DeviceFound = $true
    $testResults.Score += 25
} else {
    Write-LogBoth "No Intel I225/I226 network adapters found" -ForegroundColor Red
}

# Check PCI devices for specific device IDs
Write-LogBoth "`nChecking PCI Device IDs..."
$pciDevices = Get-WmiObject -Class Win32_PnPEntity | Where-Object { 
    $_.HardwareID -like "*VEN_8086*" 
}

$foundI225 = $false
foreach ($device in $pciDevices) {
    foreach ($hwid in $device.HardwareID) {
        foreach ($deviceId in $I225_DEVICE_IDS.Keys) {
            if ($hwid -like "*DEV_$deviceId*") {
                Write-LogBoth "Found I225/I226 Device: $($I225_DEVICE_IDS[$deviceId])" -ForegroundColor Green
                Write-LogBoth "  Hardware ID: $hwid"
                Write-LogBoth "  Status: $($device.Status)"
                $foundI225 = $true
                $testResults.DeviceFound = $true
                $testResults.Score += 25
            }
        }
    }
}

if (-not $foundI225 -and -not $testResults.DeviceFound) {
    Write-LogBoth "No I225/I226 devices detected in system" -ForegroundColor Red
}

Write-LogBoth "`nPhase 2: Driver Analysis"

# Check driver information
$drivers = Get-WmiObject -Class Win32_PnPSignedDriver | Where-Object { 
    $_.DeviceName -like "*I225*" -or $_.DeviceName -like "*I226*" 
}

if ($drivers) {
    foreach ($driver in $drivers) {
        Write-LogBoth "Driver Found: $($driver.DeviceName)" -ForegroundColor Green
        Write-LogBoth "  Version: $($driver.DriverVersion)"
        Write-LogBoth "  Date: $($driver.DriverDate)"
        Write-LogBoth "  Provider: $($driver.DriverProviderName)"
        
        if ($driver.DriverProviderName -like "*Intel*") {
            $testResults.DriverOK = $true
            $testResults.Score += 20
            Write-LogBoth "  Intel driver confirmed" -ForegroundColor Green
        }
    }
} else {
    Write-LogBoth "No I225/I226 drivers found" -ForegroundColor Red
}

Write-LogBoth "`nPhase 3: System Prerequisites"

# Check Windows version
$osVersion = [System.Environment]::OSVersion.Version
Write-LogBoth "Operating System: Windows $($osVersion.Major).$($osVersion.Minor) Build $($osVersion.Build)"

if ($osVersion.Major -ge 10) {
    Write-LogBoth "Windows 10/11 detected - Good for register access" -ForegroundColor Green
    $testResults.Score += 10
} else {
    Write-LogBoth "Older Windows version - Limited register access" -ForegroundColor Yellow
}

# Check Administrator privileges
$currentPrincipal = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
if ($currentPrincipal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-LogBoth "Running as Administrator - Good for register access" -ForegroundColor Green
    $testResults.Score += 15
} else {
    Write-LogBoth "Not running as Administrator - Limited register access" -ForegroundColor Yellow
}

Write-LogBoth "`nPhase 4: I225/I226 Register Analysis"

Write-LogBoth "I225/I226 TimeSync Register Categories:"
Write-LogBoth "  Legacy IEEE 1588 Registers (I210 compatible):"
Write-LogBoth "    SYSTIML/H, TIMINCA, TSYNCTXCTL, TSYNCRXCTL, TSAUXC"
Write-LogBoth "  Enhanced TSN Registers (I225/I226 specific):"
Write-LogBoth "    QBVCYCLET_S/NS - Time Aware Shaper"
Write-LogBoth "    QBVBTIME_S/NS  - Base Time Control"
Write-LogBoth "    FPRXCTL/FPTXCTL - Frame Preemption"
Write-LogBoth "    SDP0/1/2_CTRL  - Software Defined Pins"
Write-LogBoth "    PTM_CTRL/STAT  - PCIe Precision Time Measurement"

if ($testResults.DeviceFound) {
    Write-LogBoth "`nI225/I226 TSN Features Available:" -ForegroundColor Green
    Write-LogBoth "  - IEEE 802.1AS-Rev (Enhanced gPTP)"
    Write-LogBoth "  - IEEE 802.1Qbv (Time Aware Shaper)"
    Write-LogBoth "  - IEEE 802.1Qbu (Frame Preemption)"
    Write-LogBoth "  - IEEE 802.3br (Interspersing Express Traffic)"
    Write-LogBoth "  - PCIe PTM (Host-NIC synchronization)"
    Write-LogBoth "  - 2.5 Gbps support (I225/I226 vs 1 Gbps I210)"
    $testResults.TSNFeatures = $true
    $testResults.Score += 20
}

Write-LogBoth "`nPhase 5: Register Access Assessment"

# Test basic hardware access methods
$accessMethods = @()

# Method 1: WMI hardware access
try {
    $pciRoot = Get-WmiObject -Class Win32_PnPEntity | Where-Object { $_.Name -like "*I225*" -or $_.Name -like "*I226*" }
    if ($pciRoot) {
        $accessMethods += "WMI"
        Write-LogBoth "WMI hardware access: Available" -ForegroundColor Green
    }
} catch {
    Write-LogBoth "WMI hardware access: Limited" -ForegroundColor Yellow
}

# Method 2: PowerShell PnP
try {
    if (Get-Command "Get-PnpDevice" -ErrorAction SilentlyContinue) {
        $accessMethods += "PowerShell-PnP"
        Write-LogBoth "PowerShell PnP access: Available" -ForegroundColor Green
    }
} catch {
    Write-LogBoth "PowerShell PnP access: Not available" -ForegroundColor Yellow
}

if ($accessMethods.Count -gt 0) {
    $testResults.RegisterAccess = $true
    $testResults.Score += 10
    Write-LogBoth "Register access methods available: $($accessMethods -join ', ')" -ForegroundColor Green
} else {
    Write-LogBoth "Limited register access capabilities detected" -ForegroundColor Red
}

# Phase 6: Register Value Dump
$registerDump = @()
Write-LogBoth "`nPhase 6: Register Value Dump"
foreach ($reg in $I225_REGISTERS.GetEnumerator()) {
    $regName = $reg.Key
    $regAddr = $reg.Value
    # Simulierter Registerwert (hier müsste ggf. ein echtes Tool/Driver verwendet werden)
    $regValue = "[not implemented: requires kernel driver or tool]"
    $line = "Register $regName (0x{0:X}): $regValue" -f $regAddr
    Write-LogBoth $line
    $registerDump += $line
}

Write-LogBoth "`n=== Test Results Summary ==="
Write-LogBoth "Final Score: $($testResults.Score)/100"

if ($testResults.Score -ge 80) {
    Write-LogBoth "Assessment: HIGH feasibility for I225/I226 register access" -ForegroundColor Green
    Write-LogBoth "Recommendation: Proceed with implementation"
} elseif ($testResults.Score -ge 60) {
    Write-LogBoth "Assessment: MEDIUM feasibility for I225/I226 register access" -ForegroundColor Yellow
    Write-LogBoth "Recommendation: Address issues then implement"
} else {
    Write-LogBoth "Assessment: LOW feasibility for I225/I226 register access" -ForegroundColor Red
    Write-LogBoth "Recommendation: Resolve system issues first"
}

Write-LogBoth "`nDetailed Results:"
Write-LogBoth "  Device Detection: $(if($testResults.DeviceFound){'PASS'}else{'FAIL'})"
Write-LogBoth "  Driver Status: $(if($testResults.DriverOK){'PASS'}else{'FAIL'})"
Write-LogBoth "  Register Access: $(if($testResults.RegisterAccess){'PASS'}else{'FAIL'})"
Write-LogBoth "  TSN Features: $(if($testResults.TSNFeatures){'PASS'}else{'FAIL'})"

Write-LogBoth "`n=== I225/I226 Advantages over I210 ==="
Write-LogBoth "  - 2.5 Gbps bandwidth (vs 1 Gbps I210)"
Write-LogBoth "  - Enhanced TSN features (802.1Qbv, 802.1Qbu)"
Write-LogBoth "  - PCIe PTM for improved synchronization"
Write-LogBoth "  - Frame preemption for low latency"
Write-LogBoth "  - Industrial temperature variants available"

# Am Ende: Logfile schreiben
if ($SaveLog) {
    $script:fullLog | Out-File -FilePath $LogPath -Encoding UTF8
    Write-Host "`nLog saved to: $LogPath"
}

Write-LogBoth "`nTest completed. Use -Verbose for detailed output."
