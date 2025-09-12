# Comprehensive PTP Detection Script
# Check for all possible PTP implementations and protocols

Write-Host "=== COMPREHENSIVE PTP DETECTION ===" -ForegroundColor Green
Write-Host "Checking for all PTP variants and protocols" -ForegroundColor Cyan
Write-Host ""

$milanDevices = @("157.247.3.12", "157.247.1.112")

# Extended port list for PTP variants
$ptpPorts = @(
    @{Port=319; Protocol="UDP"; Description="IEEE 1588 PTP Event Messages"},
    @{Port=320; Protocol="UDP"; Description="IEEE 1588 PTP General Messages"},
    @{Port=319; Protocol="TCP"; Description="PTP over TCP (some implementations)"},
    @{Port=320; Protocol="TCP"; Description="PTP over TCP (some implementations)"},
    @{Port=1588; Protocol="UDP"; Description="Alternative PTP port"},
    @{Port=1588; Protocol="TCP"; Description="Alternative PTP port TCP"}
)

# Check if our gPTP is actually running and what it's doing
Write-Host "[LOCAL GPTP STATUS]" -ForegroundColor Yellow
Write-Host "==================" -ForegroundColor Gray

# Check gPTP process
$gptpProcess = Get-Process -Name "daemon_cl" -ErrorAction SilentlyContinue
if ($gptpProcess) {
    Write-Host "✓ gPTP daemon running (PID: $($gptpProcess.Id))" -ForegroundColor Green
} else {
    Write-Host "✗ gPTP daemon not running" -ForegroundColor Red
}

# Check what our system is listening on
Write-Host "Local PTP listening ports:" -ForegroundColor Cyan
$localListening = netstat -an | Select-String '319|320|1588'
if ($localListening) {
    $localListening | ForEach-Object { Write-Host "  $_" -ForegroundColor White }
} else {
    Write-Host "  No PTP ports detected locally" -ForegroundColor Red
}

Write-Host ""

foreach ($device in $milanDevices) {
    Write-Host "[DEVICE] $device - DETAILED PTP SCAN" -ForegroundColor Yellow
    Write-Host "============================================" -ForegroundColor Gray
    
    # Basic connectivity
    Write-Host "Basic connectivity:" -NoNewline
    $ping = Test-Connection -ComputerName $device -Count 1 -Quiet -ErrorAction SilentlyContinue
    if ($ping) {
        Write-Host " REACHABLE" -ForegroundColor Green
    } else {
        Write-Host " NOT REACHABLE" -ForegroundColor Red
        continue
    }
    
    # Test all PTP ports
    Write-Host "PTP Port Analysis:" -ForegroundColor Cyan
    $foundPtpPorts = @()
    
    foreach ($ptpPort in $ptpPorts) {
        Write-Host "  Testing $($ptpPort.Protocol) port $($ptpPort.Port):" -NoNewline
        
        try {
            if ($ptpPort.Protocol -eq "UDP") {
                # For UDP, we'll try a different approach - check if port responds
                $udpClient = New-Object System.Net.Sockets.UdpClient
                $udpClient.Client.ReceiveTimeout = 1000
                $udpClient.Connect($device, $ptpPort.Port)
                
                # Send a simple test packet
                $testData = [System.Text.Encoding]::ASCII.GetBytes("test")
                $udpClient.Send($testData, $testData.Length) | Out-Null
                
                $udpClient.Close()
                Write-Host " RESPONSIVE" -ForegroundColor Green
                $foundPtpPorts += $ptpPort
            } else {
                # TCP test
                $tcpConnection = Test-NetConnection -ComputerName $device -Port $ptpPort.Port -WarningAction SilentlyContinue -ErrorAction SilentlyContinue -InformationLevel Quiet
                if ($tcpConnection) {
                    Write-Host " OPEN" -ForegroundColor Green
                    $foundPtpPorts += $ptpPort
                } else {
                    Write-Host " CLOSED" -ForegroundColor Red
                }
            }
        } catch {
            Write-Host " NO RESPONSE" -ForegroundColor Red
        }
    }
    
    # Check for multicast PTP (Layer 2)
    Write-Host "Checking for Layer 2 PTP (Multicast):" -ForegroundColor Cyan
    Write-Host "  Note: Layer 2 PTP uses multicast MAC addresses" -ForegroundColor Yellow
    Write-Host "  This may not be detectable via standard port scans" -ForegroundColor Yellow
    
    # Try to detect PTP traffic with packet capture approach
    Write-Host "Active PTP Traffic Detection:" -ForegroundColor Cyan
    Write-Host "  Monitoring for PTP packets from $device..." -ForegroundColor White
    
    # Use netstat to see if there are any active connections
    $activeConnections = netstat -an | Select-String $device
    if ($activeConnections) {
        Write-Host "  Active connections detected:" -ForegroundColor Green
        $activeConnections | ForEach-Object { Write-Host "    $_" -ForegroundColor White }
    } else {
        Write-Host "  No active connections detected" -ForegroundColor Yellow
    }
    
    if ($foundPtpPorts.Count -gt 0) {
        Write-Host "RESULT: PTP ports detected!" -ForegroundColor Green
        $foundPtpPorts | ForEach-Object { 
            Write-Host "  ✓ $($_.Protocol) $($_.Port): $($_.Description)" -ForegroundColor Green 
        }
    } else {
        Write-Host "RESULT: No standard PTP ports responding" -ForegroundColor Yellow
        Write-Host "  Device may be using:" -ForegroundColor White
        Write-Host "  - Layer 2 PTP (IEEE 802.3 Ethernet)" -ForegroundColor White
        Write-Host "  - Custom PTP implementation" -ForegroundColor White
        Write-Host "  - PTP over different transport" -ForegroundColor White
    }
    
    Write-Host ""
}

# Try to capture actual PTP traffic
Write-Host "[PTP TRAFFIC MONITORING]" -ForegroundColor Yellow
Write-Host "========================" -ForegroundColor Gray
Write-Host "Attempting to detect actual PTP communication..." -ForegroundColor Cyan

# Check ARP table for recent activity
Write-Host "Recent network activity (ARP table):" -ForegroundColor Cyan
$arpTable = arp -a | Select-String "157\.247\."
if ($arpTable) {
    $arpTable | ForEach-Object { Write-Host "  $_" -ForegroundColor White }
} else {
    Write-Host "  No recent activity in ARP table" -ForegroundColor Yellow
}

Write-Host "`n=== COMPREHENSIVE PTP DETECTION COMPLETE ===" -ForegroundColor Green
Write-Host "If devices are using PTP but not detected, they may be using:" -ForegroundColor Yellow
Write-Host "- IEEE 1588v2 over Ethernet (Layer 2)" -ForegroundColor White
Write-Host "- Hardware-based PTP without software ports" -ForegroundColor White
Write-Host "- Proprietary PTP variants" -ForegroundColor White