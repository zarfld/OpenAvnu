# ============================================================================
# ASIO Listener Debug and Monitoring Script
# ============================================================================

param(
    [switch]$ContinuousMonitoring,
    [switch]$DetailedLogging,
    [int]$MonitorDuration = 30
)

Write-Host ""
Write-Host "🔧 ASIO Listener Debug & Monitoring 🔧" -ForegroundColor Yellow
Write-Host "=======================================" -ForegroundColor Yellow
Write-Host ""

function Show-NetworkAdapterDetails {
    Write-Host "📡 Network Adapter Details:" -ForegroundColor Cyan
    $adapters = Get-NetAdapter | Where-Object { $_.InterfaceDescription -like '*Intel*' }
    
    foreach ($adapter in $adapters) {
        Write-Host "  Adapter: $($adapter.Name)" -ForegroundColor White
        Write-Host "    Description: $($adapter.InterfaceDescription)" -ForegroundColor Gray
        Write-Host "    Status: $($adapter.Status)" -ForegroundColor Gray
        Write-Host "    Speed: $($adapter.LinkSpeed)" -ForegroundColor Gray
        Write-Host "    MAC: $($adapter.MacAddress)" -ForegroundColor Gray
        
        # Get IP configuration
        $ipConfig = Get-NetIPAddress -InterfaceAlias $adapter.Name -ErrorAction SilentlyContinue | Where-Object { $_.AddressFamily -eq 'IPv4' }
        if ($ipConfig) {
            Write-Host "    IP: $($ipConfig.IPAddress)" -ForegroundColor Gray
        }
        Write-Host ""
    }
}

function Show-DaemonDetails {
    Write-Host "🔍 OpenAvnu Daemon Analysis:" -ForegroundColor Cyan
    
    $processes = Get-Process -Name "mrpd","maap_daemon" -ErrorAction SilentlyContinue
    
    if ($processes) {
        foreach ($process in $processes) {
            Write-Host "  Process: $($process.ProcessName)" -ForegroundColor White
            Write-Host "    PID: $($process.Id)" -ForegroundColor Gray
            Write-Host "    Memory: $([math]::Round($process.WorkingSet/1MB,2)) MB" -ForegroundColor Gray
            Write-Host "    CPU Time: $($process.CPU)" -ForegroundColor Gray
            Write-Host "    Start Time: $($process.StartTime)" -ForegroundColor Gray
            Write-Host "    Threads: $($process.Threads.Count)" -ForegroundColor Gray
            Write-Host ""
        }
    } else {
        Write-Host "  ❌ No OpenAvnu daemons running" -ForegroundColor Red
        Write-Host ""
    }
}

function Test-NetworkConnectivity {
    Write-Host "🌐 Network Connectivity Test:" -ForegroundColor Cyan
    
    # Test multicast capability
    Write-Host "  Testing multicast support..." -ForegroundColor Gray
    $adapter = Get-NetAdapter | Where-Object { $_.InterfaceDescription -like '*Intel*' -and $_.Status -eq 'Up' } | Select-Object -First 1
    
    if ($adapter) {
        Write-Host "    Using adapter: $($adapter.Name)" -ForegroundColor White
        
        # Check if adapter supports multicast
        $multicastSupport = Get-NetAdapterAdvancedProperty -Name $adapter.Name -DisplayName "*multicast*" -ErrorAction SilentlyContinue
        if ($multicastSupport) {
            Write-Host "    ✅ Multicast support detected" -ForegroundColor Green
        } else {
            Write-Host "    ⚠️  Multicast support unclear" -ForegroundColor Yellow
        }
    }
    Write-Host ""
}

