#!/usr/bin/env powershell
# Test Script: Identify which AVDECC implementation is sending packets
# This helps determine if Intel AVB or PCAP version is working

Write-Host "🧪 AVDECC Implementation Packet Test" -ForegroundColor Magenta
Write-Host "=====================================" -ForegroundColor Magenta
Write-Host ""

# Stop any existing PCAP implementation
Write-Host "🛑 Stopping any existing PCAP implementations..." -ForegroundColor Yellow
Get-Process | Where-Object {$_.ProcessName -like "*pcap*avdecc*"} | Stop-Process -Force -ErrorAction SilentlyContinue

# Check Intel AVB status
$intelProcess = Get-Process -Name "*intel_avb*" -ErrorAction SilentlyContinue
if ($intelProcess) {
    Write-Host "✅ Intel AVB Entity is running (PID: $($intelProcess.Id))" -ForegroundColor Green
    Write-Host ""
    Write-Host "📊 Test Instructions:" -ForegroundColor Cyan
    Write-Host "=====================" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "1. In Wireshark, use this filter:" -ForegroundColor White
    Write-Host "   eth.src == c0:47:e0:16:7b:89 and eth.type == 0x22f0" -ForegroundColor Green
    Write-Host ""
    Write-Host "2. Look for packets every 2 seconds with:" -ForegroundColor White
    Write-Host "   - Source: c0:47:e0:16:7b:89" -ForegroundColor Gray
    Write-Host "   - Destination: 91:e0:f0:01:00:00" -ForegroundColor Gray
    Write-Host "   - Protocol: IEEE 1722 Audio Video Transport Protocol" -ForegroundColor Gray
    Write-Host "   - Subtype: ADP (0xfa)" -ForegroundColor Gray
    Write-Host ""
    Write-Host "3. If you see these packets:" -ForegroundColor White
    Write-Host "   ✅ Intel AVB implementation is working!" -ForegroundColor Green
    Write-Host "   ✅ Hardware filter driver is avoiding packet duplication" -ForegroundColor Green
    Write-Host ""
    Write-Host "4. If you DON'T see these packets:" -ForegroundColor White
    Write-Host "   ❌ Intel AVB may need debugging" -ForegroundColor Red
    Write-Host "   💡 Try the PCAP implementation as fallback" -ForegroundColor Yellow
    
} else {
    Write-Host "❌ Intel AVB Entity is NOT running" -ForegroundColor Red
    Write-Host ""
    Write-Host "🔄 Starting PCAP implementation for testing..." -ForegroundColor Yellow
    
    # Start PCAP version as Administrator
    try {
        $pcapPath = "D:\Repos\OpenAvnu\build\bin\Release\intel_pcap_avdecc_entity_responsive.exe"
        if (Test-Path $pcapPath) {
            Write-Host "🚀 Launching PCAP AVDECC Entity..." -ForegroundColor Cyan
            Start-Process -FilePath $pcapPath -Verb RunAs -PassThru
            Start-Sleep 3
            
            $pcapProcess = Get-Process -Name "*pcap*avdecc*" -ErrorAction SilentlyContinue
            if ($pcapProcess) {
                Write-Host "✅ PCAP Entity started (PID: $($pcapProcess.Id))" -ForegroundColor Green
                Write-Host ""
                Write-Host "📊 Now test with same Wireshark filter:" -ForegroundColor Cyan
                Write-Host "   eth.src == c0:47:e0:16:7b:89 and eth.type == 0x22f0" -ForegroundColor Green
                Write-Host ""
                Write-Host "⚠️  Note: PCAP implementation may show packet duplication" -ForegroundColor Yellow
                Write-Host "   This is why we prefer the Intel AVB filter driver approach" -ForegroundColor Yellow
            } else {
                Write-Host "❌ Failed to start PCAP entity" -ForegroundColor Red
            }
        } else {
            Write-Host "❌ PCAP executable not found at: $pcapPath" -ForegroundColor Red
        }
    } catch {
        Write-Host "❌ Error starting PCAP entity: $_" -ForegroundColor Red
    }
}

Write-Host ""
Write-Host "🎯 Summary:" -ForegroundColor Magenta
Write-Host "===========" -ForegroundColor Magenta
Write-Host ""
Write-Host "✅ Intel AVB: Uses hardware filter driver, avoids packet duplication" -ForegroundColor Green
Write-Host "⚠️  PCAP: Software-based, may have packet duplication issues" -ForegroundColor Yellow
Write-Host ""
Write-Host "🏆 Recommended: Use Intel AVB implementation for production" -ForegroundColor Cyan
