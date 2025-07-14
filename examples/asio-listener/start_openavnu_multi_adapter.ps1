Write-Host "=== OpenAvnu Multi-Adapter Detection & Startup ===" -ForegroundColor Cyan
Write-Host ""

$buildDir = "d:\Repos\OpenAvnu\build"
$mrpdPath = "$buildDir\daemons\mrpd\Release\mrpd.exe"
$maapPath = "$buildDir\daemons\maap\Release\maap_daemon.exe"

# Intel Adapter Priority List (highest to lowest priority)
$IntelAdapterPriority = @(
    @{Pattern = "*I226*"; Name = "Intel I226"; Priority = 1},
    @{Pattern = "*I225*"; Name = "Intel I225"; Priority = 2},
    @{Pattern = "*I210*"; Name = "Intel I210"; Priority = 3},
    @{Pattern = "*I219*"; Name = "Intel I219"; Priority = 4},
    @{Pattern = "*I217*"; Name = "Intel I217"; Priority = 5}
)

function Get-BestIntelAdapter {
    Write-Host "Scanning for supported Intel adapters..." -ForegroundColor Yellow
    
    # Get all Intel Ethernet adapters that are Up
    $allIntelAdapters = Get-NetAdapter | Where-Object { 
        $_.InterfaceDescription -like "*Intel*" -and 
        $_.InterfaceDescription -like "*Ethernet*" -and
        $_.Status -eq "Up"
    }
    
    if ($allIntelAdapters.Count -eq 0) {
        Write-Host "No Intel Ethernet adapters found!" -ForegroundColor Red
        return $null
    }
    
    Write-Host "Found $($allIntelAdapters.Count) Intel Ethernet adapter(s):" -ForegroundColor Cyan
    foreach ($adapter in $allIntelAdapters) {
        Write-Host "  - $($adapter.Name): $($adapter.InterfaceDescription)" -ForegroundColor White
    }
    Write-Host ""
    
    # Check each adapter against priority list
    $foundAdapters = @()
    
    foreach ($priority in $IntelAdapterPriority) {
        foreach ($adapter in $allIntelAdapters) {
            if ($adapter.InterfaceDescription -like $priority.Pattern) {
                $foundAdapters += @{
                    Adapter = $adapter
                    Priority = $priority.Priority
                    TypeName = $priority.Name
                }
                Write-Host "✓ Found $($priority.Name): $($adapter.Name)" -ForegroundColor Green
                Write-Host "  Description: $($adapter.InterfaceDescription)" -ForegroundColor Gray
                Write-Host "  Link Speed: $($adapter.LinkSpeed)" -ForegroundColor Gray
                Write-Host ""
            }
        }
    }
    
    if ($foundAdapters.Count -eq 0) {
        Write-Host "No supported Intel adapters found!" -ForegroundColor Yellow
        Write-Host "Supported models: I226, I225, I210, I219, I217" -ForegroundColor Yellow
        
        # Fallback to any Intel Ethernet adapter
        Write-Host "Using first available Intel Ethernet adapter as fallback..." -ForegroundColor Yellow
        $fallbackAdapter = $allIntelAdapters[0]
        return @{
            Adapter = $fallbackAdapter
            Priority = 99
            TypeName = "Intel Ethernet (Unknown Model)"
        }
    }
    
    # Sort by priority and return the best one
    $bestAdapter = $foundAdapters | Sort-Object Priority | Select-Object -First 1
    
    Write-Host "=== SELECTED ADAPTER ===" -ForegroundColor Green
    Write-Host "Best Intel Adapter: $($bestAdapter.TypeName)" -ForegroundColor Green
    Write-Host "Interface Name: $($bestAdapter.Adapter.Name)" -ForegroundColor White
    Write-Host "Description: $($bestAdapter.Adapter.InterfaceDescription)" -ForegroundColor White
    Write-Host "Priority: $($bestAdapter.Priority) (1=highest)" -ForegroundColor White
    Write-Host "Link Speed: $($bestAdapter.Adapter.LinkSpeed)" -ForegroundColor White
    Write-Host ""
    
    return $bestAdapter
}

