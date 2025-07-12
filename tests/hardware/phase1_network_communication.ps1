# OpenAvnu Hardware Testing - Phase 1: Basic Network Communication Test
# Test Case 2: Basic Network Communication Test  
# Date: 2025-01-11
# System: DEV01

param(
    [string]$TestTargetIP = "",
    [switch]$SkipConnectivity = $false
)

Write-Host "=== OpenAvnu Phase 1 Hardware Testing ===" -ForegroundColor Green
Write-Host "Test Case: Basic Network Communication" -ForegroundColor Yellow
Write-Host "Machine: $env:COMPUTERNAME" -ForegroundColor Cyan
Write-Host "Date: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')" -ForegroundColor Cyan
Write-Host ""

# Test Case 2.1: Check Ethernet adapter connectivity
Write-Host "2.1 Ethernet Adapter Connectivity Check..." -ForegroundColor Blue
Write-Host "=" * 50

$ethernetAdapter = Get-NetAdapter -Name "Ethernet" -ErrorAction SilentlyContinue
if (-not $ethernetAdapter) {
    Write-Host "❌ FAIL: Ethernet adapter not found" -ForegroundColor Red
    exit 1
}

Write-Host "Adapter: $($ethernetAdapter.InterfaceDescription)" -ForegroundColor White
Write-Host "Status: $($ethernetAdapter.Status)" -ForegroundColor $(if ($ethernetAdapter.Status -eq "Up") { "Green" } else { "Yellow" })
Write-Host "Speed: $($ethernetAdapter.LinkSpeed)" -ForegroundColor Gray
Write-Host "MAC: $($ethernetAdapter.MacAddress)" -ForegroundColor Gray

if ($ethernetAdapter.Status -ne "Up") {
    Write-Host ""
    Write-Host "⚠️  WARNING: Ethernet adapter is not connected" -ForegroundColor Yellow
    Write-Host "   - Please connect an Ethernet cable to proceed with connectivity tests" -ForegroundColor Yellow
    Write-Host "   - Or use -SkipConnectivity flag to skip ping tests" -ForegroundColor Yellow
    
    if (-not $SkipConnectivity) {
        Write-Host ""
        Write-Host "❌ FAIL: Cannot proceed with connectivity tests - adapter disconnected" -ForegroundColor Red
        Write-Host "   Rerun with: -SkipConnectivity to test other functionality" -ForegroundColor Yellow
        exit 1
    } else {
        Write-Host "   Skipping connectivity tests as requested..." -ForegroundColor Yellow
    }
}

# Test Case 2.2: Link speed and duplex verification
Write-Host ""
Write-Host "2.2 Link Speed and Duplex Configuration..." -ForegroundColor Blue
Write-Host "=" * 50

try {
    $speedDuplex = Get-NetAdapterAdvancedProperty -Name "Ethernet" -DisplayName "*Speed*" -ErrorAction SilentlyContinue
    if ($speedDuplex) {
        foreach ($prop in $speedDuplex) {
            Write-Host "  $($prop.DisplayName): $($prop.DisplayValue)" -ForegroundColor Gray
        }
    } else {
        Write-Host "  Speed/Duplex: Auto-negotiated ($($ethernetAdapter.LinkSpeed))" -ForegroundColor Gray
    }
    
    # Check for gigabit capability
    if ($ethernetAdapter.LinkSpeed -like "*Gbps*" -or $ethernetAdapter.LinkSpeed -like "*1000*") {
        Write-Host "  ✅ Gigabit capable" -ForegroundColor Green
    } else {
        Write-Host "  ⚠️  Not operating at Gigabit speed" -ForegroundColor Yellow
    }
} catch {
    Write-Host "  Error retrieving speed/duplex info: $($_.Exception.Message)" -ForegroundColor Red
}

# Test Case 2.3: IP Configuration check
Write-Host ""
Write-Host "2.3 IP Configuration..." -ForegroundColor Blue
Write-Host "=" * 50

