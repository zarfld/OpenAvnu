# gPTP Grandmaster Simple Test (ASCII-safe)
# Usage: powershell -ExecutionPolicy Bypass -File .\test_gptp_grandmaster_simple.ps1 -Duration 20 -AdapterPattern Intel
param(
  [int]$Duration = 20,
  [string]$AdapterPattern = "Intel"
)
Write-Host "=== gPTP Grandmaster Simple Test ==="
Write-Host "Duration: $Duration"
Write-Host "Adapter Pattern: *$AdapterPattern*"

# 1. Find adapter
$adapter = Get-NetAdapter | Where-Object { $_.InterfaceDescription -like ("*${AdapterPattern}*") -and $_.Status -eq 'Up' } | Select-Object -First 1
if (-not $adapter) { Write-Host "ERROR: No active adapter matching *$AdapterPattern*" -ForegroundColor Red; exit 2 }
Write-Host "Adapter: $($adapter.InterfaceDescription)  MAC=$($adapter.MacAddress)" -ForegroundColor Green

# 2. Locate gptp.exe
$candidates = @(
  "thirdparty\gptp\windows\daemon_cl\gptp.exe",
  "thirdparty\gptp\build\Debug\gptp.exe",
  "thirdparty\gptp\build\Release\gptp.exe",
  "windows\daemon_cl\gptp.exe",
  "build\Debug\gptp.exe",
  "build\Release\gptp.exe"
)
$gptp = $candidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $gptp) {
  Write-Host "ERROR: gptp.exe not found." -ForegroundColor Red
  Write-Host "Build suggestion:" -ForegroundColor Yellow
  Write-Host "  pushd thirdparty\gptp"
  Write-Host "  mkdir build; cd build"
  Write-Host "  cmake .. -G 'Visual Studio 17 2022'"
  Write-Host "  cmake --build . --config Release"
  Write-Host "  popd"
  exit 3
}
Write-Host "Found: $gptp" -ForegroundColor Green

# 3. Config file
$cfg = "grandmaster_test.ini"
@"
gPTP_cfg_enable=TRUE
logLevel=2
use_syslog=FALSE
gmCapable=TRUE
priority1=128
clockQuality.clockClass=248
clockQuality.clockAccuracy=0xFE
clockQuality.offsetScaledLogVariance=0x436A
announce_interval=-3
sync_interval=-3
pDelay_interval=0
neighborPropDelayThresh=800
asCapable=TRUE
"@ | Out-File -FilePath $cfg -Encoding ASCII
Write-Host "Config: $cfg" -ForegroundColor Green

# 4. Launch
$ts = Get-Date -Format 'yyyyMMdd_HHmmss'
$log = "gptp_${ts}.log"
$err = "gptp_${ts}.err"
$mac = $adapter.MacAddress  # Keep Windows format XX-XX-XX-XX-XX-XX
Write-Host "Starting gptp (Duration=$Duration)" -ForegroundColor Cyan
try { $p = Start-Process -FilePath $gptp -ArgumentList "-F", $cfg, $mac -PassThru -RedirectStandardOutput $log -RedirectStandardError $err } catch { Write-Host "ERROR: Start failed: $($_.Exception.Message)" -ForegroundColor Red; exit 4 }
Start-Sleep -Seconds $Duration
if ($p -and -not $p.HasExited) { try { $p.Kill() | Out-Null } catch {} }

# 5. Analyze
if (Test-Path $log) {
  $c = Get-Content $log -ErrorAction SilentlyContinue
  if ($c) {
    $announce = ($c | Select-String -SimpleMatch "Announce" | Measure-Object).Count
    $sync = ($c | Select-String -SimpleMatch "Sync" | Measure-Object).Count
    $gm = ($c | Select-String -Pattern "grandmaster|master|BMCA" | Measure-Object).Count
    Write-Host "Announce: $announce"
    Write-Host "Sync:     $sync"
    Write-Host "GM/BMCA:  $gm"
    Write-Host "Last 5 lines:" -ForegroundColor Yellow
    $c | Select-Object -Last 5 | ForEach-Object { Write-Host "  $_" }
  } else { Write-Host "WARNING: Log empty" -ForegroundColor Yellow }
} else { Write-Host "ERROR: Missing log $log" -ForegroundColor Red }

if (Test-Path $err) {
  $esize = (Get-Item $err).Length
  if ($esize -gt 0) {
    Write-Host "stderr (first 5 lines):" -ForegroundColor Yellow
    Get-Content $err | Select-Object -First 5 | ForEach-Object { Write-Host "  $_" }
  }
}

Write-Host "Done. Logs: $log / $err"
exit 0
