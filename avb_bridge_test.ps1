param([switch]$TestOnly)
Write-Host ""
Write-Host " OpenAvnu AVB Audio Bridge Test " -ForegroundColor Magenta
Write-Host ""

# Check Virtual Audio Cable
Write-Host " Checking for Virtual Audio Cable..." -ForegroundColor Yellow
$audioDevices = Get-WmiObject -Class Win32_SoundDevice | Where-Object { $_.Name -like "*VB-Audio*" -or $_.Name -like "*CABLE*" }
if ($audioDevices.Count -gt 0) {
    Write-Host " Virtual Audio Cable detected" -ForegroundColor Green
} else {
    Write-Host " Virtual Audio Cable not found!" -ForegroundColor Red
    Write-Host "Download from: https://vb-audio.com/Cable/" -ForegroundColor Cyan
}

# Check daemons
Write-Host ""
Write-Host " Checking OpenAvnu Daemons..." -ForegroundColor Yellow
$daemons = Get-Process -Name "mrpd","maap_daemon" -ErrorAction SilentlyContinue
if ($daemons) {
    Write-Host " OpenAvnu Daemons running" -ForegroundColor Green
} else {
    Write-Host " OpenAvnu Daemons not running!" -ForegroundColor Red
}

Write-Host ""
Write-Host " Test completed!" -ForegroundColor Green