$ipConfig = Get-NetIPConfiguration -InterfaceAlias "Ethernet" -ErrorAction SilentlyContinue
if ($ipConfig) {
    if ($ipConfig.IPv4Address) {
        Write-Host "  IPv4 Address: $($ipConfig.IPv4Address.IPAddress)" -ForegroundColor Green
        Write-Host "  Subnet Mask: $($ipConfig.IPv4Address.PrefixLength)" -ForegroundColor Gray
    } else {
        Write-Host "  IPv4 Address: Not configured" -ForegroundColor Yellow
    }
    
    if ($ipConfig.IPv4DefaultGateway) {
        Write-Host "  Default Gateway: $($ipConfig.IPv4DefaultGateway.NextHop)" -ForegroundColor Gray
    } else {
        Write-Host "  Default Gateway: None" -ForegroundColor Yellow
    }
} else {
    Write-Host "  ⚠️  No IP configuration found" -ForegroundColor Yellow
}

# Test Case 2.4: Basic connectivity test (if adapter is up and target specified)
if (-not $SkipConnectivity -and $ethernetAdapter.Status -eq "Up") {
    Write-Host ""
    Write-Host "2.4 Basic Connectivity Test..." -ForegroundColor Blue
    Write-Host "=" * 50
    
    if (-not $TestTargetIP) {
        # Try to find a suitable target on the local network
        if ($ipConfig -and $ipConfig.IPv4DefaultGateway) {
            $TestTargetIP = $ipConfig.IPv4DefaultGateway.NextHop
            Write-Host "  Using default gateway as target: $TestTargetIP" -ForegroundColor Gray
        } else {
            Write-Host "  No target IP specified and no gateway found" -ForegroundColor Yellow
            Write-Host "  Use -TestTargetIP parameter to specify a target" -ForegroundColor Yellow
            $SkipConnectivity = $true
        }
    } else {
        Write-Host "  Using specified target: $TestTargetIP" -ForegroundColor Gray
    }
    
    if (-not $SkipConnectivity) {
        Write-Host "  Testing connectivity to $TestTargetIP..." -ForegroundColor White
        
        # Ping test
        $pingResult = Test-Connection -ComputerName $TestTargetIP -Count 4 -ErrorAction SilentlyContinue
        if ($pingResult) {
            $avgLatency = ($pingResult | Measure-Object -Property ResponseTime -Average).Average
            Write-Host "  ✅ Ping successful - Average latency: $([math]::Round($avgLatency, 2))ms" -ForegroundColor Green
            
            # Check for consistent latency (jitter test)
            $maxLatency = ($pingResult | Measure-Object -Property ResponseTime -Maximum).Maximum
            $minLatency = ($pingResult | Measure-Object -Property ResponseTime -Minimum).Minimum
            $jitter = $maxLatency - $minLatency
            
            Write-Host "  Latency range: $minLatency - $maxLatency ms (jitter: $jitter ms)" -ForegroundColor Gray
            
            if ($jitter -lt 5) {
                Write-Host "  ✅ Low jitter - Good network stability" -ForegroundColor Green
            } elseif ($jitter -lt 20) {
                Write-Host "  ⚠️  Moderate jitter" -ForegroundColor Yellow
            } else {
                Write-Host "  ❌ High jitter - Network may be unstable" -ForegroundColor Red
            }
        } else {
            Write-Host "  ❌ Ping failed - No connectivity to target" -ForegroundColor Red
        }
    }
}

# Test Case 2.5: Jumbo frame capability test
Write-Host ""
Write-Host "2.5 Advanced Ethernet Features..." -ForegroundColor Blue
Write-Host "=" * 50

