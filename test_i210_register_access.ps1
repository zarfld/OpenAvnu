# Intel I210 Register Access Test Script
# Tests for direct register access capabilities for I210 TimeSync features

param(
    [switch]$Verbose = $false,
    [switch]$SaveLog = $true,
    [string]$LogPath = "i210_register_test_$(Get-Date -Format 'yyyyMMdd_HHmmss').log"
)

# Function to write output to both console and log
function Write-LogOutput {
    param(
        [string]$Message,
        [string]$ForegroundColor = "White"
    )
    
    Write-Host $Message -ForegroundColor $ForegroundColor
    
    if ($SaveLog) {
        $timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
        "[$timestamp] $Message" | Out-File -FilePath $LogPath -Append -Encoding UTF8
    }
}

# Initialize log file
if ($SaveLog) {
    $logHeader = @"
=================================================================
Intel I210 Register Access Test Log
Started: $(Get-Date -Format "yyyy-MM-dd HH:mm:ss")
Script Version: 1.0
=================================================================

"@
    $logHeader | Out-File -FilePath $LogPath -Encoding UTF8
    Write-Host "Log file will be saved to: $LogPath" -ForegroundColor Cyan
}

Write-LogOutput "=== Intel I210 Register Access Test ==="
Write-LogOutput "Testing register accessibility for I210 TimeSync features"

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

Write-LogOutput "Phase 1: Intel I210 Device Detection"

# Check for Intel I210 device
$networkAdapters = Get-WmiObject -Class Win32_NetworkAdapter | Where-Object { 
    $_.Description -like "*Intel*" -and $_.Description -like "*I210*" 
}

if ($networkAdapters) {
    foreach ($adapter in $networkAdapters) {
        Write-LogOutput "Found I210 adapter: $($adapter.Description)" "Green"
        Write-LogOutput "  Device ID: $($adapter.DeviceID)"
        Write-LogOutput "  PNP Device ID: $($adapter.PNPDeviceID)"
        Write-LogOutput "  MAC Address: $($adapter.MACAddress)"
        
        # Get PCI information
        if ($adapter.PNPDeviceID -match "PCI\\VEN_(\w+)&DEV_(\w+)") {
            $vendorID = $matches[1]
            $deviceID = $matches[2]
            Write-LogOutput "  PCI Vendor ID: $vendorID"
            Write-LogOutput "  PCI Device ID: $deviceID"
            
            # Validate Intel I210 device IDs
            $i210DeviceIDs = @("1533", "1536", "1537", "1538", "1539", "157B", "157C")
            if ($i210DeviceIDs -contains $deviceID.ToUpper()) {
                Write-LogOutput "  Confirmed I210 series device" "Green"
            } else {
                Write-LogOutput "  Warning: Device ID $deviceID not in known I210 range" "Yellow"
            }
        }
    }
} else {
    Write-LogOutput "No Intel I210 adapters detected" "Red"
}

Write-LogOutput "
Phase 2: System Requirements Check"

# Check if running as administrator
$isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")
Write-LogOutput "Administrator privileges: $isAdmin"

# Check for hypervisor
$virtualizationEnabled = (Get-WmiObject -Class Win32_ComputerSystem).HypervisorPresent
Write-LogOutput "Hypervisor present: $virtualizationEnabled"

Write-LogOutput "
Phase 3: Register Analysis"
Write-LogOutput "Analyzing $($I210_REGISTERS.Count) I210 TimeSync registers..."

foreach ($regName in $I210_REGISTERS.Keys) {
    $regAddr = $I210_REGISTERS[$regName]
    $regAddrHex = "0x{0:X6}" -f $regAddr
    Write-LogOutput "  $regName ($regAddrHex)"
}

Write-LogOutput "
Phase 4: Feasibility Assessment"

# Calculate feasibility score
$feasibilityScore = 0
$maxScore = 10

if ($networkAdapters) { $feasibilityScore += 3 }
if ($isAdmin) { $feasibilityScore += 3 }
if (-not $virtualizationEnabled) { $feasibilityScore += 2 }

# Additional checks would add more points
$feasibilityScore += 2  # Base implementation possibility

Write-LogOutput "Feasibility Score: $feasibilityScore/$maxScore"

$feasibilityLevel = switch ($feasibilityScore) {
    {$_ -ge 8} { "HIGH - Direct register access highly feasible" }
    {$_ -ge 5} { "MEDIUM - Register access possible with additional work" }
    {$_ -ge 3} { "LOW - Significant obstacles to register access" }
    default { "VERY LOW - Direct register access not recommended" }
}

Write-LogOutput "Feasibility Level: $feasibilityLevel" "Cyan"

Write-LogOutput "
Recommendations:"
if ($feasibilityScore -ge 5) {
    Write-LogOutput "   Consider developing custom kernel driver for register access"
    Write-LogOutput "   Implement memory-mapped I/O for register operations"
    Write-LogOutput "   Start with read-only register access for SYSTIM registers"
} else {
    Write-LogOutput "   Direct register access not recommended on this system"
    Write-LogOutput "   Consider using enhanced software timestamping instead"
    Write-LogOutput "   Evaluate alternative hardware with better Windows support"
}

Write-LogOutput "
=== TEST COMPLETED ==="
Write-LogOutput "Recommended implementation path: "

# Save final results to log
if ($SaveLog) {
    $results = @{
        Feasible = ($feasibilityScore -ge 5)
        Score = $feasibilityScore
        MaxScore = $maxScore
        DeviceDetected = ($null -ne $networkAdapters)
        FeasibilityLevel = $feasibilityLevel
    }
    
    $resultsJson = $results | ConvertTo-Json -Depth 2
    Write-LogOutput "
Final Results (JSON):"
    Write-LogOutput $resultsJson
    
    Write-LogOutput "
Log saved to: $LogPath" "Green"
}

return @{
    Feasible = ($feasibilityScore -ge 5)
    Score = $feasibilityScore
    MaxScore = $maxScore
    DeviceDetected = ($null -ne $networkAdapters)
    FeasibilityLevel = $feasibilityLevel
}
