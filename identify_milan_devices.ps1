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
            $vendors = @{
                "00:0A:92" = "Hirschmann (Industrial Networking)"
                "00:1C:F0" = "PreSonus Audio Electronics"
                "00:07:7D" = "MOTU (Mark of the Unicorn)"
                "00:50:C2" = "L-Acoustics"
                "00:80:92" = "Focusrite Audio Engineering"
                "00:0C:CC" = "Behringer/Music Group"
            }
            
            if ($vendors.ContainsKey($macPrefix)) {
                Write-Host "  Vendor: $($vendors[$macPrefix])" -ForegroundColor Green
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
                switch ($port) {
                    80 { Write-Host "    HTTP (80): OPEN - Web interface likely available" -ForegroundColor Green }
                    443 { Write-Host "    HTTPS (443): OPEN - Secure web interface available" -ForegroundColor Green }
                    22 { Write-Host "    SSH (22): OPEN - SSH access available" -ForegroundColor Green }
                    23 { Write-Host "    Telnet (23): OPEN - Telnet access available" -ForegroundColor Yellow }
                    161 { Write-Host "    SNMP (161): OPEN - Network management available" -ForegroundColor Green }
                    319 { Write-Host "    PTP Event (319): OPEN - PTP enabled!" -ForegroundColor Green }
                    320 { Write-Host "    PTP General (320): OPEN - PTP enabled!" -ForegroundColor Green }
                    8080 { Write-Host "    HTTP Alt (8080): OPEN - Alternative web interface" -ForegroundColor Green }
                    default { Write-Host "    Port ${port}: OPEN" -ForegroundColor White }
                }
            }
        } catch {
            # Port closed or filtered - silent
        }
    }
    
    if ($openPorts.Count -eq 0) {
        Write-Host "    No common ports open - device may be heavily firewalled" -ForegroundColor Yellow
    }
    
    # Try to access web interface
    Write-Host "  Web Interface Test:" -ForegroundColor Cyan
    $webUrls = @("http://$device", "https://$device", "http://${device}:8080")
    
    foreach ($url in $webUrls) {
        try {
            $response = Invoke-WebRequest -Uri $url -TimeoutSec 5 -ErrorAction SilentlyContinue
            if ($response.StatusCode -eq 200) {
                Write-Host "    ${url}: ACCESSIBLE" -ForegroundColor Green
                Write-Host "    Title: $($response.ParsedHtml.title)" -ForegroundColor Gray
                break
            }
        } catch {
            # Web interface not available on this URL
        }
    }
    
    # Device identification suggestions
    Write-Host "  Device Type Assessment:" -ForegroundColor Cyan
    if (319 -in $openPorts -and 320 -in $openPorts) {
        Write-Host "    ✅ PTP ENABLED - This device is running PTP!" -ForegroundColor Green
        Write-Host "    Device should be visible to gPTP" -ForegroundColor Green
    } elseif (80 -in $openPorts -or 443 -in $openPorts) {
        Write-Host "    🌐 Web interface available - check device configuration" -ForegroundColor Yellow
        Write-Host "    Access $device in web browser to enable PTP/Milan mode" -ForegroundColor White
    } elseif (22 -in $openPorts) {
        Write-Host "    🔧 SSH available - command line configuration possible" -ForegroundColor Yellow
    } elseif (161 -in $openPorts) {
        Write-Host "    📊 SNMP available - network management device" -ForegroundColor Yellow
    } else {
        Write-Host "    ❓ Device type unclear - limited access" -ForegroundColor Red
        Write-Host "    May need physical access or different configuration method" -ForegroundColor Gray
    }
    
    Write-Host ""
}

# Summary and next steps
Write-Host "[SUMMARY] Next Steps Based on Device Analysis:" -ForegroundColor Magenta
Write-Host "1. If web interfaces are available:" -ForegroundColor White
Write-Host "   - Open browser to device IP addresses" -ForegroundColor Gray
Write-Host "   - Look for PTP, AVB, or Milan settings" -ForegroundColor Gray
Write-Host "   - Enable IEEE 1588 PTP protocol" -ForegroundColor Gray
Write-Host ""
Write-Host "2. If devices are Hirschmann switches (MAC 00:0A:92):" -ForegroundColor White
Write-Host "   - These are industrial network switches" -ForegroundColor Gray
Write-Host "   - May support IEEE 1588 but need configuration" -ForegroundColor Gray
Write-Host "   - Check for PTP transparent clock or boundary clock modes" -ForegroundColor Gray
Write-Host ""
Write-Host "3. If no web/SSH access:" -ForegroundColor White
Write-Host "   - Contact network administrator" -ForegroundColor Gray
Write-Host "   - Check device documentation for PTP enablement" -ForegroundColor Gray
Write-Host "   - Verify devices are actually Milan-capable" -ForegroundColor Gray
Write-Host ""

Write-Host "=== DEVICE IDENTIFICATION COMPLETE ===" -ForegroundColor Green