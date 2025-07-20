#!/usr/bin/env powershell
# Fix Critical AVDECC Entity Issues and Test

Write-Host "🔧 Fixing Critical AVDECC Entity Issues" -ForegroundColor Red
Write-Host "=======================================" -ForegroundColor Red
Write-Host ""

Write-Host "📋 Issues Identified from Wireshark:" -ForegroundColor Yellow
Write-Host "1. MAC Address Corruption: c0:47:0e:16:7b:89 (should be c0:47:e0:16:7b:89)" -ForegroundColor White
Write-Host "2. Entity ID Corruption: 0xc0470e167b890082 (should be 0xc047e0fffe167b89)" -ForegroundColor White
Write-Host "3. Missing Entity Model ID (all zeros)" -ForegroundColor White
Write-Host "4. Entity announces DEPARTING and disappears" -ForegroundColor White
Write-Host "5. Available Index increments every 2 seconds (should be stable)" -ForegroundColor White
Write-Host ""

# Stop any running entities
Write-Host "🛑 Stopping existing entities..." -ForegroundColor Cyan
Get-Process | Where-Object {$_.ProcessName -like "*avdecc*"} | Stop-Process -Force -ErrorAction SilentlyContinue

# Rebuild with fixes
Write-Host "🔨 Rebuilding with fixes..." -ForegroundColor Cyan
Set-Location "D:\Repos\OpenAvnu"
& cmake --build build --config Release --target intel_pcap_avdecc_entity_responsive

if ($LASTEXITCODE -eq 0) {
    Write-Host "✅ Build successful!" -ForegroundColor Green
    
    # Start corrected PCAP implementation
    Write-Host ""
    Write-Host "🚀 Starting Fixed PCAP AVDECC Entity..." -ForegroundColor Green
    Write-Host ""
    Write-Host "🎯 Expected Fixed Behavior:" -ForegroundColor Magenta
    Write-Host "- Correct MAC: c0:47:e0:16:7b:89" -ForegroundColor White
    Write-Host "- Correct Entity ID: 0xc047e0fffe167b89" -ForegroundColor White
    Write-Host "- Proper Entity Model ID: 0x0017fffe00000001" -ForegroundColor White
    Write-Host "- Audio capabilities shown" -ForegroundColor White
    Write-Host "- Available index stable until state change" -ForegroundColor White
    Write-Host "- NO entity departing messages" -ForegroundColor White
    Write-Host ""
    
    Write-Host "🔍 Wireshark Filter:" -ForegroundColor Yellow
    Write-Host "eth.src == c0:47:e0:16:7b:89 and eth.type == 0x22f0" -ForegroundColor Green
    Write-Host ""
    
    # Run with detailed output
    Start-Process -FilePath "D:\Repos\OpenAvnu\build\bin\Release\intel_pcap_avdecc_entity_responsive.exe" -Verb RunAs -PassThru
    
    Write-Host "✅ Fixed PCAP Entity launched!" -ForegroundColor Green
    Write-Host "📊 Check Wireshark for corrected packets" -ForegroundColor Cyan
    
} else {
    Write-Host "❌ Build failed!" -ForegroundColor Red
}
