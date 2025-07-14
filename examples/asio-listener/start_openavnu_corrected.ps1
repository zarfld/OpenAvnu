Write-Host "=== OpenAvnu ASIO Startup with Corrected MAAP ===" -ForegroundColor Cyan
Write-Host ""

$buildDir = "d:\Repos\OpenAvnu\build"
$mrpdPath = "$buildDir\daemons\mrpd\Release\mrpd.exe"
$maapPath = "$buildDir\daemons\maap\Release\maap_daemon.exe"

# Step 1: Get Intel I219 interface details
Write-Host "Step 1: Detecting Intel I219 interface..." -ForegroundColor Yellow
$i219Adapter = Get-NetAdapter | Where-Object { 
    $_.InterfaceDescription -like "*Intel*" -and 
    $_.InterfaceDescription -like "*I219*" -and
    $_.Status -eq "Up"
} | Select-Object -First 1

if (-not $i219Adapter) {
    Write-Host "ERROR: Intel I219 interface not found!" -ForegroundColor Red
    exit 1
}

$windowsInterfaceName = $i219Adapter.Name
$pcapDeviceName = "\Device\NPF_$($i219Adapter.InterfaceGuid)"

Write-Host "SUCCESS: Intel I219 found!" -ForegroundColor Green
Write-Host "  Windows Name: $windowsInterfaceName" -ForegroundColor White
Write-Host "  PCap Device: $pcapDeviceName" -ForegroundColor White
Write-Host "  Description: $($i219Adapter.InterfaceDescription)" -ForegroundColor White
Write-Host ""

# Step 2: Stop existing daemons
Write-Host "Step 2: Stopping existing OpenAvnu daemons..." -ForegroundColor Yellow
$processNames = @("mrpd", "maap_daemon", "gptp2")
foreach ($processName in $processNames) {
    $processes = Get-Process -Name $processName -ErrorAction SilentlyContinue
    foreach ($process in $processes) {
        Write-Host "  Stopping $processName (PID: $($process.Id))..." -ForegroundColor Gray
        $process.Kill()
        $process.WaitForExit(3000)
    }
}
Start-Sleep -Seconds 2

# Step 3: Start MRPD daemon
Write-Host "Step 3: Starting MRPD daemon..." -ForegroundColor Yellow
try {
    $mrpdProcess = Start-Process -FilePath $mrpdPath -ArgumentList @("-i", "`"$windowsInterfaceName`"") `
                                -WorkingDirectory $buildDir -PassThru -WindowStyle Hidden
    Start-Sleep -Seconds 2
    
    if ($mrpdProcess.HasExited) {
        Write-Host "ERROR: MRPD daemon failed to start!" -ForegroundColor Red
        exit 1
    } else {
        Write-Host "SUCCESS: MRPD daemon started (PID: $($mrpdProcess.Id))" -ForegroundColor Green
    }
} catch {
    Write-Host "ERROR: Failed to start MRPD: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}

# Step 4: Start MAAP daemon with pcap device name
Write-Host "Step 4: Starting MAAP daemon..." -ForegroundColor Yellow
try {
    $maapProcess = Start-Process -FilePath $maapPath -ArgumentList @("-i", "`"$pcapDeviceName`"") `
                                -WorkingDirectory $buildDir -PassThru -WindowStyle Hidden
    Start-Sleep -Seconds 2
    
    if ($maapProcess.HasExited) {
        Write-Host "ERROR: MAAP daemon failed to start!" -ForegroundColor Red
        # Show error details
        Write-Host "Exit code: $($maapProcess.ExitCode)" -ForegroundColor Red
        exit 1
    } else {
        Write-Host "SUCCESS: MAAP daemon started (PID: $($maapProcess.Id))" -ForegroundColor Green
    }
} catch {
    Write-Host "ERROR: Failed to start MAAP: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}

# Step 5: Wait a moment and verify daemons are still running
Write-Host ""
Write-Host "Step 5: Verifying daemon status..." -ForegroundColor Yellow
Start-Sleep -Seconds 3

$mrpdStillRunning = -not $mrpdProcess.HasExited
$maapStillRunning = -not $maapProcess.HasExited

Write-Host "MRPD Status: $(if($mrpdStillRunning){'RUNNING'}else{'STOPPED'})" -ForegroundColor $(if($mrpdStillRunning){'Green'}else{'Red'})
Write-Host "MAAP Status: $(if($maapStillRunning){'RUNNING'}else{'STOPPED'})" -ForegroundColor $(if($maapStillRunning){'Green'}else{'Red'})

if ($mrpdStillRunning -and $maapStillRunning) {
    Write-Host ""
    Write-Host "=== SUCCESS! OpenAvnu Ready for ASIO ===" -ForegroundColor Green
    Write-Host "Network Interface: $windowsInterfaceName (Intel I219 Auto-Detected)" -ForegroundColor Green
    Write-Host "MRPD Daemon: RUNNING (PID: $($mrpdProcess.Id))" -ForegroundColor Green
    Write-Host "MAAP Daemon: RUNNING (PID: $($maapProcess.Id))" -ForegroundColor Green
    Write-Host ""
    Write-Host "Your Intel I219 is now configured for AVB/TSN audio streaming!" -ForegroundColor Green
    Write-Host "ASIO Audio Listener can now connect to the OpenAvnu stack." -ForegroundColor Green
} else {
    Write-Host ""
    Write-Host "WARNING: Some daemons are not running properly!" -ForegroundColor Yellow
    Write-Host "Check the daemon logs for more information." -ForegroundColor Yellow
}

Write-Host ""
Write-Host "To stop all daemons, use:" -ForegroundColor Cyan
Write-Host "  Get-Process mrpd,maap_daemon | Stop-Process" -ForegroundColor White
