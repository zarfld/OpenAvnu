param(
    [int]$Duration = 30,
    [switch]$DetailedLogging
)

# Test both Intel adapters in parallel
Write-Host "=========================================" -ForegroundColor Magenta
Write-Host "PARALLEL DUAL ADAPTER gPTP TESTING" -ForegroundColor Green
Write-Host "=========================================" -ForegroundColor Magenta
Write-Host ""

# Get both adapters
$i210 = Get-NetAdapter | Where-Object { $_.InterfaceDescription -like "*I210*" -and $_.Status -eq 'Up' } | Select-Object -First 1
$i226 = Get-NetAdapter | Where-Object { $_.InterfaceDescription -like "*I226*" -and $_.Status -eq 'Up' } | Select-Object -First 1

if (-not $i210) {
    Write-Host "❌ I210 adapter not found or not connected" -ForegroundColor Red
    exit 1
}

if (-not $i226) {
    Write-Host "❌ I226 adapter not found or not connected" -ForegroundColor Red
    exit 1
}

Write-Host "🔍 Detected Adapters:" -ForegroundColor Cyan
Write-Host "   I210: $($i210.InterfaceDescription) MAC=$($i210.MacAddress)" -ForegroundColor White
Write-Host "   I226: $($i226.InterfaceDescription) MAC=$($i226.MacAddress)" -ForegroundColor White
Write-Host ""

# Check gPTP executable
$gptpExe = ".\thirdparty\gptp\build\Release\gptp.exe"
if (-not (Test-Path $gptpExe)) {
    Write-Host "❌ gPTP executable not found: $gptpExe" -ForegroundColor Red
    exit 1
}

Write-Host "✅ gPTP executable found: $gptpExe" -ForegroundColor Green
Write-Host ""

# Cleanup any existing processes
Get-Process | Where-Object { $_.ProcessName -like "*gptp*" } | Stop-Process -Force -ErrorAction SilentlyContinue

# Clean old log files
Remove-Item "gptp_i210_*.log" -ErrorAction SilentlyContinue
Remove-Item "gptp_i210_*.err" -ErrorAction SilentlyContinue
Remove-Item "gptp_i226_*.log" -ErrorAction SilentlyContinue
Remove-Item "gptp_i226_*.err" -ErrorAction SilentlyContinue

$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"

Write-Host "🚀 Starting parallel gPTP instances..." -ForegroundColor Yellow
Write-Host ""

# Start I210 instance
Write-Host "Starting I210 gPTP daemon..." -ForegroundColor Cyan
$i210Process = Start-Process -FilePath $gptpExe -ArgumentList @($i210.MacAddress) `
    -RedirectStandardOutput "gptp_i210_${timestamp}.log" `
    -RedirectStandardError "gptp_i210_${timestamp}.err" `
    -PassThru -NoNewWindow

Start-Sleep -Seconds 2

# Start I226 instance  
Write-Host "Starting I226 gPTP daemon..." -ForegroundColor Cyan
$i226Process = Start-Process -FilePath $gptpExe -ArgumentList @($i226.MacAddress) `
    -RedirectStandardOutput "gptp_i226_${timestamp}.log" `
    -RedirectStandardError "gptp_i226_${timestamp}.err" `
    -PassThru -NoNewWindow

Write-Host ""
Write-Host "⏱️  Running parallel test for $Duration seconds..." -ForegroundColor Yellow
Write-Host ""

# Monitor both processes
$startTime = Get-Date
$endTime = $startTime.AddSeconds($Duration)

$i210Status = "Starting..."
$i226Status = "Starting..."
$iteration = 0

while ((Get-Date) -lt $endTime) {
    $iteration++
    
    # Check if processes are still running
    $i210Running = -not $i210Process.HasExited
    $i226Running = -not $i226Process.HasExited
    
    if ($iteration % 5 -eq 0 -or $DetailedLogging) {
        Write-Host "📊 Status Update ($(((Get-Date) - $startTime).TotalSeconds.ToString('F1'))s elapsed):" -ForegroundColor Magenta
        
        # I210 Status
        if ($i210Running) {
            $i210Status = "✅ Running (PID: $($i210Process.Id))"
        } else {
            $i210Status = "❌ Stopped (Exit: $($i210Process.ExitCode))"
        }
        
        # I226 Status  
        if ($i226Running) {
            $i226Status = "✅ Running (PID: $($i226Process.Id))"
        } else {
            $i226Status = "❌ Stopped (Exit: $($i226Process.ExitCode))"
        }
        
        Write-Host "   I210: $i210Status" -ForegroundColor White
        Write-Host "   I226: $i226Status" -ForegroundColor White
        
        # Quick log analysis
        if (Test-Path "gptp_i210_${timestamp}.err") {
            $i210Errors = (Get-Content "gptp_i210_${timestamp}.err" -ErrorAction SilentlyContinue | Measure-Object).Count
            if ($i210Errors -gt 0) {
                $lastError = Get-Content "gptp_i210_${timestamp}.err" -Tail 1 -ErrorAction SilentlyContinue
                Write-Host "   I210 Errors: $i210Errors (Last: $($lastError.Substring(0, [Math]::Min($lastError.Length, 80))))" -ForegroundColor Yellow
            }
        }
        
        if (Test-Path "gptp_i226_${timestamp}.err") {
            $i226Errors = (Get-Content "gptp_i226_${timestamp}.err" -ErrorAction SilentlyContinue | Measure-Object).Count
            if ($i226Errors -gt 0) {
                $lastError = Get-Content "gptp_i226_${timestamp}.err" -Tail 1 -ErrorAction SilentlyContinue
                Write-Host "   I226 Errors: $i226Errors (Last: $($lastError.Substring(0, [Math]::Min($lastError.Length, 80))))" -ForegroundColor Yellow
            }
        }
        
        Write-Host ""
    }
    
    # Exit early if both processes died
    if (-not $i210Running -and -not $i226Running) {
        Write-Host "⚠️  Both processes stopped early, ending test..." -ForegroundColor Yellow
        break
    }
    
    Start-Sleep -Seconds 1
}

