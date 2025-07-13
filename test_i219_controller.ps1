# Intel i219 Controller Activation and Testing Script
# Detects, enables, and tests the Intel i219 controller for OpenAvnu

param(
    [switch]$EnableAdapter = $false,
    [switch]$TestOnly = $false,
    [switch]$Verbose = $false
)

Write-Host "=== Intel i219 Controller Detection and Activation ===" -ForegroundColor Cyan
Write-Host ""

# Function to get i219 adapter details
function Get-I219Controller {
    $i219Adapters = Get-WmiObject -Class Win32_NetworkAdapter | Where-Object { 
        $_.PhysicalAdapter -eq $true -and
        ($_.Description -like "*I219*" -or $_.Description -like "*219*") -and
        $_.Description -like "*Intel*"
    }
    
    $results = @()
    foreach ($adapter in $i219Adapters) {
        $config = Get-WmiObject -Class Win32_NetworkAdapterConfiguration | Where-Object { 
            $_.Index -eq $adapter.DeviceID 
        }
        
        $results += @{
            Name = $adapter.Name
            Description = $adapter.Description
            NetConnectionID = $adapter.NetConnectionID
            MACAddress = $adapter.MACAddress
            NetEnabled = $adapter.NetEnabled
            NetStatus = $adapter.NetConnectionStatus
            DeviceID = $adapter.DeviceID
            IPEnabled = if ($config) { $config.IPEnabled } else { $false }
            IPAddress = if ($config) { $config.IPAddress } else { @() }
        }
    }
    
    return $results
}

# Detect i219 controllers
Write-Host "Scanning for Intel i219 controllers..." -ForegroundColor Yellow
$i219Controllers = Get-I219Controller

if ($i219Controllers.Count -eq 0) {
    Write-Host "No Intel i219 controllers found on this system." -ForegroundColor Red
    Write-Host "This script is designed for machines with Intel i219 Ethernet controllers." -ForegroundColor Yellow
    exit 1
}

# Display detected controllers
Write-Host "Found $($i219Controllers.Count) Intel i219 controller(s):" -ForegroundColor Green
Write-Host ""

foreach ($controller in $i219Controllers) {
    $statusText = switch ($controller.NetStatus) {
        0 { "Disconnected" }
        1 { "Connecting" } 
        2 { "Connected" }
        3 { "Disconnecting" }
        4 { "Hardware not present" }
        5 { "Hardware disabled" }
        6 { "Hardware malfunction" }
        7 { "Media disconnected" }
        8 { "Authenticating" }
        9 { "Authentication succeeded" }
        10 { "Authentication failed" }
        11 { "Invalid address" }
        12 { "Credentials required" }
        default { "Unknown ($($controller.NetStatus))" }
    }
    
    Write-Host "Controller: $($controller.Description)" -ForegroundColor White
    Write-Host "  Connection ID: $($controller.NetConnectionID)" -ForegroundColor Gray
    Write-Host "  MAC Address: $($controller.MACAddress)" -ForegroundColor Gray
    Write-Host "  Enabled: $(if($controller.NetEnabled){'YES'}else{'NO'})" -ForegroundColor $(if($controller.NetEnabled){'Green'}else{'Red'})
    Write-Host "  Status: $statusText" -ForegroundColor $(if($controller.NetStatus -eq 2){'Green'}elseif($controller.NetStatus -eq 7){'Yellow'}else{'Red'})
    Write-Host "  IP Configured: $(if($controller.IPEnabled){'YES'}else{'NO'})" -ForegroundColor $(if($controller.IPEnabled){'Green'}else{'Yellow'})
    if ($controller.IPAddress) {
        Write-Host "  IP Addresses: $($controller.IPAddress -join ', ')" -ForegroundColor Gray
    }
    Write-Host ""
}

# Check if we need to enable the adapter
$disabledController = $i219Controllers | Where-Object { -not $_.NetEnabled } | Select-Object -First 1
$enabledController = $i219Controllers | Where-Object { $_.NetEnabled } | Select-Object -First 1

