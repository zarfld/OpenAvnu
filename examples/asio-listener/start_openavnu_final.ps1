Write-Host "=== OpenAvnu Multi-Intel Adapter Startup ===" -ForegroundColor Cyan
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
    # Ensure working directory exists and is valid
    $workingDir = Split-Path -Parent $mrpdPath
    if (-not (Test-Path $workingDir)) {
        $workingDir = $buildDir
    }
    if (-not (Test-Path $workingDir)) {
        $workingDir = Get-Location
    }
    
    Write-Host "  Using working directory: $workingDir" -ForegroundColor Gray
    Write-Host "  MRPD path: $mrpdPath" -ForegroundColor Gray
    Write-Host "  Interface: $windowsInterfaceName" -ForegroundColor Gray
    
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

# Step 4: Start MAAP daemon
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
    Write-Host "  MAAP path: $maapPath" -ForegroundColor Gray
    Write-Host "  PCap device: $pcapDeviceName" -ForegroundColor Gray
    
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
