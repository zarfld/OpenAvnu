# Diagnose PDelay PCAP Configuration Issues
# Check if PDelay request/response packets are properly captured and transmitted

Write-Host "PDelay PCAP Diagnostics" -ForegroundColor Magenta
Write-Host "========================" -ForegroundColor Magenta
Write-Host ""

# Check current privileges
$isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")
Write-Host "Current Privileges: $(if ($isAdmin) { 'Administrator' } else { 'Standard User' })" -ForegroundColor $(if ($isAdmin) { "Green" } else { "Red" })

# Check PCAP installation
Write-Host ""
Write-Host "=== PCAP Installation Check ===" -ForegroundColor Cyan

$npcapVersions = @()
try {
    $npcapVersions = Get-ItemProperty "HKLM:\SOFTWARE\WOW6432Node\Npcap" -ErrorAction SilentlyContinue
    if ($npcapVersions) {
        Write-Host "Npcap Version: $($npcapVersions.Version)" -ForegroundColor Green
    }
} catch {
    Write-Host "Npcap not found in registry" -ForegroundColor Yellow
}

$winpcapCheck = Test-Path "C:\Windows\System32\wpcap.dll"
if ($winpcapCheck) {
    Write-Host "WinPcap libraries found" -ForegroundColor Yellow
} else {
    Write-Host "WinPcap libraries not found" -ForegroundColor Gray
}

# Check Intel I226 configuration
Write-Host ""
Write-Host "=== Intel I226 Configuration ===" -ForegroundColor Cyan

$i226Adapters = Get-NetAdapter | Where-Object { $_.InterfaceDescription -like "*I226*" }
foreach ($adapter in $i226Adapters) {
    Write-Host "Adapter: $($adapter.InterfaceDescription)" -ForegroundColor White
    Write-Host "  Status: $($adapter.Status)" -ForegroundColor $(if ($adapter.Status -eq "Up") { "Green" } else { "Red" })
    Write-Host "  MAC: $($adapter.MacAddress)" -ForegroundColor Gray
    Write-Host "  Speed: $($adapter.LinkSpeed)" -ForegroundColor Gray
    
    # Check for Intel HAL support
    $adapterPath = "HKLM:\SYSTEM\CurrentControlSet\Control\Class\{4d36e972-e325-11ce-bfc1-08002be10318}"
    $instances = Get-ChildItem $adapterPath -ErrorAction SilentlyContinue
    foreach ($instance in $instances) {
        $props = Get-ItemProperty $instance.PSPath -ErrorAction SilentlyContinue
        if ($props.NetCfgInstanceId -eq $adapter.InterfaceGuid) {
            $ptpSupport = $props.PTPEnable
            Write-Host "  PTP Support: $(if ($ptpSupport) { $ptpSupport } else { 'Not configured' })" -ForegroundColor $(if ($ptpSupport -eq 1) { "Green" } else { "Yellow" })
        }
    }
}

# Analyze recent gPTP logs for PDelay issues
Write-Host ""
Write-Host "=== PDelay Log Analysis ===" -ForegroundColor Cyan

$logFile = ".\thirdparty\gptp\milan_test_output.log"
if (Test-Path $logFile) {
    $logContent = Get-Content $logFile
    
    # Check PDelay transmission attempts
    $pdelayTX = $logContent | Select-String -Pattern "send.*PDelay|TX.*PDelay|transmit.*PDelay" | Measure-Object | Select-Object -ExpandProperty Count
    Write-Host "PDelay TX Attempts: $pdelayTX" -ForegroundColor $(if ($pdelayTX -gt 0) { "Green" } else { "Red" })
    
    # Check PDelay reception
    $pdelayRX = $logContent | Select-String -Pattern "Received.*PDelay|RX.*PDelay|PDelay.*received" | Measure-Object | Select-Object -ExpandProperty Count
    Write-Host "PDelay RX Messages: $pdelayRX" -ForegroundColor $(if ($pdelayRX -gt 0) { "Green" } else { "Red" })
    
    # Check for timestamping errors
    $timestampErrors = $logContent | Select-String -Pattern "Error.*timestamp|error=87|timestamping.*failed" | Measure-Object | Select-Object -ExpandProperty Count
    Write-Host "Timestamping Errors: $timestampErrors" -ForegroundColor $(if ($timestampErrors -eq 0) { "Green" } else { "Red" })
    
    # Check asCapable status
    $asCapableAttempts = $logContent | Select-String -Pattern "asCapable.*startup|Beginning.*PDelay.*exchanges" | Measure-Object | Select-Object -ExpandProperty Count
    $asCapableSuccess = $logContent | Select-String -Pattern "asCapable.*true|asCapable=true" | Measure-Object | Select-Object -ExpandProperty Count
    Write-Host "asCapable Attempts: $asCapableAttempts" -ForegroundColor White
    Write-Host "asCapable Success: $asCapableSuccess" -ForegroundColor $(if ($asCapableSuccess -gt 0) { "Green" } else { "Red" })
    
    # Show recent critical errors
    $criticalErrors = $logContent | Select-String -Pattern "ERROR|CRITICAL|FAILED" | Select-Object -Last 5
    if ($criticalErrors.Count -gt 0) {
        Write-Host ""
        Write-Host "Recent Critical Messages:" -ForegroundColor Red
        foreach ($errorMsg in $criticalErrors) {
            Write-Host "  $($errorMsg.Line)" -ForegroundColor Yellow
        }
    }
} else {
    Write-Host "No gPTP log file found at $logFile" -ForegroundColor Red
}

# Check multicast configuration
Write-Host ""
Write-Host "=== Multicast Configuration ===" -ForegroundColor Cyan

# gPTP uses multicast address 01-1B-19-00-00-00
Write-Host "gPTP Multicast Address: 01-1B-19-00-00-00" -ForegroundColor White

try {
    $multicastStats = netsh interface ipv4 show join | Select-String -Pattern "224.0.0.107|01-1B-19"
    if ($multicastStats) {
        Write-Host "Multicast groups detected" -ForegroundColor Green
    } else {
        Write-Host "No gPTP multicast groups found" -ForegroundColor Yellow
    }
} catch {
    Write-Host "Could not query multicast configuration" -ForegroundColor Yellow
}

# Summary and recommendations
Write-Host ""
Write-Host "=== Summary and Recommendations ===" -ForegroundColor Cyan

if (-not $isAdmin) {
    Write-Host "CRITICAL: Run as Administrator for Intel HAL hardware timestamping" -ForegroundColor Red
    Write-Host "This is likely the primary cause of PDelay transmission failures" -ForegroundColor Yellow
}

if ($i226Adapters.Count -eq 0) {
    Write-Host "ERROR: No Intel I226 adapters found" -ForegroundColor Red
} elseif (($i226Adapters | Where-Object { $_.Status -eq "Up" }).Count -eq 0) {
    Write-Host "ERROR: Intel I226 adapter not active" -ForegroundColor Red
}

Write-Host ""
Write-Host "To resolve PDelay issues:" -ForegroundColor White
Write-Host "1. Restart VS Code as Administrator" -ForegroundColor Gray
Write-Host "2. Ensure Intel I226 driver supports hardware timestamping" -ForegroundColor Gray
Write-Host "3. Verify Npcap installation for packet capture" -ForegroundColor Gray
Write-Host "4. Check network connectivity to Milan devices" -ForegroundColor Gray