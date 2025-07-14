# ============================================================================
# OpenAvnu Daemon Startup Framework for ASIO Audio Listener
# Coordinated startup with dependency checking and health monitoring
# ============================================================================

param(
    [string]$NetworkInterface = "", # Auto-detect if empty
    [switch]$SkipGptp,
    [switch]$Verbose,
    [switch]$DryRun,
    [int]$StartupTimeout = 30
)

# Global configuration
$ErrorActionPreference = "Stop"
$BuildDir = "d:\Repos\OpenAvnu\build"
$ConfigDir = "d:\Repos\OpenAvnu"

# Daemon process tracking
$Global:DaemonProcesses = @{}

# ============================================================================
# Utility Functions
# ============================================================================

function Write-StatusMessage {
    param([string]$Message, [string]$Status = "INFO")
    
    $timestamp = Get-Date -Format "HH:mm:ss"
    $color = switch ($Status) {
        "SUCCESS" { "Green" }
        "WARNING" { "Yellow" }
        "ERROR"   { "Red" }
        "INFO"    { "Cyan" }
        default   { "White" }
    }
    
    Write-Host "[$timestamp] [$Status] $Message" -ForegroundColor $color
}

function Test-ProcessRunning {
    param([string]$ProcessName)
    
    return (Get-Process -Name $ProcessName -ErrorAction SilentlyContinue) -ne $null
}

function Wait-ForDaemonReady {
    param(
        [string]$DaemonName,
        [scriptblock]$HealthCheck,
        [int]$TimeoutSeconds = 10
    )
    
    Write-StatusMessage "Waiting for $DaemonName to become ready..." "INFO"
    
    $timeout = (Get-Date).AddSeconds($TimeoutSeconds)
    while ((Get-Date) -lt $timeout) {
        if (& $HealthCheck) {
            Write-StatusMessage "$DaemonName is ready!" "SUCCESS"
            return $true
        }
        Start-Sleep -Milliseconds 500
    }
    
    Write-StatusMessage "$DaemonName failed to start within $TimeoutSeconds seconds!" "ERROR"
    return $false
}

