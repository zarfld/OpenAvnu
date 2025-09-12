# Milan Device Identification Script
# Attempts to identify what type of devices we're dealing with

Write-Host "=== MILAN DEVICE IDENTIFICATION ===" -ForegroundColor Green
Write-Host "Attempting to identify and configure Milan devices" -ForegroundColor Cyan
Write-Host ""

$milanDevices = @("157.247.3.12", "157.247.1.112")

foreach ($device in $milanDevices) {
    Write-Host "[DEVICE] $device" -ForegroundColor Yellow
    Write-Host "========================================" -ForegroundColor Gray
    
    # Basic connectivity test
    Write-Host "  Basic Connectivity:" -NoNewline
    $ping = Test-Connection -ComputerName $device -Count 1 -Quiet -ErrorAction SilentlyContinue
    if ($ping) {
        Write-Host " REACHABLE" -ForegroundColor Green
    } else {
        Write-Host " NOT REACHABLE" -ForegroundColor Red
        continue
    }
    
    # Get MAC address from ARP
    Write-Host "  MAC Address:" -NoNewline
    $arpInfo = arp -a $device 2>$null | Where-Object { $_ -match $device }
    if ($arpInfo) {
        $macMatch = [regex]::Match($arpInfo, '([a-fA-F0-9]{2}[-:]){5}[a-fA-F0-9]{2}')
        if ($macMatch.Success) {
            $mac = $macMatch.Value
            Write-Host " $mac" -ForegroundColor White
            
            # Try to identify vendor from MAC
            $macPrefix = $mac.Substring(0, 8).Replace("-", ":").ToUpper()
            if ($macPrefix -eq "00:0A:92") {
                Write-Host "  Vendor: Hirschmann (Industrial Networking)" -ForegroundColor Green
            } elseif ($macPrefix -eq "00:1C:F0") {
                Write-Host "  Vendor: PreSonus Audio Electronics" -ForegroundColor Green
            } elseif ($macPrefix -eq "00:07:7D") {
                Write-Host "  Vendor: MOTU (Mark of the Unicorn)" -ForegroundColor Green
            } else {
                Write-Host "  Vendor: Unknown (MAC: $macPrefix)" -ForegroundColor Yellow
            }
        } else {
            Write-Host " Not found in ARP" -ForegroundColor Red
        }
    }
    
    # Try to detect open ports
    Write-Host "  Port Scan:" -ForegroundColor Cyan
    $commonPorts = @(80, 443, 22, 23, 161, 319, 320, 8080)
    $openPorts = @()
    
    foreach ($port in $commonPorts) {
        try {
            $connection = Test-NetConnection -ComputerName $device -Port $port -WarningAction SilentlyContinue -ErrorAction SilentlyContinue -InformationLevel Quiet
            if ($connection) {
                $openPorts += $port
                if ($port -eq 80) {
                    Write-Host "    HTTP (80): OPEN - Web interface likely available" -ForegroundColor Green
                } elseif ($port -eq 443) {
                    Write-Host "    HTTPS (443): OPEN - Secure web interface available" -ForegroundColor Green
                } elseif ($port -eq 319) {
                    Write-Host "    PTP Event (319): OPEN - PTP enabled!" -ForegroundColor Green
                } elseif ($port -eq 320) {
                    Write-Host "    PTP General (320): OPEN - PTP enabled!" -ForegroundColor Green
                } else {
                    Write-Host "    Port $port OPEN" -ForegroundColor White
                }
            }
        } catch {
            # Port closed or filtered - silent
        }
    }
    
    if ($openPorts.Count -eq 0) {
        Write-Host "    No common ports open - device may be heavily firewalled" -ForegroundColor Yellow
    }
    
    # Device identification suggestions
    Write-Host "  Device Assessment:" -ForegroundColor Cyan
    if (319 -in $openPorts -and 320 -in $openPorts) {
        Write-Host "    SUCCESS: PTP ENABLED - This device is running PTP!" -ForegroundColor Green
    } elseif (80 -in $openPorts -or 443 -in $openPorts) {
        Write-Host "    Web interface available - check device configuration" -ForegroundColor Yellow
        Write-Host "    Try accessing http://$device in web browser" -ForegroundColor White
    } else {
        Write-Host "    Device type unclear - limited access" -ForegroundColor Red
    }
    
    Write-Host ""
}

Write-Host "[SUMMARY] Based on device analysis:" -ForegroundColor Magenta
Write-Host "1. If devices have web interfaces, access them to enable PTP" -ForegroundColor White
Write-Host "2. If Hirschmann devices, they may need PTP configuration" -ForegroundColor White
Write-Host "3. Contact network administrator if no access available" -ForegroundColor White

Write-Host "`n=== DEVICE IDENTIFICATION COMPLETE ===" -ForegroundColor Green