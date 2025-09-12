# PTP Traffic Diagnostic Script
# Checks for PTP communication issues with Milan devices

Write-Host "=== PTP TRAFFIC DIAGNOSTIC ===" -ForegroundColor Green
Write-Host "Diagnosing why gPTP is not communicating with Milan devices" -ForegroundColor Cyan
Write-Host ""

# Get the connected Intel adapter
$connectedAdapter = Get-NetAdapter | Where-Object { 
    ($_.InterfaceDescription -like '*I226*' -or $_.InterfaceDescription -like '*I225*' -or $_.InterfaceDescription -like '*I219*' -or $_.InterfaceDescription -like '*I210*') -and 
    $_.Status -eq 'Up' 
} | Select-Object -First 1

if (-not $connectedAdapter) {
    Write-Host "ERROR: No connected Intel adapter found" -ForegroundColor Red
    exit 1
}

Write-Host "[1] ADAPTER STATUS" -ForegroundColor Yellow
Write-Host "  Adapter: $($connectedAdapter.Name) ($($connectedAdapter.InterfaceDescription))" -ForegroundColor Green
Write-Host "  MAC: $($connectedAdapter.MacAddress)" -ForegroundColor White
Write-Host "  Status: $($connectedAdapter.Status)" -ForegroundColor Green
Write-Host "  Speed: $($connectedAdapter.LinkSpeed)" -ForegroundColor White

# Get IP configuration
$ipConfig = Get-NetIPAddress -InterfaceIndex $connectedAdapter.InterfaceIndex -AddressFamily IPv4 -ErrorAction SilentlyContinue
if ($ipConfig) {
    Write-Host "  IP: $($ipConfig.IPAddress)" -ForegroundColor Green
} else {
    Write-Host "  ERROR: No IP configuration" -ForegroundColor Red
    exit 1
}
Write-Host ""

# Check Windows Firewall for PTP ports
Write-Host "[2] WINDOWS FIREWALL CHECK" -ForegroundColor Yellow
$ptpPorts = @(319, 320)
foreach ($port in $ptpPorts) {
    try {
        $firewallRules = Get-NetFirewallRule | Where-Object { 
            $_.DisplayName -like "*PTP*" -or $_.DisplayName -like "*$port*" 
        }
        if ($firewallRules) {
            Write-Host "  Found PTP firewall rules for port $port" -ForegroundColor Green
        } else {
            Write-Host "  WARNING: No specific PTP firewall rules found for port $port" -ForegroundColor Yellow
        }
    } catch {
        Write-Host "  WARNING: Could not check firewall rules" -ForegroundColor Yellow
    }
}

# Check if PTP ports are listening
Write-Host ""
Write-Host "[3] PTP PORT STATUS" -ForegroundColor Yellow
foreach ($port in $ptpPorts) {
    $listening = Get-NetUDPEndpoint | Where-Object { $_.LocalPort -eq $port }
    if ($listening) {
        Write-Host "  UDP ${port}: LISTENING" -ForegroundColor Green
        Write-Host "    Process: $($listening.OwningProcess)" -ForegroundColor Gray
    } else {
        Write-Host "  UDP ${port}: NOT LISTENING" -ForegroundColor Red
    }
}
Write-Host ""

# Test Milan devices connectivity
Write-Host "[4] MILAN DEVICE CONNECTIVITY" -ForegroundColor Yellow
$milanDevices = @("157.247.3.12", "157.247.1.112")
foreach ($device in $milanDevices) {
    Write-Host "  Testing $device..." -NoNewline
    try {
        $ping = Test-Connection -ComputerName $device -Count 1 -Quiet -ErrorAction SilentlyContinue
        if ($ping) {
            Write-Host " REACHABLE" -ForegroundColor Green
            
            # Try to test PTP ports specifically
            foreach ($port in $ptpPorts) {
                try {
                    $connection = Test-NetConnection -ComputerName $device -Port $port -WarningAction SilentlyContinue -ErrorAction SilentlyContinue
                    if ($connection.TcpTestSucceeded) {
                        Write-Host "    Port ${port}: OPEN" -ForegroundColor Green
                    } else {
                        Write-Host "    Port ${port}: CLOSED/FILTERED" -ForegroundColor Yellow
                    }
                } catch {
                    Write-Host "    Port ${port}: UDP test not supported" -ForegroundColor Gray
                }
            }
        } else {
            Write-Host " NOT REACHABLE" -ForegroundColor Red
        }
    } catch {
        Write-Host " ERROR: $($_.Exception.Message)" -ForegroundColor Red
    }
}
Write-Host ""

# Check for multicast support
Write-Host "[5] MULTICAST SUPPORT" -ForegroundColor Yellow
try {
    $multicastGroups = Get-NetRoute | Where-Object { $_.DestinationPrefix -like "224.*" }
    if ($multicastGroups) {
        Write-Host "  Multicast routing: ENABLED" -ForegroundColor Green
        Write-Host "  Found $($multicastGroups.Count) multicast routes" -ForegroundColor White
    } else {
        Write-Host "  WARNING: No multicast routes found" -ForegroundColor Yellow
    }
} catch {
    Write-Host "  WARNING: Could not check multicast support" -ForegroundColor Yellow
}
Write-Host ""

# Network interface statistics
Write-Host "[6] NETWORK INTERFACE STATISTICS" -ForegroundColor Yellow
try {
    $stats = Get-NetAdapterStatistics -Name $connectedAdapter.Name
    Write-Host "  Packets Sent: $($stats.SentUnicastPackets)" -ForegroundColor White
    Write-Host "  Packets Received: $($stats.ReceivedUnicastPackets)" -ForegroundColor White
    Write-Host "  Errors: Send=$($stats.OutboundErrors), Receive=$($stats.InboundErrors)" -ForegroundColor $(if ($stats.OutboundErrors -gt 0 -or $stats.InboundErrors -gt 0) { 'Yellow' } else { 'Green' })
} catch {
    Write-Host "  WARNING: Could not get interface statistics" -ForegroundColor Yellow
}
Write-Host ""

# Suggest next steps
Write-Host "[7] RECOMMENDED ACTIONS" -ForegroundColor Yellow
Write-Host "  1. Check if Milan devices are in PTP/AVB mode:" -ForegroundColor White
Write-Host "     - Access device web interface or configuration" -ForegroundColor Gray
Write-Host "     - Enable 'IEEE 1588 PTP' or 'AVB/Milan' mode" -ForegroundColor Gray
Write-Host ""
Write-Host "  2. Temporarily disable Windows Firewall for testing:" -ForegroundColor White
Write-Host "     netsh advfirewall set allprofiles state off" -ForegroundColor Gray
Write-Host "     (Re-enable after testing!)" -ForegroundColor Red
Write-Host ""
Write-Host "  3. Capture network traffic with Wireshark:" -ForegroundColor White
Write-Host "     - Filter: udp.port == 319 or udp.port == 320" -ForegroundColor Gray
Write-Host "     - Look for PTP messages from Milan devices" -ForegroundColor Gray
Write-Host ""
Write-Host "  4. Check if gPTP is sending on correct interface:" -ForegroundColor White
Write-Host "     - Verify MAC address in gPTP command line" -ForegroundColor Gray
Write-Host "     - Try explicit interface specification" -ForegroundColor Gray
Write-Host ""

Write-Host "=== DIAGNOSTIC COMPLETE ===" -ForegroundColor Green
Write-Host "Next: Run Wireshark capture during gPTP test to see actual traffic" -ForegroundColor Cyan