Write-Host "=== OpenAvnu ASIO Startup with Corrected MAAP ===" -ForegroundColor Cyan
Write-Host ""

# Dynamically find the build directory and executables
$scriptLocation = Split-Path -Parent $MyInvocation.MyCommand.Path
$workspaceRoot = Split-Path -Parent (Split-Path -Parent $scriptLocation)

# Try different possible build directory locations
$possibleBuildDirs = @(
    "$workspaceRoot\build",
    "$workspaceRoot\..\build",
    "C:\Users\$env:USERNAME\Source\OpenAvnu\build",
    ".\..\..\build",
    "build"
)

$buildDir = $null
$mrpdPath = $null
$maapPath = $null

foreach ($dir in $possibleBuildDirs) {
    $testMrpd = "$dir\daemons\mrpd\Release\mrpd.exe"
    $testMaap = "$dir\daemons\maap\Release\maap_daemon.exe"
    
    if ((Test-Path $testMrpd) -and (Test-Path $testMaap)) {
        $buildDir = $dir
        $mrpdPath = $testMrpd
        $maapPath = $testMaap
        Write-Host "Found OpenAvnu build at: $buildDir" -ForegroundColor Green
        break
    }
}

if (-not $buildDir) {
    Write-Host "ERROR: OpenAvnu executables not found!" -ForegroundColor Red
    Write-Host "Searched in:" -ForegroundColor Yellow
    foreach ($dir in $possibleBuildDirs) {
        Write-Host "  $dir" -ForegroundColor Gray
    }
    Write-Host "Please build OpenAvnu first or check paths" -ForegroundColor Yellow
    exit 1
}

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
    # Ensure working directory exists and is valid
    $workingDir = Split-Path -Parent $mrpdPath
    if (-not (Test-Path $workingDir)) {
        $workingDir = $buildDir
    }
    if (-not (Test-Path $workingDir)) {
        $workingDir = Get-Location
    }
    
    Write-Host "  Using working directory: $workingDir" -ForegroundColor Gray
    
    $mrpdProcess = Start-Process -FilePath $mrpdPath -ArgumentList @("-i", "`"$windowsInterfaceName`"") `
                                -WorkingDirectory $workingDir -PassThru -WindowStyle Hidden
    Start-Sleep -Seconds 2
    
    if ($mrpdProcess -and -not $mrpdProcess.HasExited) {
        Write-Host "SUCCESS: MRPD daemon started (PID: $($mrpdProcess.Id))" -ForegroundColor Green
    } else {
        Write-Host "ERROR: MRPD daemon failed to start!" -ForegroundColor Red
        exit 1
    }
} catch {
    Write-Host "ERROR: Failed to start MRPD: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}

# Step 4: Start MAAP daemon with pcap device name
Write-Host "Step 4: Starting MAAP daemon..." -ForegroundColor Yellow
try {
    # Ensure working directory exists and is valid
    $workingDir = Split-Path -Parent $maapPath
    if (-not (Test-Path $workingDir)) {
        $workingDir = $buildDir
    }
    if (-not (Test-Path $workingDir)) {
        $workingDir = Get-Location
    }
    
    Write-Host "  Using working directory: $workingDir" -ForegroundColor Gray
    
    $maapProcess = Start-Process -FilePath $maapPath -ArgumentList @("-i", "`"$pcapDeviceName`"") `
                                -WorkingDirectory $workingDir -PassThru -WindowStyle Hidden
    Start-Sleep -Seconds 2
    
    if ($maapProcess -and -not $maapProcess.HasExited) {
        Write-Host "SUCCESS: MAAP daemon started (PID: $($maapProcess.Id))" -ForegroundColor Green
    } else {
        Write-Host "ERROR: MAAP daemon failed to start!" -ForegroundColor Red
        exit 1
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
