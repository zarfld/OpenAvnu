# BMCA Debug Analysis
# Why isn't BMCA selecting a master from available devices?

Write-Host "=== BMCA FAILURE ANALYSIS ===" -ForegroundColor Red
Write-Host "USER CORRECT: BMCA should select master automatically!" -ForegroundColor Yellow
Write-Host ""

Write-Host "[BMCA THEORY]" -ForegroundColor Yellow
Write-Host "=============" -ForegroundColor Gray
Write-Host "✓ BMCA compares ALL announce messages on network" -ForegroundColor Green
Write-Host "✓ Even with identical priority1=248, BMCA uses tie-breakers:" -ForegroundColor Green
Write-Host "  1. Priority1 (same for all: 248)" -ForegroundColor White
Write-Host "  2. ClockClass (same for all: 248)" -ForegroundColor White
Write-Host "  3. ClockAccuracy (potential tie-breaker)" -ForegroundColor White
Write-Host "  4. ClockVariance (potential tie-breaker)" -ForegroundColor White
Write-Host "  5. Priority2 (potential tie-breaker)" -ForegroundColor White
Write-Host "  6. ClockIdentity (MAC-based - FINAL tie-breaker)" -ForegroundColor Cyan
Write-Host ""

Write-Host "[PROBLEM DIAGNOSIS]" -ForegroundColor Yellow
Write-Host "==================" -ForegroundColor Gray
Write-Host "If BMCA isn't working, the issue must be:" -ForegroundColor Cyan

Write-Host ""
Write-Host "1. ANNOUNCE MESSAGE TRANSMISSION?" -ForegroundColor Red
Write-Host "   ✗ Our device not sending announce messages" -ForegroundColor Red
Write-Host "   ✗ Milan devices not sending announce messages" -ForegroundColor Red
Write-Host "   ✗ Announce messages not reaching other devices" -ForegroundColor Red

Write-Host ""
Write-Host "2. MULTICAST LAYER 2 ISSUES?" -ForegroundColor Red
Write-Host "   ✗ Multicast 01-1B-19-00-00-00 not working" -ForegroundColor Red
Write-Host "   ✗ Switch blocking multicast traffic" -ForegroundColor Red
Write-Host "   ✗ VLAN configuration issues" -ForegroundColor Red

Write-Host ""
Write-Host "3. DOMAIN MISMATCH?" -ForegroundColor Red
Write-Host "   ✗ Different gPTP domains (domain 0 vs others)" -ForegroundColor Red
Write-Host "   ✗ Milan vs standard gPTP domain separation" -ForegroundColor Red

Write-Host ""
Write-Host "4. ANNOUNCE INTERVAL ISSUES?" -ForegroundColor Red
Write-Host "   ✗ Announce interval too long/short" -ForegroundColor Red
Write-Host "   ✗ Announce timeout too short" -ForegroundColor Red

Write-Host ""
Write-Host "[DEBUGGING STEPS]" -ForegroundColor Yellow
Write-Host "=================" -ForegroundColor Gray

Write-Host "Step 1: Check if WE are sending announces" -ForegroundColor Cyan
$gptpProcess = Get-Process -Name "gptp" -ErrorAction SilentlyContinue
if ($gptpProcess) {
    Write-Host "✓ gPTP process running (PID: $($gptpProcess.Id))" -ForegroundColor Green
    
    # Check recent log for announce activity
    $logFiles = Get-ChildItem "*.log" | Sort-Object LastWriteTime -Descending | Select-Object -First 3
    if ($logFiles) {
        Write-Host "Checking recent logs for announce activity..." -ForegroundColor White
        foreach ($logFile in $logFiles) {
            $announceLines = Get-Content $logFile.Name -ErrorAction SilentlyContinue | Select-String -Pattern "announce|Announce|ANNOUNCE" | Select-Object -First 3
            if ($announceLines) {
                Write-Host "✓ Found announce activity in $($logFile.Name):" -ForegroundColor Green
                $announceLines | ForEach-Object { Write-Host "    $_" -ForegroundColor Gray }
                break
            }
        }
        if (-not $announceLines) {
            Write-Host "✗ NO announce activity found in recent logs" -ForegroundColor Red
        }
    }
} else {
    Write-Host "✗ gPTP process not running" -ForegroundColor Red
}

