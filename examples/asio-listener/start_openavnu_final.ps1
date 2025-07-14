Write-Host "=== OpenAvnu Multi-Intel Adapter Startup ===" -ForegroundColor Cyan
Write-Host ""

$buildDir = "d:\Repos\OpenAvnu\build"
$mrpdPath = "$buildDir\daemons\mrpd\Release\mrpd.exe"
$maapPath = "$buildDir\daemons\maap\Release\maap_daemon.exe"

# Step 1: Multi-Intel Adapter Detection with Priority
Write-Host "Step 1: Multi-Intel adapter detection..." -ForegroundColor Yellow
Write-Host "Priority order: I226 > I225 > I210 > I219 > I217" -ForegroundColor Gray
Write-Host ""

# Get all Intel Ethernet adapters
$intelAdapters = Get-NetAdapter | Where-Object { 
    $_.InterfaceDescription -like "*Intel*" -and 
    $_.InterfaceDescription -like "*Ethernet*" -and
    $_.Status -eq "Up"
}

if ($intelAdapters.Count -eq 0) {
    Write-Host "ERROR: No Intel Ethernet adapters found!" -ForegroundColor Red
    exit 1
}

Write-Host "Found $($intelAdapters.Count) Intel Ethernet adapter(s):" -ForegroundColor Green
foreach ($adapter in $intelAdapters) {
    Write-Host "  - $($adapter.Name): $($adapter.InterfaceDescription)" -ForegroundColor White
}
Write-Host ""

# Find best supported adapter
$bestAdapter = $null
$bestPriority = 99
$bestTypeName = ""

foreach ($adapter in $intelAdapters) {
    $description = $adapter.InterfaceDescription
    $priority = 99
    $typeName = "Unknown Intel"
    
    if ($description -like "*I226*") {
        $priority = 1
        $typeName = "Intel I226"
    }
    elseif ($description -like "*I225*") {
        $priority = 2
        $typeName = "Intel I225"
    }
    elseif ($description -like "*I210*") {
        $priority = 3
        $typeName = "Intel I210"
    }
    elseif ($description -like "*I219*") {
        $priority = 4
        $typeName = "Intel I219"
    }
    elseif ($description -like "*I217*") {
        $priority = 5
        $typeName = "Intel I217"
    }
    
    if ($priority -lt $bestPriority) {
        $bestPriority = $priority
        $bestAdapter = $adapter
        $bestTypeName = $typeName
    }
    
    if ($priority -lt 99) {
        Write-Host "Found supported: $typeName (Priority: $priority)" -ForegroundColor Green
    }
}

if (-not $bestAdapter) {
    Write-Host "ERROR: No supported Intel adapters found!" -ForegroundColor Red
    Write-Host "Supported models: I226, I225, I210, I219, I217" -ForegroundColor Yellow
    exit 1
}

Write-Host ""
Write-Host "=== SELECTED ADAPTER ===" -ForegroundColor Green
Write-Host "Best Available: $bestTypeName" -ForegroundColor Green
Write-Host "Interface Name: $($bestAdapter.Name)" -ForegroundColor White
Write-Host "Description: $($bestAdapter.InterfaceDescription)" -ForegroundColor White
Write-Host "Priority Rank: $bestPriority of 5" -ForegroundColor White
Write-Host "Link Speed: $($bestAdapter.LinkSpeed)" -ForegroundColor White

$windowsInterfaceName = $bestAdapter.Name
$pcapDeviceName = "\Device\NPF_$($bestAdapter.InterfaceGuid)"

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

# Step 4: Start MAAP daemon
Write-Host "Step 4: Starting MAAP daemon..." -ForegroundColor Yellow
try {
    $maapProcess = Start-Process -FilePath $maapPath -ArgumentList @("-i", "`"$pcapDeviceName`"") `
                                -WorkingDirectory $buildDir -PassThru -WindowStyle Hidden
    Start-Sleep -Seconds 2
    
    if ($maapProcess.HasExited) {
        Write-Host "ERROR: MAAP daemon failed to start!" -ForegroundColor Red
        exit 1
    } else {
        Write-Host "SUCCESS: MAAP daemon started (PID: $($maapProcess.Id))" -ForegroundColor Green
    }
} catch {
    Write-Host "ERROR: Failed to start MAAP: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}

# Step 5: Verify and show final status
Write-Host ""
Write-Host "Step 5: Verifying daemon status..." -ForegroundColor Yellow
Start-Sleep -Seconds 3

$mrpdRunning = -not $mrpdProcess.HasExited
$maapRunning = -not $maapProcess.HasExited

if ($mrpdRunning -and $maapRunning) {
    Write-Host ""
    Write-Host "=== SUCCESS! Multi-Adapter OpenAvnu Ready ===" -ForegroundColor Green
    Write-Host "Selected: $bestTypeName (Priority: $bestPriority)" -ForegroundColor Green
    Write-Host "Interface: $windowsInterfaceName" -ForegroundColor Green
    Write-Host "MRPD: RUNNING (PID: $($mrpdProcess.Id))" -ForegroundColor Green
    Write-Host "MAAP: RUNNING (PID: $($maapProcess.Id))" -ForegroundColor Green
    
    # Show adapter-specific recommendations
    Write-Host ""
    Write-Host "Adapter-Specific Configuration:" -ForegroundColor Cyan
    if ($bestTypeName -like "*I226*" -or $bestTypeName -like "*I225*") {
        Write-Host "  Optimized for: Professional AVB/TSN audio streaming" -ForegroundColor Green
        Write-Host "  Features: Full TSN support, latest Intel HAL" -ForegroundColor Green
    }
    elseif ($bestTypeName -like "*I210*") {
        Write-Host "  Optimized for: Professional AVB audio streaming" -ForegroundColor Green
        Write-Host "  Features: Proven AVB/TSN technology" -ForegroundColor Green
    }
    elseif ($bestTypeName -like "*I219*") {
        Write-Host "  Optimized for: Basic AVB audio streaming" -ForegroundColor Yellow
        Write-Host "  Features: Good AVB support, limited TSN" -ForegroundColor Yellow
    }
    elseif ($bestTypeName -like "*I217*") {
        Write-Host "  Optimized for: Legacy AVB applications" -ForegroundColor Yellow
        Write-Host "  Features: Basic AVB support only" -ForegroundColor Yellow
    }
    
    Write-Host ""
    Write-Host "ASIO Audio Listener ready for professional streaming!" -ForegroundColor Green
    
} else {
    Write-Host ""
    Write-Host "WARNING: Daemon startup incomplete!" -ForegroundColor Yellow
    Write-Host "MRPD: $(if($mrpdRunning){'RUNNING'}else{'FAILED'})" -ForegroundColor $(if($mrpdRunning){'Green'}else{'Red'})
    Write-Host "MAAP: $(if($maapRunning){'RUNNING'}else{'FAILED'})" -ForegroundColor $(if($maapRunning){'Green'}else{'Red'})
}

Write-Host ""
Write-Host "=== Configuration Summary ===" -ForegroundColor Cyan
Write-Host "Adapter Type: $bestTypeName" -ForegroundColor White
Write-Host "Windows Interface: $windowsInterfaceName" -ForegroundColor White
Write-Host "PCap Device: $pcapDeviceName" -ForegroundColor White
Write-Host "Priority Rank: $bestPriority of 5 (1=best)" -ForegroundColor White
Write-Host ""
Write-Host "To stop all daemons:" -ForegroundColor Cyan
Write-Host "  Get-Process mrpd,maap_daemon | Stop-Process" -ForegroundColor White