function Get-AdapterCapabilities {
    param($Adapter)
    
    Write-Host "Checking adapter capabilities..." -ForegroundColor Yellow
    
    $typeName = $Adapter.TypeName
    $description = $Adapter.Adapter.InterfaceDescription
    
    # Determine capabilities based on adapter type
    $capabilities = @{
        AVBSupport = $false
        TSNSupport = $false
        IntelHAL = $false
        RecommendedUse = ""
    }
    
    if ($description -like "*I226*") {
        $capabilities.AVBSupport = $true
        $capabilities.TSNSupport = $true
        $capabilities.IntelHAL = $true
        $capabilities.RecommendedUse = "Excellent for professional AVB/TSN audio - latest technology"
    }
    elseif ($description -like "*I225*") {
        $capabilities.AVBSupport = $true
        $capabilities.TSNSupport = $true
        $capabilities.IntelHAL = $true
        $capabilities.RecommendedUse = "Excellent for professional AVB/TSN audio"
    }
    elseif ($description -like "*I210*") {
        $capabilities.AVBSupport = $true
        $capabilities.TSNSupport = $true
        $capabilities.IntelHAL = $true
        $capabilities.RecommendedUse = "Very good for AVB/TSN audio - proven technology"
    }
    elseif ($description -like "*I219*") {
        $capabilities.AVBSupport = $true
        $capabilities.TSNSupport = $false
        $capabilities.IntelHAL = $true
        $capabilities.RecommendedUse = "Good for AVB audio - limited TSN support"
    }
    elseif ($description -like "*I217*") {
        $capabilities.AVBSupport = $true
        $capabilities.TSNSupport = $false
        $capabilities.IntelHAL = $true
        $capabilities.RecommendedUse = "Basic AVB support - older technology"
    }
    else {
        $capabilities.RecommendedUse = "Unknown capabilities - may have limited AVB/TSN support"
    }
    
    Write-Host "Adapter Capabilities:" -ForegroundColor Cyan
    Write-Host "  AVB Support: $(if($capabilities.AVBSupport){'✓ Yes'}else{'✗ No'})" -ForegroundColor $(if($capabilities.AVBSupport){'Green'}else{'Red'})
    Write-Host "  TSN Support: $(if($capabilities.TSNSupport){'✓ Yes'}else{'✗ Limited'})" -ForegroundColor $(if($capabilities.TSNSupport){'Green'}else{'Yellow'})
    Write-Host "  Intel HAL: $(if($capabilities.IntelHAL){'✓ Yes'}else{'✗ No'})" -ForegroundColor $(if($capabilities.IntelHAL){'Green'}else{'Red'})
    Write-Host "  Recommendation: $($capabilities.RecommendedUse)" -ForegroundColor White
    Write-Host ""
    
    return $capabilities
}

# Step 1: Find best Intel adapter
Write-Host "Step 1: Multi-adapter detection with priority ranking..." -ForegroundColor Yellow
$selectedAdapter = Get-BestIntelAdapter

if (-not $selectedAdapter) {
    Write-Host "ERROR: No suitable Intel adapter found!" -ForegroundColor Red
    exit 1
}

$adapter = $selectedAdapter.Adapter
$windowsInterfaceName = $adapter.Name
$pcapDeviceName = "\Device\NPF_$($adapter.InterfaceGuid)"

# Show adapter capabilities
$capabilities = Get-AdapterCapabilities $selectedAdapter

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

# Step 5: Verify daemon status
Write-Host ""
Write-Host "Step 5: Verifying daemon status..." -ForegroundColor Yellow
Start-Sleep -Seconds 3

$mrpdStillRunning = -not $mrpdProcess.HasExited
$maapStillRunning = -not $maapProcess.HasExited

Write-Host "MRPD Status: $(if($mrpdStillRunning){'RUNNING'}else{'STOPPED'})" -ForegroundColor $(if($mrpdStillRunning){'Green'}else{'Red'})
Write-Host "MAAP Status: $(if($maapStillRunning){'RUNNING'}else{'STOPPED'})" -ForegroundColor $(if($maapStillRunning){'Green'}else{'Red'})

if ($mrpdStillRunning -and $maapStillRunning) {
    Write-Host ""
    Write-Host "=== SUCCESS! OpenAvnu Multi-Adapter Ready ===" -ForegroundColor Green
    Write-Host "Selected Adapter: $($selectedAdapter.TypeName)" -ForegroundColor Green
    Write-Host "Interface: $windowsInterfaceName" -ForegroundColor Green
    Write-Host "Priority Rank: $($selectedAdapter.Priority) of 5 (1=best)" -ForegroundColor Green
    Write-Host "MRPD Daemon: RUNNING (PID: $($mrpdProcess.Id))" -ForegroundColor Green
    Write-Host "MAAP Daemon: RUNNING (PID: $($maapProcess.Id))" -ForegroundColor Green
    Write-Host ""
    Write-Host "Your $($selectedAdapter.TypeName) is optimally configured!" -ForegroundColor Green
    Write-Host "ASIO Audio Listener ready for professional streaming." -ForegroundColor Green
} else {
    Write-Host ""
    Write-Host "WARNING: Some daemons failed to start!" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "=== Configuration Summary ===" -ForegroundColor Cyan
Write-Host "Adapter Type: $($selectedAdapter.TypeName)" -ForegroundColor White
Write-Host "Windows Interface: $windowsInterfaceName" -ForegroundColor White
Write-Host "PCap Device: $pcapDeviceName" -ForegroundColor White
Write-Host "AVB Support: $(if($capabilities.AVBSupport){'✓'}else{'✗'})" -ForegroundColor $(if($capabilities.AVBSupport){'Green'}else{'Red'})
Write-Host "TSN Support: $(if($capabilities.TSNSupport){'✓'}else{'Limited'})" -ForegroundColor $(if($capabilities.TSNSupport){'Green'}else{'Yellow'})
Write-Host ""
Write-Host "To stop daemons: Get-Process mrpd,maap_daemon | Stop-Process" -ForegroundColor Cyan