function Start-DaemonProcess {
    param(
        [string]$Name,
        [string]$ExecutablePath,
        [string[]]$Arguments = @(),
        [scriptblock]$HealthCheck,
        [string]$WorkingDirectory = $BuildDir
    )
    
    if ($DryRun) {
        Write-StatusMessage "[DRY RUN] Would start: $ExecutablePath $($Arguments -join ' ')" "INFO"
        return $true
    }
    
    if (-not (Test-Path $ExecutablePath)) {
        Write-StatusMessage "Executable not found: $ExecutablePath" "ERROR"
        return $false
    }
    
    Write-StatusMessage "Starting $Name daemon..." "INFO"
    if ($Verbose) {
        Write-StatusMessage "Command: $ExecutablePath $($Arguments -join ' ')" "INFO"
    }
    
    try {
        $process = Start-Process -FilePath $ExecutablePath -ArgumentList $Arguments `
                                -WorkingDirectory $WorkingDirectory -PassThru -WindowStyle Hidden
        
        $Global:DaemonProcesses[$Name] = $process
        
        if ($HealthCheck) {
            return Wait-ForDaemonReady -DaemonName $Name -HealthCheck $HealthCheck -TimeoutSeconds $StartupTimeout
        } else {
            Start-Sleep -Seconds 2
            if ($process.HasExited) {
                Write-StatusMessage "$Name daemon exited unexpectedly!" "ERROR"
                return $false
            }
            Write-StatusMessage "$Name daemon started successfully!" "SUCCESS"
            return $true
        }
    }
    catch {
        Write-StatusMessage "Failed to start $Name daemon: $($_.Exception.Message)" "ERROR"
        return $false
    }
}

# ============================================================================
# Daemon Health Check Functions  
# ============================================================================

function Test-GptpReady {
    # Check if gPTP process is running and responsive
    return (Test-ProcessRunning "gptp2") -and (Test-Path "$ConfigDir\gptp.log" -ErrorAction SilentlyContinue)
}

function Test-MrpdReady {
    # Check if MRPD process is running
    return Test-ProcessRunning "mrpd"
}

function Test-MaapReady {
    # Check if MAAP daemon is running  
    return Test-ProcessRunning "maap_daemon"
}

function Test-ShaperReady {
    # Check if Windows Shaper is running
    return Test-ProcessRunning "openavnu_shaper_windows"
}

# ============================================================================
# Network Interface Validation
# ============================================================================

# ============================================================================
# Network Interface Auto-Detection and Validation
# ============================================================================

function Get-BestNetworkInterface {
    Write-StatusMessage "Auto-detecting best network interface..." "INFO"
    
    # List all available adapters for debugging
    if ($Verbose) {
        Write-StatusMessage "Available network adapters:" "INFO"
        Get-NetAdapter | Where-Object { $_.Status -eq "Up" } | ForEach-Object {
            Write-StatusMessage "  - $($_.Name): $($_.InterfaceDescription)" "INFO"
        }
    }
    
    # Priority 1: Intel I219 adapters (your specific hardware)
    $i219Adapters = @(Get-NetAdapter | Where-Object { 
        $_.InterfaceDescription -like "*Intel*" -and 
        $_.InterfaceDescription -like "*I219*" -and
        $_.Status -eq "Up"
    })
    
    if ($i219Adapters.Count -gt 0) {
        $selected = $i219Adapters[0]
        Write-StatusMessage "★ Found Intel I219 adapter: $($selected.Name) ★" "SUCCESS"
        Write-StatusMessage "Description: $($selected.InterfaceDescription)" "INFO"
        Write-StatusMessage "Link Speed: $($selected.LinkSpeed)" "INFO"
        Write-StatusMessage "This adapter supports Intel HAL for best AVB performance!" "SUCCESS"
        return $selected.Name
    }
    
    # Priority 2: Intel I225/I226 adapters
    $i225i226Adapters = @(Get-NetAdapter | Where-Object { 
        $_.InterfaceDescription -like "*Intel*" -and 
        ($_.InterfaceDescription -like "*I225*" -or $_.InterfaceDescription -like "*I226*") -and
        $_.Status -eq "Up"
    })
    
    if ($i225i226Adapters.Count -gt 0) {
        $selected = $i225i226Adapters[0]
        Write-StatusMessage "Found Intel I225/I226 adapter: $($selected.Name)" "SUCCESS"
        Write-StatusMessage "Description: $($selected.InterfaceDescription)" "INFO"
        return $selected.Name
    }
    
    # Priority 3: Any other Intel Ethernet adapter
    $intelEthernetAdapters = @(Get-NetAdapter | Where-Object { 
        $_.InterfaceDescription -like "*Intel*" -and 
        $_.InterfaceDescription -like "*Ethernet*" -and
        $_.Status -eq "Up"
    })
    
    if ($intelEthernetAdapters.Count -gt 0) {
        $selected = $intelEthernetAdapters[0]
        Write-StatusMessage "Found Intel Ethernet adapter: $($selected.Name)" "SUCCESS"
        Write-StatusMessage "Description: $($selected.InterfaceDescription)" "INFO"
        return $selected.Name
    }
    
    # Priority 4: Any Intel adapter (excluding WiFi)
    $anyIntelAdapters = @(Get-NetAdapter | Where-Object { 
        $_.InterfaceDescription -like "*Intel*" -and 
        $_.Status -eq "Up" -and
        $_.InterfaceDescription -notlike "*Wi-Fi*" -and
        $_.InterfaceDescription -notlike "*Wireless*"
    })
    
    if ($anyIntelAdapters.Count -gt 0) {
        $selected = $anyIntelAdapters[0]
        Write-StatusMessage "Found Intel adapter: $($selected.Name)" "SUCCESS"
        Write-StatusMessage "Description: $($selected.InterfaceDescription)" "INFO"
        return $selected.Name
    }
    
    # Fallback: Best non-Intel Ethernet adapter (avoid USB/VPN)
    $ethernetAdapters = @(Get-NetAdapter | Where-Object { 
        $_.Status -eq "Up" -and 
        $_.Name -notlike "*Loopback*" -and
        $_.InterfaceDescription -notlike "*VPN*" -and
        $_.InterfaceDescription -notlike "*Virtual*" -and
        $_.InterfaceDescription -notlike "*Wi-Fi*"
    })
    
    if ($ethernetAdapters.Count -gt 0) {
        $selected = $ethernetAdapters[0]
        Write-StatusMessage "Using Ethernet adapter: $($selected.Name)" "WARNING"
        Write-StatusMessage "Description: $($selected.InterfaceDescription)" "INFO"
        Write-StatusMessage "Warning: Non-Intel adapter - limited AVB/TSN support" "WARNING"
        return $selected.Name
    }
    
    # Last resort: Any active adapter
    $activeAdapters = @(Get-NetAdapter | Where-Object { 
        $_.Status -eq "Up" -and 
        $_.Name -notlike "*Loopback*" 
    })
    
    if ($activeAdapters.Count -gt 0) {
        $selected = $activeAdapters[0]
        Write-StatusMessage "Using fallback adapter: $($selected.Name)" "WARNING"
        Write-StatusMessage "Description: $($selected.InterfaceDescription)" "INFO"
        Write-StatusMessage "Warning: This adapter may not support AVB properly" "WARNING"
        return $selected.Name
    }
    
    Write-StatusMessage "No suitable network interface found!" "ERROR"
    return $null
}

function Test-NetworkInterface {
    param([string]$InterfaceName)
    
    if ([string]::IsNullOrEmpty($InterfaceName)) {
        $InterfaceName = Get-BestNetworkInterface
        if (-not $InterfaceName) {
            return $false
        }
    }
    
    Write-StatusMessage "Validating network interface: $InterfaceName" "INFO"
    
    try {
        $adapters = @(Get-NetAdapter | Where-Object { 
            $_.Name -eq $InterfaceName -or 
            $_.Name -like "*$InterfaceName*" -or 
            $_.InterfaceDescription -like "*$InterfaceName*" 
        })
        
        if ($adapters.Count -eq 0) {
            Write-StatusMessage "Network interface '$InterfaceName' not found!" "ERROR"
            Write-StatusMessage "Available interfaces:" "INFO"
            Get-NetAdapter | Where-Object { $_.Status -eq "Up" } | ForEach-Object { 
                Write-StatusMessage "  - $($_.Name) ($($_.InterfaceDescription))" "INFO"
            }
            return $false
        }
        
        $adapter = $adapters[0]
        if ($adapter.Status -ne "Up") {
            Write-StatusMessage "Network interface '$($adapter.Name)' is not up! Status: $($adapter.Status)" "WARNING"
            return $false
        }
        
        # Store the exact adapter name for global use (don't override auto-detection!)
        $script:DetectedInterface = $adapter.Name
        
        Write-StatusMessage "✓ Validated interface: $($adapter.Name)" "SUCCESS"
        Write-StatusMessage "Description: $($adapter.InterfaceDescription)" "INFO"
        if ($adapter.LinkSpeed) {
            Write-StatusMessage "Link Speed: $($adapter.LinkSpeed)" "INFO"
        }
        
        return $true
    }
    catch {
        Write-StatusMessage "Error validating network interface: $($_.Exception.Message)" "ERROR"
        return $false
    }
}

# ============================================================================
# Intel HAL Validation
# ============================================================================

function Test-IntelHalSupport {
    Write-StatusMessage "Checking Intel HAL support..." "INFO"
    
    # Check if Intel Ethernet Controller is present
    $intelAdapters = Get-NetAdapter | Where-Object { 
        $_.InterfaceDescription -like "*Intel*" -and 
        ($_.InterfaceDescription -like "*I219*" -or $_.InterfaceDescription -like "*I225*" -or $_.InterfaceDescription -like "*I226*")
    }
    
    if ($intelAdapters.Count -gt 0) {
        Write-StatusMessage "Intel HAL compatible adapters found:" "SUCCESS"
        $intelAdapters | ForEach-Object { 
            Write-StatusMessage "  - $($_.Name): $($_.InterfaceDescription)" "INFO"
        }
        return $true
    } else {
        Write-StatusMessage "No Intel HAL compatible adapters found. Software fallback will be used." "WARNING"
        return $false
    }
}

# ============================================================================
# Main Startup Sequence
# ============================================================================

function Start-OpenAvnuDaemons {
    Write-StatusMessage "=== OpenAvnu Daemon Startup Framework ===" "INFO"
    
    # Auto-detect network interface if not specified
    if ([string]::IsNullOrEmpty($NetworkInterface)) {
        Write-StatusMessage "No network interface specified, auto-detecting..." "INFO"
        $NetworkInterface = Get-BestNetworkInterface
        if (-not $NetworkInterface) {
            Write-StatusMessage "Failed to auto-detect suitable network interface!" "ERROR"
            return $false
        }
    }
    
    Write-StatusMessage "Target Network Interface: $NetworkInterface" "INFO"
    
    # Pre-flight checks
    if (-not (Test-NetworkInterface $NetworkInterface)) {
        return $false
    }
    
    # Use the detected interface name for daemon arguments
    $effectiveInterface = $script:DetectedInterface
    Write-StatusMessage "Using interface for daemons: $effectiveInterface" "INFO"
    
    Test-IntelHalSupport | Out-Null
    
    # Step 1: Start gPTP Daemon (if not skipped and available)
    if (-not $SkipGptp) {
        $gptpPath = "$BuildDir\gptp\windows_hal\Release\gptp2.exe"
        if (Test-Path $gptpPath) {
            $gptpSuccess = Start-DaemonProcess -Name "gPTP" `
                -ExecutablePath $gptpPath `
                -Arguments @("-F", "$ConfigDir\gptp_cfg.ini") `
                -HealthCheck ${function:Test-GptpReady}
            
            if (-not $gptpSuccess) {
                Write-StatusMessage "gPTP daemon startup failed, but continuing without it..." "WARNING"
            }
        } else {
            Write-StatusMessage "gPTP daemon not found at $gptpPath, skipping..." "WARNING"
            Write-StatusMessage "To build gPTP, run: cmake --build . --target gptp2" "INFO"
        }
    } else {
        Write-StatusMessage "Skipping gPTP daemon startup as requested." "WARNING"
    }
    
    # Step 2: Start MRPD Daemon
    $mrpdSuccess = Start-DaemonProcess -Name "MRPD" `
        -ExecutablePath "$BuildDir\daemons\mrpd\Release\mrpd.exe" `
        -Arguments @("-i", "`"$effectiveInterface`"") `
        -HealthCheck ${function:Test-MrpdReady}
    
    if (-not $mrpdSuccess) {
        Write-StatusMessage "MRPD daemon startup failed!" "ERROR"
        Stop-AllDaemons
        return $false
    }
    
    # Step 3: Start MAAP Daemon
    $maapSuccess = Start-DaemonProcess -Name "MAAP" `
        -ExecutablePath "$BuildDir\daemons\maap\Release\maap_daemon.exe" `
        -HealthCheck ${function:Test-MaapReady}
    
    if (-not $maapSuccess) {
        Write-StatusMessage "MAAP daemon startup failed!" "ERROR"
        Stop-AllDaemons
        return $false
    }
    
    # Step 4: Start Windows Shaper (if available)
    if (Test-Path "$BuildDir\daemons\shaper\Release\openavnu_shaper_windows.exe") {
        $shaperSuccess = Start-DaemonProcess -Name "Shaper" `
            -ExecutablePath "$BuildDir\daemons\shaper\Release\openavnu_shaper_windows.exe" `
            -HealthCheck ${function:Test-ShaperReady}
        
        if (-not $shaperSuccess) {
            Write-StatusMessage "Windows Shaper startup failed, but continuing..." "WARNING"
        }
    } else {
        Write-StatusMessage "Windows Shaper not found, skipping." "WARNING"
    }
    
    Write-StatusMessage "=== OpenAvnu Daemon Startup Complete ===" "SUCCESS"
    Write-StatusMessage "All required daemons are running and ready for ASIO Audio Listener!" "SUCCESS"
    
    return $true
}

