Write-Host "AVDECC Implementation Test" -ForegroundColor Cyan
Write-Host "=========================" -ForegroundColor Cyan
Write-Host ""

# Check Intel AVB Entity
$intelProcess = Get-Process -Name "*intel_avb*" -ErrorAction SilentlyContinue
if ($intelProcess) {
    Write-Host "Intel AVB Entity: RUNNING (PID: $($intelProcess.Id))" -ForegroundColor Green
} else {
    Write-Host "Intel AVB Entity: NOT RUNNING" -ForegroundColor Red
}

# Check PCAP Entity  
$pcapProcess = Get-Process -Name "*pcap*avdecc*" -ErrorAction SilentlyContinue
if ($pcapProcess) {
    Write-Host "PCAP Entity: RUNNING (PID: $($pcapProcess.Id))" -ForegroundColor Green
} else {
    Write-Host "PCAP Entity: NOT RUNNING" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "Wireshark Filter for our entity:" -ForegroundColor Yellow
Write-Host "eth.src == c0:47:e0:16:7b:89 and eth.type == 0x22f0" -ForegroundColor Green
Write-Host ""
Write-Host "Expected: ADP packets every 2 seconds if entity is working" -ForegroundColor White