function Monitor-ASIOPerformance {
    param([int]$Duration)
    
    Write-Host "📊 Starting $Duration second performance monitoring..." -ForegroundColor Cyan
    
    $startTime = Get-Date
    $samples = @()
    
    while ((Get-Date) -lt $startTime.AddSeconds($Duration)) {
        $timestamp = Get-Date -Format "HH:mm:ss"
        $processes = Get-Process -Name "mrpd","maap_daemon","asio_listener" -ErrorAction SilentlyContinue
        
        foreach ($process in $processes) {
            $sample = [PSCustomObject]@{
                Time = $timestamp
                Process = $process.ProcessName
                PID = $process.Id
                CPU = $process.CPU
                Memory_MB = [math]::Round($process.WorkingSet/1MB,2)
                Threads = $process.Threads.Count
            }
            $samples += $sample
        }
        
        # Show current status
        Write-Host "  [$timestamp] Monitoring: $($processes.Count) processes" -ForegroundColor Gray
        Start-Sleep -Seconds 1
    }
    
    Write-Host ""
    Write-Host "📈 Performance Summary:" -ForegroundColor Cyan
    $samples | Group-Object Process | ForEach-Object {
        $processName = $_.Name
        $avgMemory = ($_.Group | Measure-Object Memory_MB -Average).Average
        $maxMemory = ($_.Group | Measure-Object Memory_MB -Maximum).Maximum
        $avgThreads = ($_.Group | Measure-Object Threads -Average).Average
        
        Write-Host "  $processName:" -ForegroundColor White
        Write-Host "    Avg Memory: $([math]::Round($avgMemory,2)) MB" -ForegroundColor Gray
        Write-Host "    Max Memory: $([math]::Round($maxMemory,2)) MB" -ForegroundColor Gray
        Write-Host "    Avg Threads: $([math]::Round($avgThreads,1))" -ForegroundColor Gray
    }
    Write-Host ""
}

# Main execution
Show-NetworkAdapterDetails
Show-DaemonDetails
Test-NetworkConnectivity

# Check if daemons are running, if not, start them
$runningDaemons = Get-Process -Name "mrpd","maap_daemon" -ErrorAction SilentlyContinue
if (-not $runningDaemons) {
    Write-Host "🚀 Starting OpenAvnu daemons..." -ForegroundColor Yellow
    & ".\start_openavnu_final.ps1"
    Start-Sleep -Seconds 3
    Show-DaemonDetails
}

# Performance monitoring
if ($ContinuousMonitoring) {
    Write-Host "🔄 Starting continuous monitoring (Ctrl+C to stop)..." -ForegroundColor Yellow
    while ($true) {
        Monitor-ASIOPerformance -Duration 10
        Start-Sleep -Seconds 2
    }
} else {
    Monitor-ASIOPerformance -Duration $MonitorDuration
}

# ASIO-specific tests
Write-Host "🎵 ASIO Listener Specific Tests:" -ForegroundColor Cyan

# Check if ASIO listener exists
$asioPath = "examples\asio-listener\asio_listener.exe"
if (Test-Path $asioPath) {
    Write-Host "  ✅ ASIO Listener found: $asioPath" -ForegroundColor Green
    
    # Get file details
    $fileInfo = Get-Item $asioPath
    Write-Host "    Size: $([math]::Round($fileInfo.Length/1KB,2)) KB" -ForegroundColor Gray
    Write-Host "    Modified: $($fileInfo.LastWriteTime)" -ForegroundColor Gray
    
    if ($DetailedLogging) {
        Write-Host "  🔍 Running ASIO Listener with detailed logging..." -ForegroundColor Yellow
        Set-Location "examples\asio-listener"
        Write-Host "    Starting in 3 seconds (Ctrl+C to stop)..." -ForegroundColor Gray
        Start-Sleep -Seconds 3
        
        try {
            & ".\asio_listener.exe" --verbose
        } catch {
            Write-Host "    ⚠️  ASIO Listener stopped: $($_.Exception.Message)" -ForegroundColor Yellow
        } finally {
            Set-Location "..\..\"
        }
    }
} else {
    Write-Host "  ❌ ASIO Listener not found: $asioPath" -ForegroundColor Red
    Write-Host "  💡 Build it first using VS Code task or build.bat" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "🔧 Debug session complete! 🔧" -ForegroundColor Yellow
