# Sequential Multi-gPTP Instance Testing
# Demonstrates running multiple gPTP instances in time-multiplexed mode
param(
    [int]$SessionDuration = 10,
    [switch]$DetailedLogging
)

Write-Host "SEQUENTIAL MULTI-gPTP TESTING" -ForegroundColor Magenta
Write-Host "Session Duration: $SessionDuration seconds per adapter" -ForegroundColor Yellow
Write-Host ""

# Find available adapters
$adapters = @()
$i210 = Get-NetAdapter | Where-Object { $_.InterfaceDescription -like "*I210*" -and $_.Status -eq 'Up' } | Select-Object -First 1
$i226 = Get-NetAdapter | Where-Object { $_.InterfaceDescription -like "*I226*" -and $_.Status -eq 'Up' } | Select-Object -First 1

if ($i210) { $adapters += @{Name="I210"; MAC=$i210.MacAddress.Replace("-",":")} }
if ($i226) { $adapters += @{Name="I226"; MAC=$i226.MacAddress.Replace("-",":")} }

if ($adapters.Count -eq 0) {
    Write-Host "No supported adapters found" -ForegroundColor Red
    exit 1
}

Write-Host "Found $($adapters.Count) adapters for sequential testing:" -ForegroundColor Green
foreach ($adapter in $adapters) {
    Write-Host "  $($adapter.Name): $($adapter.MAC)" -ForegroundColor White
}
Write-Host ""

# Check for gPTP executable
$gptpPath = ".\thirdparty\gptp\build\Release\gptp.exe"
if (-not (Test-Path $gptpPath)) {
    Write-Host "gPTP executable not found at $gptpPath" -ForegroundColor Red
    exit 1
}

# Sequential execution of gPTP instances
$results = @()
$sessionId = 1

foreach ($adapter in $adapters) {
    Write-Host "=== SESSION ${sessionId}: $($adapter.Name) ===" -ForegroundColor Cyan
    
    # Generate unique log files
    $timestamp = (Get-Date).ToString("yyyyMMdd_HHmmss")
    $logFile = "gptp_$($adapter.Name.ToLower())_sequential_$timestamp.log"
    $errFile = "gptp_$($adapter.Name.ToLower())_sequential_$timestamp.err"
    
    Write-Host "Starting gPTP on $($adapter.Name) ($($adapter.MAC))" -ForegroundColor Yellow
    Write-Host "Logs: $logFile / $errFile" -ForegroundColor Gray
    
    # Start gPTP process with logging
    $processArgs = @{
        FilePath = $gptpPath
        ArgumentList = @($adapter.MAC)
        RedirectStandardOutput = $logFile
        RedirectStandardError = $errFile
        NoNewWindow = $true
        PassThru = $true
    }
    
    $process = Start-Process @processArgs
    Write-Host "Process started (PID: $($process.Id))" -ForegroundColor Green
    
    # Monitor for specified duration
    $startTime = Get-Date
    $endTime = $startTime.AddSeconds($SessionDuration)
    $heartbeatCount = 0
    
    while ((Get-Date) -lt $endTime -and -not $process.HasExited) {
        Start-Sleep -Seconds 2
        $heartbeatCount++
        
        if ($DetailedLogging) {
            $elapsed = [math]::Round(((Get-Date) - $startTime).TotalSeconds, 1)
            Write-Host "  [$elapsed s] Process alive (PID: $($process.Id))" -ForegroundColor Gray
        } else {
            Write-Host "." -NoNewline -ForegroundColor Green
        }
    }
    
    if (-not $DetailedLogging) { Write-Host "" } # New line after dots
    
    # Stop the process
    if (-not $process.HasExited) {
        Write-Host "Stopping gPTP process..." -ForegroundColor Yellow
        $process.Kill()
        $process.WaitForExit(5000)
    }
    
    # Analyze results
    $logExists = Test-Path $logFile
    $errExists = Test-Path $errFile
    $logSize = if ($logExists) { (Get-Item $logFile).Length } else { 0 }
    $errSize = if ($errExists) { (Get-Item $errFile).Length } else { 0 }
    
    # Check for key success indicators
    $daemonStarted = $false
    $ptpPackets = 0
    $pdelayRequests = 0
    
    if ($errExists -and $errSize -gt 0) {
        $errorContent = Get-Content $errFile -Raw
        if ($errorContent -match "LISTENING THREAD STARTED") {
            $daemonStarted = $true
        }
        $ptpMatches = [regex]::Matches($errorContent, "PDelay.*Request")
        $ptpPackets = $ptpMatches.Count
        $pdelayMatches = [regex]::Matches($errorContent, "PDelay Request seq=(\d+)")
        $pdelayRequests = $pdelayMatches.Count
    }
    
    $result = @{
        Session = $sessionId
        Adapter = $adapter.Name
        MAC = $adapter.MAC
        ProcessId = $process.Id
        Duration = $SessionDuration
        DaemonStarted = $daemonStarted
        PTPPackets = $ptpPackets
        PDelayRequests = $pdelayRequests
        LogFile = $logFile
        ErrorFile = $errFile
        LogSize = $logSize
        ErrorSize = $errSize
        Success = ($daemonStarted -and ($ptpPackets -gt 0 -or $pdelayRequests -gt 0))
    }
    
    $results += $result
    
    Write-Host "Session Results:" -ForegroundColor Yellow
    Write-Host "  Daemon Started: $(if ($result.DaemonStarted) {'YES'} else {'NO'})" -ForegroundColor $(if ($result.DaemonStarted) {'Green'} else {'Red'})
    Write-Host "  PTP Packets: $($result.PTPPackets)" -ForegroundColor White
    Write-Host "  PDelay Requests: $($result.PDelayRequests)" -ForegroundColor White
    Write-Host "  Overall Success: $(if ($result.Success) {'PASS'} else {'PARTIAL'})" -ForegroundColor $(if ($result.Success) {'Green'} else {'Yellow'})
    Write-Host ""
    
    $sessionId++
    
    # Brief pause between sessions to ensure clean separation
    Start-Sleep -Seconds 2
}