Write-Host ""
Write-Host "Step 2: Check network multicast capability" -ForegroundColor Cyan
Write-Host "Testing Layer 2 multicast support..." -ForegroundColor White

# Check if network adapter supports multicast
$adapter = Get-NetAdapter | Where-Object { $_.InterfaceDescription -like '*I226*' -and $_.Status -eq 'Up' } | Select-Object -First 1
if ($adapter) {
    Write-Host "✓ Intel I226 adapter active: $($adapter.Name)" -ForegroundColor Green
    
    # Check multicast settings
    try {
        $multicastInfo = Get-NetAdapterAdvancedProperty -Name $adapter.Name -DisplayName "*multicast*" -ErrorAction SilentlyContinue
        if ($multicastInfo) {
            Write-Host "✓ Multicast settings found:" -ForegroundColor Green
            $multicastInfo | ForEach-Object { Write-Host "    $($_.DisplayName): $($_.DisplayValue)" -ForegroundColor Gray }
        } else {
            Write-Host "⚠️ No multicast settings visible" -ForegroundColor Yellow
        }
    } catch {
        Write-Host "⚠️ Could not query multicast settings" -ForegroundColor Yellow
    }
} else {
    Write-Host "✗ Intel adapter not found or not active" -ForegroundColor Red
}

Write-Host ""
Write-Host "Step 3: Check BMCA configuration validity" -ForegroundColor Cyan
$milanConfig = "thirdparty\gptp\test_milan_config.ini"
if (Test-Path $milanConfig) {
    Write-Host "✓ Milan config file exists" -ForegroundColor Green
    $config = Get-Content $milanConfig
    
    # Check critical BMCA parameters
    $announceInterval = $config | Select-String "announce_interval"
    $announceTimeout = $config | Select-String "announceReceiptTimeout"
    
    Write-Host "BMCA timing configuration:" -ForegroundColor White
    if ($announceInterval) { Write-Host "    $announceInterval" -ForegroundColor Gray }
    if ($announceTimeout) { Write-Host "    $announceTimeout" -ForegroundColor Gray }
    
    # Check if announce interval allows BMCA
    if ($announceInterval -match "announce_interval.*=.*0") {
        Write-Host "✓ Announce interval = 0 (1 second) - Good for BMCA" -ForegroundColor Green
    } else {
        Write-Host "⚠️ Announce interval may be too long for quick BMCA" -ForegroundColor Yellow
    }
} else {
    Write-Host "✗ Milan config file not found" -ForegroundColor Red
}

Write-Host ""
Write-Host "[HYPOTHESIS]" -ForegroundColor Yellow
Write-Host "============" -ForegroundColor Gray
Write-Host "Most likely cause:" -ForegroundColor Cyan
Write-Host "1. Announce messages not being transmitted (Layer 2 issue)" -ForegroundColor White
Write-Host "2. Milan devices in different mode (not participating in gPTP)" -ForegroundColor White
Write-Host "3. Network infrastructure blocking multicast" -ForegroundColor White
Write-Host ""

Write-Host "SOLUTION APPROACH:" -ForegroundColor Green
Write-Host "1. Verify announce message transmission with Wireshark" -ForegroundColor White
Write-Host "2. Check switch multicast configuration" -ForegroundColor White
Write-Host "3. Test with simpler gPTP configuration" -ForegroundColor White
Write-Host ""

Write-Host "=== BMCA DEBUG ANALYSIS COMPLETE ===" -ForegroundColor Red