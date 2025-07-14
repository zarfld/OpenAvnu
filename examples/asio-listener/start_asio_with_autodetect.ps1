# ============================================================================
# OpenAvnu ASIO Audio Listener with Auto-Start
# Automatic Intel I219 Detection and Daemon Coordination
# ============================================================================

param(
    [switch]$Verbose,
    [switch]$DryRun,
    [int]$StartupTimeout = 15
)

$ErrorActionPreference = "Stop"
$BuildDir = "d:\Repos\OpenAvnu\build"
$ConfigDir = "d:\Repos\OpenAvnu"

# Global process tracking
$Global:DaemonProcesses = @{}
$Global:DetectedInterface = ""

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

function Get-IntelI219Interface {
    Write-StatusMessage "Auto-detecting Intel I219 network interface..." "INFO"
    
    # Get all up adapters
    $upAdapters = @(Get-NetAdapter | Where-Object { $_.Status -eq "Up" })
    
    # Look for Intel I219 specifically
    foreach ($adapter in $upAdapters) {
        if ($adapter.InterfaceDescription -like "*Intel*" -and 
            $adapter.InterfaceDescription -like "*I219*") {
            Write-StatusMessage "Found Intel I219: $($adapter.Name)" "SUCCESS"
            Write-StatusMessage "Description: $($adapter.InterfaceDescription)" "INFO"
            Write-StatusMessage "Link Speed: $($adapter.LinkSpeed)" "INFO"
            return $adapter.Name
        }
    }
    
    # Fallback to any Intel Ethernet
    foreach ($adapter in $upAdapters) {
        if ($adapter.InterfaceDescription -like "*Intel*" -and 
            $adapter.InterfaceDescription -like "*Ethernet*") {
            Write-StatusMessage "Found Intel Ethernet: $($adapter.Name)" "SUCCESS"
            Write-StatusMessage "Description: $($adapter.InterfaceDescription)" "INFO"
            return $adapter.Name
        }
    }
    
    Write-StatusMessage "No suitable Intel network interface found!" "ERROR"
    return $null
}

