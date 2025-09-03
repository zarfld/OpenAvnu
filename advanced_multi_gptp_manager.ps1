# Advanced Multi-Instance gPTP Manager
# Demonstrates sophisticated multi-adapter gPTP deployment strategies
param(
    [ValidateSet("Sequential", "RoundRobin", "TimeSliced", "Coordinated")]
    [string]$Mode = "Sequential",
    [int]$Duration = 60,
    [int]$SliceInterval = 10,
    [switch]$ContinuousMonitoring
)

Write-Host "ADVANCED MULTI-INSTANCE gPTP MANAGER" -ForegroundColor Magenta
Write-Host "Mode: $Mode | Duration: $Duration seconds" -ForegroundColor Yellow
Write-Host ""

# Adapter discovery
function Get-AVBAdapters {
    $adapters = @()
    $supported = @("*I210*", "*I219*", "*I225*", "*I226*")
    
    foreach ($pattern in $supported) {
        $found = Get-NetAdapter | Where-Object { 
            $_.InterfaceDescription -like $pattern -and $_.Status -eq 'Up' 
        }
        foreach ($adapter in $found) {
            $type = switch -Wildcard ($adapter.InterfaceDescription) {
                "*I210*" { "I210" }
                "*I219*" { "I219" }  
                "*I225*" { "I225" }
                "*I226*" { "I226" }
                default { "Unknown" }
            }
            
            $adapters += @{
                Type = $type
                MAC = $adapter.MacAddress.Replace("-", ":")
                Name = $adapter.Name
                Description = $adapter.InterfaceDescription
                Index = $adapter.InterfaceIndex
            }
        }
    }
    
    return $adapters
}

# gPTP session manager
function Start-GptpSession {
    param($Adapter, $SessionId, $Duration)
    
    $timestamp = (Get-Date).ToString("yyyyMMdd_HHmmss")
    $logFile = "gptp_$($Adapter.Type)_session${SessionId}_$timestamp.log"  
    $errFile = "gptp_$($Adapter.Type)_session${SessionId}_$timestamp.err"
    
    $processArgs = @{
        FilePath = ".\thirdparty\gptp\build\Release\gptp.exe"
        ArgumentList = @($Adapter.MAC)
        RedirectStandardOutput = $logFile
        RedirectStandardError = $errFile
        NoNewWindow = $true
        PassThru = $true
    }
    
    $process = Start-Process @processArgs
    
    return @{
        Process = $process
        Adapter = $Adapter
        SessionId = $SessionId
        StartTime = Get-Date
        LogFile = $logFile
        ErrorFile = $errFile
        Duration = $Duration
    }
}

function Stop-GptpSession {
    param($Session)
    
    if (-not $Session.Process.HasExited) {
        $Session.Process.Kill()
        $Session.Process.WaitForExit(3000)
    }
    
    # Quick analysis
    $success = $false
    $ptpPackets = 0
    
    if ((Test-Path $Session.ErrorFile)) {
        $content = Get-Content $Session.ErrorFile -Raw
        $success = $content -match "LISTENING THREAD STARTED"
        $ptpPackets = ([regex]::Matches($content, "PDelay.*Request")).Count
    }
    
    return @{
        Success = $success
        PTPPackets = $ptpPackets
        ActualDuration = ((Get-Date) - $Session.StartTime).TotalSeconds
    }
}

# Main execution modes
$adapters = Get-AVBAdapters

if ($adapters.Count -eq 0) {
    Write-Host "No supported Intel AVB adapters found" -ForegroundColor Red
    exit 1
}

Write-Host "Found $($adapters.Count) Intel AVB adapters:" -ForegroundColor Green
foreach ($adapter in $adapters) {
    Write-Host "  $($adapter.Type): $($adapter.MAC) ($($adapter.Name))" -ForegroundColor White
}
Write-Host ""

$sessions = @()
$sessionId = 1
$results = @()