# Final summary
Write-Host "=== SEQUENTIAL MULTI-gPTP SUMMARY ===" -ForegroundColor Magenta
Write-Host ""

$totalSessions = $results.Count
$successfulSessions = ($results | Where-Object { $_.Success }).Count
$partialSessions = ($results | Where-Object { $_.DaemonStarted -and -not $_.Success }).Count

Write-Host "Total Sessions: $totalSessions" -ForegroundColor White
Write-Host "Successful: $successfulSessions" -ForegroundColor Green  
Write-Host "Partial: $partialSessions" -ForegroundColor Yellow
Write-Host "Failed: $($totalSessions - $successfulSessions - $partialSessions)" -ForegroundColor Red
Write-Host ""

Write-Host "Individual Results:" -ForegroundColor Yellow
foreach ($result in $results) {
    $status = if ($result.Success) { "PASS" } elseif ($result.DaemonStarted) { "PARTIAL" } else { "FAIL" }
    $statusColor = if ($result.Success) { "Green" } elseif ($result.DaemonStarted) { "Yellow" } else { "Red" }
    
    Write-Host "  $($result.Adapter): $status (PTP: $($result.PTPPackets), PDelay: $($result.PDelayRequests))" -ForegroundColor $statusColor
}
Write-Host ""

Write-Host "CONCLUSION: Sequential Multi-Instance gPTP" -ForegroundColor Cyan
if ($successfulSessions -eq $totalSessions) {
    Write-Host "✅ FULL SUCCESS - All adapters work sequentially" -ForegroundColor Green
} elseif ($successfulSessions -gt 0) {
    Write-Host "⚠️ PARTIAL SUCCESS - Some adapters working" -ForegroundColor Yellow
} else {
    Write-Host "❌ FAILURE - No adapters working in sequential mode" -ForegroundColor Red
}

Write-Host ""
Write-Host "Log Files Available:" -ForegroundColor Gray
foreach ($result in $results) {
    Write-Host "  $($result.Adapter): $($result.ErrorFile)" -ForegroundColor White
}