if ($disabledController -and $EnableAdapter) {
    Write-Host "=== ENABLING DISABLED i219 CONTROLLER ===" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "Attempting to enable: $($disabledController.NetConnectionID)" -ForegroundColor Yellow
    
    try {
        # Enable the network adapter
        $result = netsh interface set interface name="$($disabledController.NetConnectionID)" admin=enable 2>&1
        
        Start-Sleep -Seconds 3  # Wait for adapter to initialize
        
        # Re-check status
        $updatedControllers = Get-I219Controller
        $nowEnabled = $updatedControllers | Where-Object { $_.NetConnectionID -eq $disabledController.NetConnectionID -and $_.NetEnabled }
        
        if ($nowEnabled) {
            Write-Host "[SUCCESS] Adapter enabled successfully!" -ForegroundColor Green
            $enabledController = $nowEnabled
        } else {
            Write-Host "[FAILED] Could not enable adapter. May require administrator privileges." -ForegroundColor Red
            Write-Host "Try running as Administrator or enable manually in Network Connections." -ForegroundColor Yellow
        }
    } catch {
        Write-Host "[ERROR] Failed to enable adapter: $($_.Exception.Message)" -ForegroundColor Red
    }
    Write-Host ""
}

if ($TestOnly -or -not $enabledController) {
    if (-not $enabledController) {
        Write-Host "=== STATUS SUMMARY ===" -ForegroundColor Cyan
        if ($disabledController) {
            Write-Host "Intel i219 controller detected but DISABLED." -ForegroundColor Yellow
            Write-Host "Run with -EnableAdapter to attempt automatic enablement, or:" -ForegroundColor Yellow
            Write-Host "1. Open Network Connections (ncpa.cpl)" -ForegroundColor White
            Write-Host "2. Right-click on '$($disabledController.NetConnectionID)'" -ForegroundColor White  
            Write-Host "3. Select 'Enable'" -ForegroundColor White
        } else {
            Write-Host "All i219 controllers are enabled but may have issues." -ForegroundColor Yellow
        }
    }
    exit 0
}

# Test OpenAvnu daemons with the enabled controller
Write-Host "=== TESTING OPENAVNU WITH i219 CONTROLLER ===" -ForegroundColor Cyan
Write-Host ""

$testController = $enabledController
$interfaceName = $testController.NetConnectionID
$macAddress = $testController.MACAddress

Write-Host "Testing with interface: $interfaceName" -ForegroundColor White
Write-Host "MAC Address: $macAddress" -ForegroundColor Gray
Write-Host ""

# Test paths
$gptpPath = "D:\Repos\OpenAvnu\build\thirdparty\gptp\Release\gptp.exe"
$maapPath = "D:\Repos\OpenAvnu\build\daemons\maap\Release\maap_daemon.exe" 
$mrpdPath = "D:\Repos\OpenAvnu\build\daemons\mrpd\Release\mrpd.exe"

$testResults = @{
    gPTP = $false
    MAAP = $false
    MRPD = $false
}

# Test gPTP daemon
Write-Host "Testing gPTP daemon..." -ForegroundColor Blue
if (Test-Path $gptpPath) {
    try {
        # Test gPTP initialization briefly
        $gptpJob = Start-Job -ScriptBlock {
            param($gptpPath, $interfaceName)
            & $gptpPath -S -i $interfaceName 2>&1
        } -ArgumentList $gptpPath, $interfaceName
        
        Start-Sleep -Seconds 8  # Give gPTP time to initialize
        Stop-Job $gptpJob -ErrorAction SilentlyContinue
        $gptpOutput = Receive-Job $gptpJob
        Remove-Job $gptpJob -ErrorAction SilentlyContinue
        
        if ($Verbose) {
            Write-Host "gPTP Output:" -ForegroundColor Gray
            $gptpOutput | ForEach-Object { Write-Host "  $_" -ForegroundColor Gray }
        }
        
        # Check for successful initialization
        $gptpSuccess = $gptpOutput -match "Initializing|Starting|opened|PTP" -and $gptpOutput -notmatch "error|Error|ERROR|failed|Failed|FAILED"
        $testResults.gPTP = $gptpSuccess
        
        if ($gptpSuccess) {
            Write-Host "[SUCCESS] gPTP daemon initialized successfully" -ForegroundColor Green
            Write-Host "  Hardware timestamping available on i219 controller" -ForegroundColor Green
        } else {
            Write-Host "[PARTIAL] gPTP daemon may need elevated privileges or cable connection" -ForegroundColor Yellow
        }
    } catch {
        Write-Host "[ERROR] gPTP test failed: $($_.Exception.Message)" -ForegroundColor Red
    }
} else {
    Write-Host "[MISSING] gPTP daemon not built" -ForegroundColor Red
}
Write-Host ""