Write-Host "🛑 Stopping gPTP instances..." -ForegroundColor Red

# Stop processes gracefully
if (-not $i210Process.HasExited) {
    $i210Process.Kill()
    Write-Host "   I210 process terminated" -ForegroundColor Gray
}

if (-not $i226Process.HasExited) {
    $i226Process.Kill()  
    Write-Host "   I226 process terminated" -ForegroundColor Gray
}

Write-Host ""
Write-Host "📋 PARALLEL TEST RESULTS:" -ForegroundColor Green -BackgroundColor Black
Write-Host "=========================================" -ForegroundColor Magenta

# Analyze I210 results
Write-Host ""
Write-Host "🔍 I210 Analysis:" -ForegroundColor Cyan
if (Test-Path "gptp_i210_${timestamp}.log") {
    $i210LogContent = Get-Content "gptp_i210_${timestamp}.log" -ErrorAction SilentlyContinue
    $i210Started = $i210LogContent | Where-Object { $_ -like "*LISTENING THREAD STARTED*" }
    $i210PtpPackets = $i210LogContent | Where-Object { $_ -like "*PTP PACKET*DETECTED*" }
    $i210PdelayReq = $i210LogContent | Where-Object { $_ -like "*PDELAY_REQ*" }
    
    Write-Host "   Daemon Started: $(if($i210Started) { '✅ YES' } else { '❌ NO' })" -ForegroundColor White
    Write-Host "   PTP Packets: $($i210PtpPackets.Count)" -ForegroundColor White
    Write-Host "   PDelay Requests: $($i210PdelayReq.Count)" -ForegroundColor White
} else {
    Write-Host "   ❌ No log file generated" -ForegroundColor Red
}

# Analyze I226 results
Write-Host ""
Write-Host "🔍 I226 Analysis:" -ForegroundColor Cyan
if (Test-Path "gptp_i226_${timestamp}.log") {
    $i226LogContent = Get-Content "gptp_i226_${timestamp}.log" -ErrorAction SilentlyContinue
    $i226Started = $i226LogContent | Where-Object { $_ -like "*LISTENING THREAD STARTED*" }
    $i226PtpPackets = $i226LogContent | Where-Object { $_ -like "*PTP PACKET*DETECTED*" }
    $i226PdelayReq = $i226LogContent | Where-Object { $_ -like "*PDELAY_REQ*" }
    
    Write-Host "   Daemon Started: $(if($i226Started) { '✅ YES' } else { '❌ NO' })" -ForegroundColor White
    Write-Host "   PTP Packets: $($i226PtpPackets.Count)" -ForegroundColor White
    Write-Host "   PDelay Requests: $($i226PdelayReq.Count)" -ForegroundColor White
} else {
    Write-Host "   ❌ No log file generated" -ForegroundColor Red
}

Write-Host ""
Write-Host "📁 Log files generated:" -ForegroundColor Gray
Write-Host "   gptp_i210_${timestamp}.log / .err" -ForegroundColor Gray  
Write-Host "   gptp_i226_${timestamp}.log / .err" -ForegroundColor Gray
Write-Host ""

# Final verdict
if ($i210Started -and $i226Started) {
    Write-Host "🎯 PARALLEL TEST: SUCCESS ✅" -ForegroundColor Green -BackgroundColor Black
    Write-Host "   Both I210 and I226 adapters running simultaneously!" -ForegroundColor Green
} else {
    Write-Host "⚠️  PARALLEL TEST: PARTIAL SUCCESS" -ForegroundColor Yellow -BackgroundColor Black
    Write-Host "   Check individual adapter logs for details" -ForegroundColor Yellow
}

Write-Host ""
