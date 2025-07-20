#!/usr/bin/env powershell
# Verify IEEE 1722.1 AVDECC Traffic Test
# Checks both Intel AVB and PCAP implementations

Write-Host "🔍 IEEE 1722.1 AVDECC Traffic Verification" -ForegroundColor Cyan
Write-Host "==========================================" -ForegroundColor Cyan
Write-Host ""

# Check Intel AVB Entity Status
$intelProcess = Get-Process -Name "*intel_avb*" -ErrorAction SilentlyContinue
if ($intelProcess) {
    Write-Host "✅ Intel AVB Entity: RUNNING" -ForegroundColor Green
    Write-Host "   PID: $($intelProcess.Id)" -ForegroundColor White
    Write-Host "   Runtime: $((Get-Date) - $intelProcess.StartTime)" -ForegroundColor White
    Write-Host "   CPU: $($intelProcess.CPU)s" -ForegroundColor White
    Write-Host "   Memory: $([math]::Round($intelProcess.WorkingSet/1MB, 2)) MB" -ForegroundColor White
} else {
    Write-Host "❌ Intel AVB Entity: NOT RUNNING" -ForegroundColor Red
}

Write-Host ""

# Check PCAP Entity Status  
$pcapProcess = Get-Process -Name "*pcap*avdecc*" -ErrorAction SilentlyContinue
if ($pcapProcess) {
    Write-Host "✅ PCAP Entity: RUNNING" -ForegroundColor Green
    Write-Host "   PID: $($pcapProcess.Id)" -ForegroundColor White
} else {
    Write-Host "⚠️  PCAP Entity: NOT RUNNING" -ForegroundColor Yellow
    Write-Host "   (This is expected if using Intel AVB implementation)" -ForegroundColor Gray
}

Write-Host ""
Write-Host "🎯 Wireshark Filter Instructions:" -ForegroundColor Magenta
Write-Host "=================================" -ForegroundColor Magenta
Write-Host ""

Write-Host "1. IEEE 1722.1 AVDECC Protocol:" -ForegroundColor Yellow
Write-Host "   eth.type == 0x22f0" -ForegroundColor Green
Write-Host ""

Write-Host "2. ADP Discovery packets (multicast):" -ForegroundColor Yellow  
Write-Host "   eth.dst == 91:e0:f0:01:00:00" -ForegroundColor Green
Write-Host ""

Write-Host "3. Our OpenAvnu entity MAC:" -ForegroundColor Yellow
Write-Host "   eth.src == c0:47:e0:16:7b:89" -ForegroundColor Green
Write-Host ""

Write-Host "4. Combined filter (recommended):" -ForegroundColor Yellow
Write-Host "   eth.type == 0x22f0 and (eth.dst == 91:e0:f0:01:00:00 or eth.src == c0:47:e0:16:7b:89)" -ForegroundColor Green
Write-Host ""

Write-Host "📊 Expected Traffic Patterns:" -ForegroundColor Magenta
Write-Host "=============================" -ForegroundColor Magenta
Write-Host ""

Write-Host "✅ ADP ENTITY_AVAILABLE (every 2 seconds):" -ForegroundColor White
Write-Host "   - Source: c0:47:e0:16:7b:89" -ForegroundColor Gray
Write-Host "   - Destination: 91:e0:f0:01:00:00 (AVDECC multicast)" -ForegroundColor Gray
Write-Host "   - EtherType: 0x22f0 (IEEE 1722)" -ForegroundColor Gray
Write-Host "   - AVTP Subtype: 0xfa (ADP)" -ForegroundColor Gray
Write-Host ""

Write-Host "✅ AEM Command/Response traffic:" -ForegroundColor White  
Write-Host "   - Unicast between entities" -ForegroundColor Gray
Write-Host "   - AVTP Subtype: 0xfb (AECP)" -ForegroundColor Gray
Write-Host "   - Commands: ACQUIRE_ENTITY, READ_DESCRIPTOR, etc." -ForegroundColor Gray
Write-Host ""

Write-Host "🚨 Troubleshooting:" -ForegroundColor Red
Write-Host "==================" -ForegroundColor Red
Write-Host ""

Write-Host "If NO IEEE 1722.1 packets from c0:47:e0:16:7b:89:" -ForegroundColor Yellow
Write-Host "1. Check Wireshark is capturing on correct interface (Ethernet 3)" -ForegroundColor White
Write-Host "2. Verify Windows firewall allows raw packet injection" -ForegroundColor White
Write-Host "3. Confirm PCAP/Npcap driver is working properly" -ForegroundColor White
Write-Host "4. Try running entity as Administrator" -ForegroundColor White
Write-Host ""

Write-Host "💡 Success Indicators:" -ForegroundColor Green
Write-Host "=====================" -ForegroundColor Green
Write-Host ""

Write-Host "✅ Periodic ADP packets every 2 seconds from our entity" -ForegroundColor White
Write-Host "✅ Hive AVDECC Controller shows our entity in device list" -ForegroundColor White
Write-Host "✅ AEM command/response exchanges when interacting with entity" -ForegroundColor White
Write-Host "✅ Available_index increments only on actual state changes" -ForegroundColor White
Write-Host ""

Write-Host "🎉 Both Intel AVB and PCAP implementations are designed to" -ForegroundColor Cyan
Write-Host "   avoid packet duplication issues by using proper IEEE 1722.1 compliance!" -ForegroundColor Cyan
