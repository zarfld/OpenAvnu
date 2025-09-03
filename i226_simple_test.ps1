# Simple I226 Gigabit Test
param([int]$TestDuration = 15)

$isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")

if (-not $isAdmin) {
    Write-Host "Requesting Administrator privileges..." -ForegroundColor Yellow
    $scriptPath = $MyInvocation.MyCommand.Path
    Start-Process PowerShell -ArgumentList "-ExecutionPolicy Bypass", "-File", "`"$scriptPath`"", "-TestDuration $TestDuration" -Verb RunAs
    exit
}

Write-Host "I226 GIGABIT HARDWARE TIMESTAMPING TEST" -ForegroundColor Red
Write-Host ""

Set-Location "C:\Users\dzarf\source\repos\OpenAvnu-2"

# Check I226 adapter
$i226 = Get-NetAdapter -Name "Ethernet" -ErrorAction SilentlyContinue
if (-not $i226) {
    Write-Host "ERROR: I226 not found" -ForegroundColor Red
    pause
    exit 1
}

Write-Host "Current I226: $($i226.LinkSpeed) - $($i226.Status)" -ForegroundColor Cyan

# Set to Gigabit
if ($i226.LinkSpeed -ne "1 Gbps") {
    Write-Host "Setting to Gigabit..." -ForegroundColor Yellow
    try {
        Set-NetAdapterAdvancedProperty -Name "Ethernet" -DisplayName "Geschwindigkeit und Duplex" -DisplayValue "1000 Mbit/s Vollduplex"
        Start-Sleep 8
        $i226 = Get-NetAdapter -Name "Ethernet"
        Write-Host "Updated: $($i226.LinkSpeed)" -ForegroundColor Green
    } catch {
        Write-Host "Speed change failed, continuing..." -ForegroundColor Yellow
    }
}

# Test hardware timestamping
$mac = $i226.MacAddress.Replace("-", ":")
$logFile = "I226_gigabit_test.err"

Write-Host "Testing hardware timestamping..." -ForegroundColor Yellow
Write-Host "MAC: $mac" -ForegroundColor Gray
Write-Host "Duration: $TestDuration seconds" -ForegroundColor Gray

$process = Start-Process -FilePath ".\thirdparty\gptp\build\Release\gptp.exe" -ArgumentList $mac -RedirectStandardError $logFile -NoNewWindow -PassThru

Start-Sleep $TestDuration

if (-not $process.HasExited) {
    $process.Kill()
    $process.WaitForExit(3000)
}

Write-Host "Test complete! Analyzing..." -ForegroundColor Green

# Quick analysis
$content = Get-Content $logFile -Raw

$clockErrors = [regex]::Matches($content, "Unable.*determine.*clock", [System.Text.RegularExpressions.RegexOptions]::IgnoreCase).Count
$hardwareHits = [regex]::Matches($content, "hardware.*timestamp", [System.Text.RegularExpressions.RegexOptions]::IgnoreCase).Count  
$softwareHits = [regex]::Matches($content, "software.*timestamp", [System.Text.RegularExpressions.RegexOptions]::IgnoreCase).Count

Write-Host ""
Write-Host "RESULTS:" -ForegroundColor Magenta
Write-Host "Clock errors: $clockErrors" -ForegroundColor $(if($clockErrors -eq 0) {"Green"} else {"Red"})
Write-Host "Hardware evidence: $hardwareHits" -ForegroundColor Green
Write-Host "Software evidence: $softwareHits" -ForegroundColor Red

if ($clockErrors -eq 0 -and $hardwareHits -gt 0) {
    Write-Host "🎉 SUCCESS: I226 hardware timestamping at Gigabit!" -ForegroundColor Green
} elseif ($clockErrors -eq 0) {
    Write-Host "✅ IMPROVED: No clock errors at Gigabit" -ForegroundColor Yellow
} else {
    Write-Host "❌ STILL FAILING: Clock detection issues remain" -ForegroundColor Red
}

Write-Host ""
Write-Host "Key evidence:" -ForegroundColor White
Get-Content $logFile | Where-Object { 
    $_ -match "clock rate|Unable|timestamp.*hardware|timestamp.*software|Gigabit|1000.*Hz" 
} | Select-Object -First 5 | ForEach-Object { Write-Host "  $_" -ForegroundColor Gray }

Write-Host ""
Write-Host "Full log: $logFile" -ForegroundColor Gray
pause
