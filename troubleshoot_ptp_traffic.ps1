# PTP Traffic Troubleshooting Script
# Helps diagnose why gPTP is not communicating with Milan devices

Write-Host "=== PTP TRAFFIC TROUBLESHOOTING ===" -ForegroundColor Red
Write-Host "Diagnosing gPTP communication issues with Milan devices" -ForegroundColor Yellow
Write-Host ""

# Get the active I226 adapter
$adapter = Get-NetAdapter | Where-Object { 
    $_.InterfaceDescription -like '*I226*' -and $_.Status -eq 'Up' 
} | Select-Object -First 1

if (-not $adapter) {
    Write-Host "ERROR: No active I226 adapter found" -ForegroundColor Red
    exit 1
}

Write-Host "[1] ADAPTER STATUS" -ForegroundColor Cyan
Write-Host "  Adapter: $($adapter.Name) ($($adapter.InterfaceDescription))" -ForegroundColor Green
Write-Host "  MAC: $($adapter.MacAddress)" -ForegroundColor White
Write-Host "  Status: $($adapter.Status)" -ForegroundColor Green
Write-Host "  Link Speed: $($adapter.LinkSpeed)" -ForegroundColor Green

# Get IP configuration
$ipConfig = Get-NetIPAddress -InterfaceIndex $adapter.InterfaceIndex -AddressFamily IPv4 -ErrorAction SilentlyContinue
if ($ipConfig) {
    Write-Host "  IP: $($ipConfig.IPAddress)" -ForegroundColor Green
} else {
    Write-Host "  ERROR: No IP address" -ForegroundColor Red
    exit 1
}

Write-Host ""

# Test Milan device connectivity
Write-Host "[2] MILAN DEVICE CONNECTIVITY" -ForegroundColor Cyan
$milanDevices = @("157.247.3.12", "157.247.1.112")

foreach ($device in $milanDevices) {
    Write-Host "  Testing $device..." -NoNewline -ForegroundColor White
    $ping = Test-Connection -ComputerName $device -Count 3 -Quiet -ErrorAction SilentlyContinue
    if ($ping) {
        Write-Host " REACHABLE" -ForegroundColor Green
        
        # Get detailed ping statistics
        $pingStats = Test-Connection -ComputerName $device -Count 3 -ErrorAction SilentlyContinue
        if ($pingStats) {
            $avgTime = ($pingStats | Measure-Object -Property ResponseTime -Average).Average
            Write-Host "    Average response time: $($avgTime.ToString('F1'))ms" -ForegroundColor Gray
        }
    } else {
        Write-Host " NOT REACHABLE" -ForegroundColor Red
    }
}

Write-Host ""

# Check firewall settings for PTP ports
Write-Host "[3] FIREWALL PTP PORT CHECK" -ForegroundColor Cyan
try {
    $ptpPorts = @(319, 320)
    foreach ($port in $ptpPorts) {
        Write-Host "  Checking UDP port $port..." -NoNewline -ForegroundColor White
        
        # Check if port is listening (unlikely for outbound PTP)
        $listening = Get-NetTCPConnection -LocalPort $port -ErrorAction SilentlyContinue
        $udpEndpoint = Get-NetUDPEndpoint -LocalPort $port -ErrorAction SilentlyContinue
        
        if ($udpEndpoint) {
            Write-Host " LISTENING" -ForegroundColor Green
        } else {
            Write-Host " NOT LISTENING (normal for PTP client)" -ForegroundColor Gray
        }
        
        # Check firewall rules
        $firewallRule = Get-NetFirewallRule -DisplayName "*PTP*" -ErrorAction SilentlyContinue | Where-Object { $_.Enabled -eq $true }
        if ($firewallRule) {
            Write-Host "    PTP firewall rule found: $($firewallRule.DisplayName)" -ForegroundColor Green
        }
    }
} catch {
    Write-Host "  ERROR: Cannot check firewall settings: $($_.Exception.Message)" -ForegroundColor Yellow
}

Write-Host ""

# Check if gPTP process is running
Write-Host "[4] gPTP PROCESS STATUS" -ForegroundColor Cyan
$gptpProcess = Get-Process -Name "gptp" -ErrorAction SilentlyContinue
if ($gptpProcess) {
    Write-Host "  gPTP is running (PID: $($gptpProcess.Id))" -ForegroundColor Green
    Write-Host "  CPU Time: $($gptpProcess.TotalProcessorTime.TotalMilliseconds.ToString('F0'))ms" -ForegroundColor White
    Write-Host "  Memory: $([math]::Round($gptpProcess.WorkingSet64 / 1MB, 1))MB" -ForegroundColor White
} else {
    Write-Host "  gPTP is NOT running" -ForegroundColor Red
}

Write-Host ""

# Suggest network capture
Write-Host "[5] NETWORK TRAFFIC ANALYSIS" -ForegroundColor Cyan
Write-Host "  To diagnose PTP communication issues:" -ForegroundColor Yellow
Write-Host "  1. Start Wireshark on interface: $($adapter.Name)" -ForegroundColor White
Write-Host "  2. Use filter: udp port 319 or udp port 320" -ForegroundColor White
Write-Host "  3. Start gPTP: .\gptp.exe -profile milan $($adapter.MacAddress)" -ForegroundColor White
Write-Host "  4. Look for PTP packets to/from Milan devices" -ForegroundColor White
Write-Host ""

# Check for common issues
Write-Host "[6] COMMON MILAN CONFIGURATION ISSUES" -ForegroundColor Cyan
Write-Host "  Milan devices often need explicit configuration:" -ForegroundColor Yellow
Write-Host "  • Enable AVB/Milan mode in device settings" -ForegroundColor White
Write-Host "  • Set correct sample rate (44.1/48/88.2/96 kHz)" -ForegroundColor White
Write-Host "  • Verify Milan firmware version" -ForegroundColor White
Write-Host "  • Check if device is in 'discovery' or 'Milan' mode" -ForegroundColor White
Write-Host "  • Some devices require manual Milan profile activation" -ForegroundColor White
Write-Host ""

# Administrator check
$currentPrincipal = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
$isAdmin = $currentPrincipal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)

Write-Host "[7] ADMINISTRATOR PRIVILEGES" -ForegroundColor Cyan
if ($isAdmin) {
    Write-Host "  ✓ Running as Administrator" -ForegroundColor Green
    Write-Host "  Hardware timestamping should be available" -ForegroundColor Green
} else {
    Write-Host "  ⚠️  NOT running as Administrator" -ForegroundColor Yellow
    Write-Host "  Hardware timestamping may be limited" -ForegroundColor Yellow
    Write-Host "  Run: Right-click PowerShell -> Run as Administrator" -ForegroundColor White
}

Write-Host ""
Write-Host "=== TROUBLESHOOTING COMPLETE ===" -ForegroundColor Red
Write-Host "Next step: Use Wireshark to capture PTP traffic during gPTP execution" -ForegroundColor Yellow