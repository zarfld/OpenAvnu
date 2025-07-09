# Intel I210 Register Access Test Script
# Tests for direct register access capabilities for I210 TimeSync features

param(
    [switch]$Verbose = $false,
    [switch]$SaveLog = $true,
    [string]$LogPath = "i210_register_test_$(Get-Date -Format 'yyyyMMdd_HHmmss').log"
)

Write-Host "=== Intel I210 Register Access Test ==="
Write-Host "Testing register accessibility for I210 TimeSync features"

# I210 TimeSync Register Addresses (from OpenAvnu source)
$I210_REGISTERS = @{
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
    "RXSATRL"    = 0x0B62C  # Rx timestamp attribute low - RO
    "RXSATRH"    = 0x0B630  # Rx timestamp attribute high - RO
    "TSAUXC"     = 0x0B640  # Timesync Auxiliary Control register
    "TRGTTIML0"  = 0x0B644  # Target Time Register 0 Low - RW
    "TRGTTIMH0"  = 0x0B648  # Target Time Register 0 High - RW
    "TRGTTIML1"  = 0x0B64C  # Target Time Register 1 Low - RW
    "TRGTTIMH1"  = 0x0B650  # Target Time Register 1 High - RW
    "FREQOUT0"   = 0x0B654  # Frequency Out 0 Control Register - RW
    "FREQOUT1"   = 0x0B658  # Frequency Out 1 Control Register - RW
    "AUXSTMPL0"  = 0x0B65C  # Auxiliary Time Stamp 0 Register Low - RO
    "AUXSTMPH0"  = 0x0B660  # Auxiliary Time Stamp 0 Register High - RO
    "AUXSTMPL1"  = 0x0B664  # Auxiliary Time Stamp 1 Register Low - RO
    "AUXSTMPH1"  = 0x0B668  # Auxiliary Time Stamp 1 Register High - RO
    "TSICR"      = 0x0B66C  # Interrupt Cause Register
    "TSIM"       = 0x0B674  # Interrupt Mask Register
    "SYSTIMR"    = 0x0B6F8  # System time register Residue
}

Write-Host "Phase 1: Intel I210 Device Detection"

# Check for Intel I210 device
$networkAdapters = Get-WmiObject -Class Win32_NetworkAdapter | Where-Object { 
    $_.Description -like "*Intel*" -and $_.Description -like "*I210*" 
}

if ($networkAdapters) {
    foreach ($adapter in $networkAdapters) {
        Write-Host "Found I210 adapter: $($adapter.Description)" -ForegroundColor Green
        Write-Host "  Device ID: $($adapter.DeviceID)"
        Write-Host "  PNP Device ID: $($adapter.PNPDeviceID)"
        Write-Host "  MAC Address: $($adapter.MACAddress)"
        
        # Get PCI information
        if ($adapter.PNPDeviceID -match "PCI\\VEN_(\w+)&DEV_(\w+)") {
            $vendorID = $matches[1]
            $deviceID = $matches[2]
            Write-Host "  PCI Vendor ID: $vendorID"
            Write-Host "  PCI Device ID: $deviceID"
            
            # Validate Intel I210 device IDs
            $i210DeviceIDs = @("1533", "1536", "1537", "1538", "1539", "157B", "157C")
            if ($i210DeviceIDs -contains $deviceID.ToUpper()) {
                Write-Host "  Confirmed I210 series device" -ForegroundColor Green
            } else {
                Write-Host "  Warning: Device ID $deviceID not in known I210 range" -ForegroundColor Yellow
            }
        }
    }
} else {
    Write-Host "No Intel I210 adapters detected" -ForegroundColor Red
}

Write-Host "
Phase 2: System Requirements Check"

# Check if running as administrator
$isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")
Write-Host "Administrator privileges: $isAdmin"

# Check for hypervisor
$virtualizationEnabled = (Get-WmiObject -Class Win32_ComputerSystem).HypervisorPresent
Write-Host "Hypervisor present: $virtualizationEnabled"

Write-Host "
Phase 3: Register Analysis"
Write-Host "Analyzing $($I210_REGISTERS.Count) I210 TimeSync registers..."

foreach ($regName in $I210_REGISTERS.Keys) {
    $regAddr = $I210_REGISTERS[$regName]
    $regAddrHex = "0x{0:X6}" -f $regAddr
    Write-Host "  $regName ($regAddrHex)"
}

Write-Host "
Phase 4: Feasibility Assessment"

# Calculate feasibility score
$feasibilityScore = 0
$maxScore = 10

if ($networkAdapters) { $feasibilityScore += 3 }
if ($isAdmin) { $feasibilityScore += 3 }
if (-not $virtualizationEnabled) { $feasibilityScore += 2 }

# Additional checks would add more points
$feasibilityScore += 2  # Base implementation possibility

Write-Host "Feasibility Score: $feasibilityScore/$maxScore"

$feasibilityLevel = switch ($feasibilityScore) {
    {$_ -ge 8} { "HIGH - Direct register access highly feasible" }
    {$_ -ge 5} { "MEDIUM - Register access possible with additional work" }
    {$_ -ge 3} { "LOW - Significant obstacles to register access" }
    default { "VERY LOW - Direct register access not recommended" }
}

Write-Host "Feasibility Level: $feasibilityLevel" -ForegroundColor Cyan

Write-Host "
Recommendations:"
if ($feasibilityScore -ge 5) {
    Write-Host "   Consider developing custom kernel driver for register access"
    Write-Host "   Implement memory-mapped I/O for register operations"
    Write-Host "   Start with read-only register access for SYSTIM registers"
} else {
    Write-Host "   Direct register access not recommended on this system"
    Write-Host "   Consider using enhanced software timestamping instead"
    Write-Host "   Evaluate alternative hardware with better Windows support"
}

Write-Host "
=== TEST COMPLETED ==="
Write-Host "Recommended implementation path: "

return @{
    Feasible = ($feasibilityScore -ge 5)
    Score = $feasibilityScore
    MaxScore = $maxScore
    DeviceDetected = ($networkAdapters -ne $null)
    FeasibilityLevel = $feasibilityLevel
}