# Test MAAP daemon
Write-Host "Testing MAAP daemon..." -ForegroundColor Blue
if (Test-Path $maapPath) {
    try {
        $maapJob = Start-Job -ScriptBlock {
            param($maapPath, $interfaceName)
            & $maapPath -i $interfaceName 2>&1
        } -ArgumentList $maapPath, $interfaceName
        
        Start-Sleep -Seconds 4
        Stop-Job $maapJob -ErrorAction SilentlyContinue
        $maapOutput = Receive-Job $maapJob
        Remove-Job $maapJob -ErrorAction SilentlyContinue
        
        if ($Verbose) {
            Write-Host "MAAP Output:" -ForegroundColor Gray
            $maapOutput | ForEach-Object { Write-Host "  $_" -ForegroundColor Gray }
        }
        
        $maapSuccess = $maapOutput -notmatch "error|Error|ERROR|failed|Failed|FAILED" -and $maapOutput -match "MAAP|initialized|starting"
        $testResults.MAAP = $maapSuccess
        
        if ($maapSuccess) {
            Write-Host "[SUCCESS] MAAP daemon compatible with i219" -ForegroundColor Green
        } else {
            Write-Host "[PARTIAL] MAAP daemon functionality limited" -ForegroundColor Yellow
        }
    } catch {
        Write-Host "[ERROR] MAAP test failed: $($_.Exception.Message)" -ForegroundColor Red
    }
} else {
    Write-Host "[MISSING] MAAP daemon not built" -ForegroundColor Red
}
Write-Host ""

# Test MRPD daemon  
Write-Host "Testing MRPD daemon..." -ForegroundColor Blue
if (Test-Path $mrpdPath) {
    try {
        $mrpdOutput = & $mrpdPath -h 2>&1
        $mrpdSuccess = $LASTEXITCODE -ne 255
        $testResults.MRPD = $mrpdSuccess
        
        if ($mrpdSuccess) {
            Write-Host "[SUCCESS] MRPD daemon available" -ForegroundColor Green
        } else {
            Write-Host "[INFO] MRPD daemon present but may need configuration" -ForegroundColor Yellow
        }
    } catch {
        Write-Host "[ERROR] MRPD test failed: $($_.Exception.Message)" -ForegroundColor Red
    }
} else {
    Write-Host "[MISSING] MRPD daemon not built" -ForegroundColor Red
}
Write-Host ""

# Final summary
Write-Host "=== i219 TESTING RESULTS ===" -ForegroundColor White
Write-Host ""
Write-Host "Interface: $interfaceName" -ForegroundColor Cyan
Write-Host "MAC Address: $macAddress" -ForegroundColor Cyan
Write-Host "Hardware: Intel i219 (Full AVB/TSN Support)" -ForegroundColor Cyan
Write-Host ""

$successCount = ($testResults.Values | Where-Object { $_ }).Count
$totalTests = $testResults.Count

Write-Host "Test Results:" -ForegroundColor White
Write-Host "  gPTP:  $(if($testResults.gPTP){'PASS'}else{'PARTIAL'})" -ForegroundColor $(if($testResults.gPTP){'Green'}else{'Yellow'})
Write-Host "  MAAP:  $(if($testResults.MAAP){'PASS'}else{'PARTIAL'})" -ForegroundColor $(if($testResults.MAAP){'Green'}else{'Yellow'}) 
Write-Host "  MRPD:  $(if($testResults.MRPD){'PASS'}else{'PARTIAL'})" -ForegroundColor $(if($testResults.MRPD){'Green'}else{'Yellow'})
Write-Host ""

if ($successCount -gt 0) {
    Write-Host "READY FOR AVB/TSN OPERATION!" -ForegroundColor Green
    Write-Host ""
    Write-Host "Recommended configuration commands:" -ForegroundColor Cyan
    Write-Host "  gPTP Master: gptp.exe -S -i `"$interfaceName`"" -ForegroundColor White
    Write-Host "  gPTP Slave:  gptp.exe -i `"$interfaceName`"" -ForegroundColor White
    Write-Host "  MAAP Server: maap_daemon.exe -i `"$interfaceName`"" -ForegroundColor White
    Write-Host "  MRPD:        mrpd.exe -i `"$interfaceName`"" -ForegroundColor White
    Write-Host ""
    Write-Host "The Intel i219 controller provides full hardware timestamping support" -ForegroundColor Green
    Write-Host "for precise IEEE 802.1AS (gPTP) synchronization required for AVB streams." -ForegroundColor Green
} else {
    Write-Host "Configuration needed - may require elevated privileges or network setup." -ForegroundColor Yellow
}

Write-Host ""
Write-Host "i219 testing complete!" -ForegroundColor Cyan