try {
    # Check MTU
    $mtu = (Get-NetIPInterface -InterfaceAlias "Ethernet" -AddressFamily IPv4).NlMtu
    Write-Host "  Current MTU: $mtu bytes" -ForegroundColor Gray
    
    # Check for jumbo frame support
    $jumboFrames = Get-NetAdapterAdvancedProperty -Name "Ethernet" | Where-Object { 
        $_.DisplayName -like "*Jumbo*" -or $_.DisplayName -like "*MTU*" 
    }
    
    if ($jumboFrames) {
        foreach ($prop in $jumboFrames) {
            Write-Host "  $($prop.DisplayName): $($prop.DisplayValue)" -ForegroundColor Gray
        }
    } else {
        Write-Host "  Jumbo Frames: Default (no specific setting found)" -ForegroundColor Gray
    }
    
    # Check for other relevant features
    $advancedProps = Get-NetAdapterAdvancedProperty -Name "Ethernet" | Where-Object {
        $_.DisplayName -like "*Flow Control*" -or
        $_.DisplayName -like "*Checksum*" -or
        $_.DisplayName -like "*Offload*"
    }
    
    if ($advancedProps) {
        Write-Host "  Advanced Features:" -ForegroundColor Gray
        foreach ($prop in $advancedProps) {
            Write-Host "    $($prop.DisplayName): $($prop.DisplayValue)" -ForegroundColor DarkGray
        }
    }
    
} catch {
    Write-Host "  Error checking advanced features: $($_.Exception.Message)" -ForegroundColor Red
}

# Summary
Write-Host ""
Write-Host "=== Test Summary ===" -ForegroundColor Green

$testsPassed = 0
$testsTotal = 5

# Adapter detection
if ($ethernetAdapter) {
    Write-Host "✅ Adapter Detection: PASS" -ForegroundColor Green
    $testsPassed++
} else {
    Write-Host "❌ Adapter Detection: FAIL" -ForegroundColor Red
}

# Link status
if ($ethernetAdapter.Status -eq "Up") {
    Write-Host "✅ Link Status: PASS (Connected)" -ForegroundColor Green
    $testsPassed++
} else {
    Write-Host "⚠️  Link Status: DISCONNECTED" -ForegroundColor Yellow
}

# Speed capability
if ($ethernetAdapter.LinkSpeed -like "*Gbps*" -or $ethernetAdapter.LinkSpeed -like "*1000*") {
    Write-Host "✅ Speed Capability: PASS (Gigabit)" -ForegroundColor Green
    $testsPassed++
} else {
    Write-Host "⚠️  Speed Capability: Limited" -ForegroundColor Yellow
}

# IP configuration
if ($ipConfig -and $ipConfig.IPv4Address) {
    Write-Host "✅ IP Configuration: PASS" -ForegroundColor Green
    $testsPassed++
} else {
    Write-Host "⚠️  IP Configuration: Not configured" -ForegroundColor Yellow
}

# Connectivity (if tested)
if ($SkipConnectivity) {
    Write-Host "⚠️  Connectivity: SKIPPED" -ForegroundColor Yellow
} elseif ($ethernetAdapter.Status -ne "Up") {
    Write-Host "❌ Connectivity: FAIL (No link)" -ForegroundColor Red
} elseif ($pingResult) {
    Write-Host "✅ Connectivity: PASS" -ForegroundColor Green
    $testsPassed++
} else {
    Write-Host "❌ Connectivity: FAIL" -ForegroundColor Red
}

Write-Host ""
Write-Host "Tests Passed: $testsPassed/$testsTotal" -ForegroundColor White

if ($testsPassed -ge 3) {
    Write-Host "Overall Status: ✅ PASS" -ForegroundColor Green
    Write-Host ""
    Write-Host "✅ Phase 1 Test Case 2 completed successfully!" -ForegroundColor Green
    Write-Host "Ready to proceed to Phase 2 (Register Access Testing)" -ForegroundColor Yellow
} else {
    Write-Host "Overall Status: ❌ FAIL" -ForegroundColor Red
    Write-Host ""
    Write-Host "❌ Phase 1 Test Case 2 failed. Please check network configuration." -ForegroundColor Red
}