function Stop-AllDaemons {
    Write-StatusMessage "Stopping all OpenAvnu daemons..." "INFO"
    
    foreach ($daemon in $Global:DaemonProcesses.Keys) {
        $process = $Global:DaemonProcesses[$daemon]
        if (-not $process.HasExited) {
            Write-StatusMessage "Stopping $daemon daemon..." "INFO"
            try {
                $process.Kill()
                $process.WaitForExit(5000)
                Write-StatusMessage "$daemon daemon stopped." "SUCCESS"
            }
            catch {
                Write-StatusMessage "Error stopping $daemon daemon: $($_.Exception.Message)" "WARNING"
            }
        }
    }
    
    $Global:DaemonProcesses.Clear()
}

function Show-DaemonStatus {
    Write-StatusMessage "=== Daemon Status ===" "INFO"
    
    $daemons = @(
        @{Name="gPTP"; ProcessName="gptp2"; Check=${function:Test-GptpReady}},
        @{Name="MRPD"; ProcessName="mrpd"; Check=${function:Test-MrpdReady}},
        @{Name="MAAP"; ProcessName="maap_daemon"; Check=${function:Test-MaapReady}},
        @{Name="Shaper"; ProcessName="openavnu_shaper_windows"; Check=${function:Test-ShaperReady}}
    )
    
    foreach ($daemon in $daemons) {
        $running = Test-ProcessRunning $daemon.ProcessName
        $ready = if ($daemon.Check) { & $daemon.Check } else { $running }
        
        $status = if ($ready) { "READY" } elseif ($running) { "RUNNING" } else { "STOPPED" }
        $color = if ($ready) { "Green" } elseif ($running) { "Yellow" } else { "Red" }
        
        Write-Host "  $($daemon.Name): $status" -ForegroundColor $color
    }
}

# ============================================================================
# Script Entry Point
# ============================================================================

if ($MyInvocation.InvocationName -ne '.') {
    try {
        if (Start-OpenAvnuDaemons) {
            Show-DaemonStatus
            Write-StatusMessage "OpenAvnu is ready for ASIO Audio Listener!" "SUCCESS"
            Write-StatusMessage "Use 'Stop-AllDaemons' to shutdown all daemons." "INFO"
        } else {
            Write-StatusMessage "OpenAvnu daemon startup failed!" "ERROR"
            exit 1
        }
    }
    catch {
        Write-StatusMessage "Unexpected error: $($_.Exception.Message)" "ERROR"
        Stop-AllDaemons
        exit 1
    }
}