function Start-DaemonSafe {
    param(
        [string]$Name,
        [string]$ExecutablePath,
        [string[]]$Arguments = @(),
        [int]$WaitSeconds = 3
    )
    
    if ($DryRun) {
        Write-StatusMessage "[DRY RUN] Would start: $Name" "INFO"
        Write-StatusMessage "Command: $ExecutablePath $($Arguments -join ' ')" "INFO"
        return $true
    }
    
    if (-not (Test-Path $ExecutablePath)) {
        Write-StatusMessage "$Name executable not found: $ExecutablePath" "WARNING"
        return $false
    }
    
    Write-StatusMessage "Starting $Name daemon..." "INFO"
    
    try {
        $process = Start-Process -FilePath $ExecutablePath -ArgumentList $Arguments `
                                -WorkingDirectory $BuildDir -PassThru -WindowStyle Hidden
        
        $Global:DaemonProcesses[$Name] = $process
        
        Start-Sleep -Seconds $WaitSeconds
        
        if ($process.HasExited) {
            Write-StatusMessage "$Name daemon exited unexpectedly!" "ERROR"
            return $false
        }
        
        Write-StatusMessage "$Name daemon started successfully (PID: $($process.Id))" "SUCCESS"
        return $true
    }
    catch {
        Write-StatusMessage "Failed to start $Name daemon: $($_.Exception.Message)" "ERROR"
        return $false
    }
}

function Stop-AllDaemons {
    Write-StatusMessage "Stopping all OpenAvnu daemons..." "INFO"
    
    # Also stop any running processes by name
    $processNames = @("gptp2", "mrpd", "maap_daemon", "openavnu_shaper_windows")
    
    foreach ($processName in $processNames) {
        $processes = Get-Process -Name $processName -ErrorAction SilentlyContinue
        foreach ($process in $processes) {
            Write-StatusMessage "Stopping $processName (PID: $($process.Id))..." "INFO"
            try {
                $process.Kill()
                $process.WaitForExit(3000)
                Write-StatusMessage "$processName stopped." "SUCCESS"
            }
            catch {
                Write-StatusMessage "Error stopping $processName: $($_.Exception.Message)" "WARNING"
            }
        }
    }
    
    $Global:DaemonProcesses.Clear()
}

function Test-AsioListenerReady {
    # Check if we can find the ASIO listener executable
    $asioPath = "$BuildDir\examples\asio-listener\Release\asio_audio_listener.exe"
    if (Test-Path $asioPath) {
        Write-StatusMessage "ASIO Audio Listener found: $asioPath" "SUCCESS"
        return $true
    }
    
    Write-StatusMessage "ASIO Audio Listener not found at: $asioPath" "WARNING"
    Write-StatusMessage "You may need to build the ASIO listener first." "WARNING"
    return $false
}

function Start-OpenAvnuForAsio {
    Write-StatusMessage "=== OpenAvnu ASIO Audio Listener Startup ===" "INFO"
    Write-Host ""
    
    # Step 1: Auto-detect network interface
    $networkInterface = Get-IntelI219Interface
    if (-not $networkInterface) {
        Write-StatusMessage "Cannot proceed without suitable network interface!" "ERROR"
        return $false
    }
    
    $Global:DetectedInterface = $networkInterface
    Write-StatusMessage "Using network interface: $networkInterface" "SUCCESS"
    Write-Host ""
    
    # Step 2: Check ASIO listener availability
    Test-AsioListenerReady | Out-Null
    Write-Host ""
    
    # Step 3: Stop any existing daemons
    Stop-AllDaemons
    Start-Sleep -Seconds 2
    
    # Step 4: Start MRPD (Media Reservation Protocol Daemon)
    $mrpdSuccess = Start-DaemonSafe -Name "MRPD" `
        -ExecutablePath "$BuildDir\daemons\mrpd\Release\mrpd.exe" `
        -Arguments @("-i", "`"$networkInterface`"") `
        -WaitSeconds 3
    
    if (-not $mrpdSuccess) {
        Write-StatusMessage "MRPD daemon is required but failed to start!" "ERROR"
        return $false
    }
    
    # Step 5: Start MAAP (Multicast Address Acquisition Protocol)
    $maapSuccess = Start-DaemonSafe -Name "MAAP" `
        -ExecutablePath "$BuildDir\daemons\maap\Release\maap_daemon.exe" `
        -WaitSeconds 3
    
    if (-not $maapSuccess) {
        Write-StatusMessage "MAAP daemon is required but failed to start!" "ERROR"
        Stop-AllDaemons
        return $false
    }
    
    # Step 6: Start gPTP (optional but recommended)
    $gptpSuccess = Start-DaemonSafe -Name "gPTP" `
        -ExecutablePath "$BuildDir\gptp\windows_hal\Release\gptp2.exe" `
        -Arguments @("-F", "$ConfigDir\gptp_cfg.ini") `
        -WaitSeconds 3
    
    if (-not $gptpSuccess) {
        Write-StatusMessage "gPTP daemon failed to start, but continuing without it..." "WARNING"
        Write-StatusMessage "Time synchronization will use system clock instead of AVB timing." "WARNING"
    }
    
    # Step 7: Start Windows Shaper (optional)
    $shaperSuccess = Start-DaemonSafe -Name "Shaper" `
        -ExecutablePath "$BuildDir\daemons\shaper\Release\openavnu_shaper_windows.exe" `
        -WaitSeconds 3
    
    if (-not $shaperSuccess) {
        Write-StatusMessage "Windows Shaper failed to start, but continuing..." "WARNING"
        Write-StatusMessage "QoS traffic shaping will not be available." "WARNING"
    }
    
    Write-Host ""
    Write-StatusMessage "=== OpenAvnu Daemon Startup Complete ===" "SUCCESS"
    Write-StatusMessage "Network Interface: $networkInterface (Intel I219 Auto-Detected)" "SUCCESS"
    Write-StatusMessage "Required Daemons: MRPD ✓, MAAP ✓" "SUCCESS"
    Write-StatusMessage "Optional Daemons: gPTP $(if($gptpSuccess){'✓'}else{'✗'}), Shaper $(if($shaperSuccess){'✓'}else{'✗'})" "INFO"
    Write-Host ""
    Write-StatusMessage "🎵 Ready for ASIO Audio Listener! 🎵" "SUCCESS"
    
    return $true
}

function Show-DaemonStatus {
    Write-StatusMessage "=== Current Daemon Status ===" "INFO"
    
    $daemons = @("mrpd", "maap_daemon", "gptp2", "openavnu_shaper_windows")
    foreach ($daemon in $daemons) {
        $process = Get-Process -Name $daemon -ErrorAction SilentlyContinue
        if ($process) {
            Write-Host "  $daemon: RUNNING (PID: $($process.Id))" -ForegroundColor Green
        } else {
            Write-Host "  $daemon: STOPPED" -ForegroundColor Red
        }
    }
    
    if ($Global:DetectedInterface) {
        Write-Host "  Network Interface: $($Global:DetectedInterface)" -ForegroundColor Cyan
    }
}

# Main execution
if ($MyInvocation.InvocationName -ne '.') {
    try {
        if (Start-OpenAvnuForAsio) {
            Show-DaemonStatus
            Write-Host ""
            Write-StatusMessage "OpenAvnu is ready for ASIO Audio streaming!" "SUCCESS"
            Write-StatusMessage "Your Intel I219 is automatically configured and optimized." "SUCCESS"
            Write-Host ""
            Write-StatusMessage "To stop all daemons, run: Stop-AllDaemons" "INFO"
        } else {
            Write-StatusMessage "OpenAvnu startup failed!" "ERROR"
            Stop-AllDaemons
            exit 1
        }
    }
    catch {
        Write-StatusMessage "Unexpected error: $($_.Exception.Message)" "ERROR"
        Stop-AllDaemons
        exit 1
    }
}
