# Fixed gPTP Grandmaster Test
# Run gPTP in grandmaster mode with proper error handling

param(
    [int]$Duration = 30,
    [string]$AdapterName = "Intel"
)

Write-Host "=== gPTP Grandmaster Mode Test ===" -ForegroundColor Yellow
Write-Host "Duration: $Duration seconds" -ForegroundColor White
Write-Host "Target Adapter: $AdapterName" -ForegroundColor White

# Find Intel AVB-compatible adapter
$adapter = Get-NetAdapter | Where-Object { $_.InterfaceDescription -like "*Intel*" -and $_.Status -eq "Up" } | Select-Object -First 1

if (-not $adapter) {
    Write-Host "❌ No active Intel adapter found" -ForegroundColor Red
    exit 1
}

Write-Host "✅ Using adapter: $($adapter.InterfaceDescription)" -ForegroundColor Green
Write-Host "   MAC Address: $($adapter.MacAddress)" -ForegroundColor White

# Check if we're in the right directory for gPTP
$gptpPath = ""
if (Test-Path ".\windows\daemon_cl\gptp.exe") {
    $gptpPath = ".\windows\daemon_cl\gptp.exe"
} elseif (Test-Path "..\build\Debug\gptp.exe") {
    $gptpPath = "..\build\Debug\gptp.exe"
} elseif (Test-Path ".\build\Debug\gptp.exe") {
    $gptpPath = ".\build\Debug\gptp.exe"
} else {
    Write-Host "❌ gPTP executable not found" -ForegroundColor Red
    Write-Host "   Please build gPTP first or run from correct directory" -ForegroundColor Yellow
    exit 1
}

Write-Host "✅ Found gPTP: $gptpPath" -ForegroundColor Green

# Create grandmaster config
$configContent = @"
gPTP_cfg_enable=TRUE
logLevel=2
use_syslog=FALSE
clockQuality.priority1=248
clockQuality.clockClass=248
clockQuality.clockAccuracy=0xFE
clockQuality.offsetScaledLogVariance=0x436A
announce_interval=-3
sync_interval=-3
pDelay_interval=0
neighborPropDelayThresh=800
asCapable=TRUE
"@

$configFile = ".\grandmaster_test.ini"
$configContent | Out-File -FilePath $configFile -Encoding UTF8

Write-Host "✅ Created grandmaster config: $configFile" -ForegroundColor Green

# Start gPTP
Write-Host "`n=== Starting gPTP as Grandmaster ===" -ForegroundColor Green
$logFile = ".\grandmaster_test_$(Get-Date -Format 'yyyyMMdd_HHmmss').log"

try {
    $process = Start-Process -FilePath $gptpPath -ArgumentList "-F", $configFile -PassThru -RedirectStandardOutput $logFile -RedirectStandardError "${logFile}.err"
    
    Write-Host "✅ gPTP started (PID: $($process.Id))" -ForegroundColor Green
    Write-Host "⏰ Running for $Duration seconds..." -ForegroundColor Yellow
    
    Start-Sleep -Seconds $Duration
    
    # Stop the process
    if (-not $process.HasExited) {
        $process.Kill()
        Write-Host "✅ gPTP process stopped" -ForegroundColor Green
    }
    
    # Analyze results
    Write-Host "`n=== Analysis Results ===" -ForegroundColor Cyan
    
    if (Test-Path $logFile) {
        $logContent = Get-Content $logFile -ErrorAction SilentlyContinue
        
        if ($logContent) {
            $announceCount = ($logContent | Select-String "Announce" | Measure-Object).Count
            $syncCount = ($logContent | Select-String "Sync" | Measure-Object).Count
            $grandmasterLines = $logContent | Select-String "grandmaster|master|BMCA"
            
            Write-Host "📤 Announce messages: $announceCount" -ForegroundColor Green
            Write-Host "📤 Sync messages: $syncCount" -ForegroundColor Green
            
            if ($grandmasterLines) {
                Write-Host "`n🎯 Grandmaster Events:" -ForegroundColor Yellow
                $grandmasterLines | ForEach-Object { Write-Host "  $_" -ForegroundColor White }
            }
            
            Write-Host "`n📋 Last 5 log lines:" -ForegroundColor Yellow
            $logContent | Select-Object -Last 5 | ForEach-Object { Write-Host "  $_" -ForegroundColor Gray }
        } else {
            Write-Host "❌ No log content captured" -ForegroundColor Red
        }
    } else {
        Write-Host "❌ Log file not found: $logFile" -ForegroundColor Red
    }
    
} catch {
    Write-Host "❌ Error running gPTP: $($_.Exception.Message)" -ForegroundColor Red
}

Write-Host "`n✅ Test completed. Log saved to: $logFile" -ForegroundColor Green
