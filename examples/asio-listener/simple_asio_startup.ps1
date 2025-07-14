Write-Host "=== OpenAvnu ASIO Audio Listener Startup ===" -ForegroundColor Cyan
Write-Host ""

# Step 1: Auto-detect Intel I219
Write-Host "Step 1: Auto-detecting Intel I219 network interface..." -ForegroundColor Yellow
$i219Adapter = Get-NetAdapter | Where-Object { 
    $_.InterfaceDescription -like "*Intel*" -and 
    $_.InterfaceDescription -like "*I219*" -and
    $_.Status -eq "Up"
} | Select-Object -First 1

if ($i219Adapter) {
    Write-Host "SUCCESS: Intel I219 found!" -ForegroundColor Green
    Write-Host "Interface: $($i219Adapter.Name)" -ForegroundColor White
    Write-Host "Description: $($i219Adapter.InterfaceDescription)" -ForegroundColor White
    Write-Host "Link Speed: $($i219Adapter.LinkSpeed)" -ForegroundColor White
    Write-Host ""
    
    $networkInterface = $i219Adapter.Name
} else {
    Write-Host "ERROR: No Intel I219 interface found!" -ForegroundColor Red
    exit 1
}

# Step 2: Stop any existing daemons
Write-Host "Step 2: Stopping any existing OpenAvnu daemons..." -ForegroundColor Yellow
$processNames = @("mrpd", "maap_daemon", "gptp2", "openavnu_shaper_windows")
foreach ($processName in $processNames) {
    $processes = Get-Process -Name $processName -ErrorAction SilentlyContinue
    foreach ($process in $processes) {
        Write-Host "Stopping $processName (PID: $($process.Id))..." -ForegroundColor Gray
        $process.Kill()
        $process.WaitForExit(3000)
    }
}
Start-Sleep -Seconds 2

# Step 3: Check if executables exist
Write-Host "Step 3: Checking OpenAvnu executables..." -ForegroundColor Yellow
$buildDir = "d:\Repos\OpenAvnu\build"
$mrpdPath = "$buildDir\daemons\mrpd\Release\mrpd.exe"
$maapPath = "$buildDir\daemons\maap\Release\maap_daemon.exe"
$gptpPath = "$buildDir\gptp\windows_hal\Release\gptp2.exe"

$mrpdExists = Test-Path $mrpdPath
$maapExists = Test-Path $maapPath
$gptpExists = Test-Path $gptpPath

Write-Host "MRPD: $(if($mrpdExists){'Found'}else{'Missing'}) - $mrpdPath" -ForegroundColor $(if($mrpdExists){'Green'}else{'Red'})
Write-Host "MAAP: $(if($maapExists){'Found'}else{'Missing'}) - $maapPath" -ForegroundColor $(if($maapExists){'Green'}else{'Red'})
Write-Host "gPTP: $(if($gptpExists){'Found'}else{'Missing'}) - $gptpPath" -ForegroundColor $(if($gptpExists){'Green'}else{'Yellow'})
Write-Host ""

if (-not $mrpdExists -or -not $maapExists) {
    Write-Host "ERROR: Required daemons not found! Please build OpenAvnu first." -ForegroundColor Red
    exit 1
}

# Step 4: Start MRPD daemon
Write-Host "Step 4: Starting MRPD daemon..." -ForegroundColor Yellow
try {
    $mrpdProcess = Start-Process -FilePath $mrpdPath -ArgumentList @("-i", "`"$networkInterface`"") `
                                -WorkingDirectory $buildDir -PassThru -WindowStyle Hidden
    Start-Sleep -Seconds 3
    
    if ($mrpdProcess.HasExited) {
        Write-Host "ERROR: MRPD daemon exited unexpectedly!" -ForegroundColor Red
        exit 1
    } else {
        Write-Host "SUCCESS: MRPD daemon started (PID: $($mrpdProcess.Id))" -ForegroundColor Green
    }
} catch {
    Write-Host "ERROR: Failed to start MRPD daemon: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}

# Step 5: Start MAAP daemon
Write-Host "Step 5: Starting MAAP daemon..." -ForegroundColor Yellow
try {
    $maapProcess = Start-Process -FilePath $maapPath -ArgumentList @("-i", "`"$networkInterface`"") `
                                -WorkingDirectory $buildDir -PassThru -WindowStyle Hidden
    Start-Sleep -Seconds 3
    
    if ($maapProcess.HasExited) {
        Write-Host "ERROR: MAAP daemon exited unexpectedly!" -ForegroundColor Red
        exit 1
    } else {
        Write-Host "SUCCESS: MAAP daemon started (PID: $($maapProcess.Id))" -ForegroundColor Green
    }
} catch {
    Write-Host "ERROR: Failed to start MAAP daemon: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}

# Step 6: Start gPTP daemon (optional)
if ($gptpExists) {
    Write-Host "Step 6: Starting gPTP daemon..." -ForegroundColor Yellow
    try {
        $configPath = "d:\Repos\OpenAvnu\gptp_cfg.ini"
        $gptpProcess = Start-Process -FilePath $gptpPath -ArgumentList @("-F", $configPath) `
                                    -WorkingDirectory $buildDir -PassThru -WindowStyle Hidden
        Start-Sleep -Seconds 3
        
        if ($gptpProcess.HasExited) {
            Write-Host "WARNING: gPTP daemon exited, but continuing..." -ForegroundColor Yellow
        } else {
            Write-Host "SUCCESS: gPTP daemon started (PID: $($gptpProcess.Id))" -ForegroundColor Green
        }
    } catch {
        Write-Host "WARNING: Failed to start gPTP daemon, but continuing..." -ForegroundColor Yellow
    }
} else {
    Write-Host "Step 6: Skipping gPTP daemon (not found)" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "=== OpenAvnu Startup Complete ===" -ForegroundColor Green
Write-Host "Network Interface: $networkInterface (Intel I219 Auto-Detected)" -ForegroundColor Green
Write-Host "MRPD Daemon: RUNNING" -ForegroundColor Green
Write-Host "MAAP Daemon: RUNNING" -ForegroundColor Green
Write-Host "gPTP Daemon: $(if($gptpExists -and -not $gptpProcess.HasExited){'RUNNING'}else{'OPTIONAL'})" -ForegroundColor $(if($gptpExists -and -not $gptpProcess.HasExited){'Green'}else{'Yellow'})
Write-Host ""
Write-Host "Ready for ASIO Audio Listener! No manual configuration needed!" -ForegroundColor Green