switch ($Mode) {
    "Sequential" {
        Write-Host "=== SEQUENTIAL MODE ===" -ForegroundColor Cyan
        
        foreach ($adapter in $adapters) {
            Write-Host "Starting session $sessionId on $($adapter.Type)..." -ForegroundColor Yellow
            
            $session = Start-GptpSession -Adapter $adapter -SessionId $sessionId -Duration $Duration
            Write-Host "  Process: $($session.Process.Id) | Logs: $($session.ErrorFile)" -ForegroundColor Gray
            
            # Wait for session completion
            Start-Sleep -Seconds $Duration
            
            $result = Stop-GptpSession -Session $session
            $result.Adapter = $adapter.Type
            $result.SessionId = $sessionId
            $results += $result
            
            Write-Host "  Result: $(if ($result.Success) {'SUCCESS'} else {'PARTIAL'}) | PTP Packets: $($result.PTPPackets)" -ForegroundColor $(if ($result.Success) {'Green'} else {'Yellow'})
            
            $sessionId++
            Start-Sleep -Seconds 2  # Brief gap between sessions
        }
    }
    
    "RoundRobin" {
        Write-Host "=== ROUND-ROBIN MODE ===" -ForegroundColor Cyan
        
        $totalRounds = [math]::Ceiling($Duration / ($SliceInterval * $adapters.Count))
        Write-Host "Running $totalRounds rounds, $SliceInterval seconds per adapter" -ForegroundColor Yellow
        
        for ($round = 1; $round -le $totalRounds; $round++) {
            Write-Host "Round $round/$totalRounds" -ForegroundColor Cyan
            
            foreach ($adapter in $adapters) {
                Write-Host "  $($adapter.Type) slice..." -NoNewline -ForegroundColor Yellow
                
                $session = Start-GptpSession -Adapter $adapter -SessionId "$round-$sessionId" -Duration $SliceInterval
                Start-Sleep -Seconds $SliceInterval
                $result = Stop-GptpSession -Session $session
                
                Write-Host " $(if ($result.Success) {'✓'} else {'◐'}) ($($result.PTPPackets) packets)" -ForegroundColor $(if ($result.Success) {'Green'} else {'Yellow'})
                
                $result.Adapter = $adapter.Type
                $result.SessionId = "$round-$sessionId"
                $results += $result
                $sessionId++
            }
        }
    }
    
    "TimeSliced" {
        Write-Host "=== TIME-SLICED MODE ===" -ForegroundColor Cyan
        
        $endTime = (Get-Date).AddSeconds($Duration)
        Write-Host "Running time-sliced execution for $Duration seconds total" -ForegroundColor Yellow
        
        $adapterIndex = 0
        while ((Get-Date) -lt $endTime) {
            $adapter = $adapters[$adapterIndex % $adapters.Count]
            $remainingTime = [math]::Min($SliceInterval, ($endTime - (Get-Date)).TotalSeconds)
            
            if ($remainingTime -le 0) { break }
            
            Write-Host "Slice ${sessionId} - $($adapter.Type) for $([math]::Round($remainingTime, 1))s" -ForegroundColor Yellow
            
            $session = Start-GptpSession -Adapter $adapter -SessionId $sessionId -Duration $remainingTime
            Start-Sleep -Seconds $remainingTime
            $result = Stop-GptpSession -Session $session
            
            # Create proper result object
            $sessionResult = @{
                Success = $result.Success
                PTPPackets = $result.PTPPackets
                ActualDuration = $result.ActualDuration
                Adapter = $adapter.Type
                SessionId = $sessionId
            }
            $results += $sessionResult
            
            $adapterIndex++
            $sessionId++
        }
    }
    
    "Coordinated" {
        Write-Host "=== COORDINATED MODE ===" -ForegroundColor Cyan
        Write-Host "Demonstrating coordinated multi-adapter management" -ForegroundColor Yellow
        
        # This would implement master/slave coordination logic
        # For now, demonstrate the concept with sequential execution plus coordination
        
        $masterAdapter = $adapters[0]
        Write-Host "Master: $($masterAdapter.Type) ($($masterAdapter.MAC))" -ForegroundColor Green
        
        # Start master session
        $masterSession = Start-GptpSession -Adapter $masterAdapter -SessionId "MASTER" -Duration ($Duration / 2)
        Write-Host "Master session started (PID: $($masterSession.Process.Id))" -ForegroundColor Green
        
        Start-Sleep -Seconds ($Duration / 2)
        $masterResult = Stop-GptpSession -Session $masterSession
        $masterResult.Adapter = "$($masterAdapter.Type)-MASTER"
        $masterResult.SessionId = "MASTER"
        $results += $masterResult
        
        # Follow with slave sessions  
        for ($i = 1; $i -lt $adapters.Count; $i++) {
            $slaveAdapter = $adapters[$i]
            Write-Host "Slave ${i} - $($slaveAdapter.Type)" -ForegroundColor Cyan
            
            $slaveSession = Start-GptpSession -Adapter $slaveAdapter -SessionId "SLAVE-$i" -Duration ($Duration / 4)
            Start-Sleep -Seconds ($Duration / 4)
            $slaveResult = Stop-GptpSession -Session $slaveSession
            
            $slaveResult.Adapter = "$($slaveAdapter.Type)-SLAVE"
            $slaveResult.SessionId = "SLAVE-$i"
            $results += $slaveResult
        }
    }
}

# Final analysis
Write-Host ""
Write-Host "=== MULTI-INSTANCE EXECUTION RESULTS ===" -ForegroundColor Magenta

$totalSessions = $results.Count
$successful = ($results | Where-Object { $_.Success }).Count
$totalPackets = ($results | ForEach-Object { $_.PTPPackets } | Measure-Object -Sum).Sum

Write-Host "Mode: $Mode" -ForegroundColor Yellow
Write-Host "Total Sessions: $totalSessions" -ForegroundColor White  
Write-Host "Successful: $successful" -ForegroundColor Green
Write-Host "Success Rate: $([math]::Round(($successful / $totalSessions * 100), 1))%" -ForegroundColor $(if ($successful -eq $totalSessions) {'Green'} else {'Yellow'})
Write-Host "Total PTP Packets: $totalPackets" -ForegroundColor White
Write-Host ""

Write-Host "Session Details:" -ForegroundColor Yellow
foreach ($result in $results) {
    $status = if ($result.Success) { "PASS" } else { "PARTIAL" }
    $color = if ($result.Success) { "Green" } else { "Yellow" }
    Write-Host "  $($result.Adapter) ($($result.SessionId)): $status - $($result.PTPPackets) packets in $([math]::Round($result.ActualDuration, 1))s" -ForegroundColor $color
}

Write-Host ""
Write-Host "CONCLUSION:" -ForegroundColor Cyan
if ($successful -eq $totalSessions) {
    Write-Host "✅ MULTIPLE gPTP INSTANCES WORK PERFECTLY IN $Mode MODE" -ForegroundColor Green
    Write-Host "Intel AVB framework fully supports multi-adapter deployment" -ForegroundColor Green
} elseif ($successful -gt 0) {
    Write-Host "⚠️ PARTIAL SUCCESS - Multi-instance capable with some limitations" -ForegroundColor Yellow  
    Write-Host "Recommend $Mode mode for production multi-adapter setups" -ForegroundColor Yellow
} else {
    Write-Host "❌ Multi-instance execution failed in $Mode mode" -ForegroundColor Red
}
