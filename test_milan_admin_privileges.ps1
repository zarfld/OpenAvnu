# Enhanced Milan Compliance Validation with Administrator Privilege Check
# Addresses PDelay transmission failures due to insufficient privileges

param(
    [switch]$DetailedLogging,
    [switch]$CheckPrivileges,
    [switch]$AdminCheck
)

# Check if running as Administrator
function Test-AdministratorPrivileges {
    $currentUser = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = New-Object Security.Principal.WindowsPrincipal($currentUser)
    return $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

# Check Intel HAL requirements
function Test-IntelHALRequirements {
    Write-Host "=== Intel HAL Requirements Check ===" -ForegroundColor Cyan
    
    $isAdmin = Test-AdministratorPrivileges
    Write-Host "Administrator Privileges: $isAdmin" -ForegroundColor $(if ($isAdmin) { "Green" } else { "Red" })
    
    if (-not $isAdmin) {
        Write-Host ""
        Write-Host "CRITICAL: Intel HAL requires Administrator privileges for hardware timestamping" -ForegroundColor Red
        Write-Host "This prevents PDelay request transmission and causes asCapable deadlock" -ForegroundColor Yellow
        Write-Host ""
        Write-Host "To fix:" -ForegroundColor White
        Write-Host "1. Close VS Code" -ForegroundColor Gray
        Write-Host "2. Right-click VS Code icon and 'Run as administrator'" -ForegroundColor Gray
        Write-Host "3. Reopen this workspace" -ForegroundColor Gray
        Write-Host "4. Run this test again" -ForegroundColor Gray
        Write-Host ""
        return $false
    }
    
    return $true
}

# Milan device verification
function Test-MilanDevices {
    Write-Host "=== Milan Device Detection ===" -ForegroundColor Cyan
    
    $milanDevices = @("157.247.3.12", "157.247.1.112")
    $activeDevices = @()
    
    foreach ($device in $milanDevices) {
        $ping = Test-Connection -ComputerName $device -Count 1 -Quiet
        $status = if ($ping) { "ONLINE" } else { "OFFLINE" }
        $color = if ($ping) { "Green" } else { "Red" }
        
        Write-Host "Milan Device $device : $status" -ForegroundColor $color
        if ($ping) { $activeDevices += $device }
    }
    
    if ($activeDevices.Count -eq 0) {
        Write-Host "WARNING: No Milan devices detected online" -ForegroundColor Yellow
        return $false
    }
    
    Write-Host "Active Milan devices: $($activeDevices -join ', ')" -ForegroundColor Green
    return $true
}

# Check Intel I226 adapter
function Get-IntelI226Adapter {
    Write-Host "=== Intel I226 Adapter Detection ===" -ForegroundColor Cyan
    
    $adapters = Get-NetAdapter | Where-Object { 
        $_.InterfaceDescription -like "*I226*" -and $_.Status -eq "Up" 
    }
    
    if (-not $adapters) {
        Write-Host "ERROR: No active Intel I226 adapters found" -ForegroundColor Red
        return $null
    }
    
    $selectedAdapter = $adapters | Select-Object -First 1
    Write-Host "Selected Adapter: $($selectedAdapter.InterfaceDescription)" -ForegroundColor Green
    Write-Host "MAC Address: $($selectedAdapter.MacAddress)" -ForegroundColor White
    Write-Host "Link Speed: $($selectedAdapter.LinkSpeed)" -ForegroundColor White
    
    return $selectedAdapter.MacAddress.Replace("-", ":")
}

# Main execution
Write-Host "Enhanced Milan Compliance Validation with Privilege Check" -ForegroundColor Magenta
Write-Host "=========================================================" -ForegroundColor Magenta
Write-Host ""

# Check administrator privileges first
if (-not (Test-IntelHALRequirements)) {
    Write-Host "STOPPING: Administrator privileges required for Intel HAL" -ForegroundColor Red
    exit 1
}

# Check Milan devices
if (-not (Test-MilanDevices)) {
    Write-Host "WARNING: Limited Milan testing without active devices" -ForegroundColor Yellow
}

# Get Intel adapter
$mac = Get-IntelI226Adapter
if (-not $mac) {
    Write-Host "STOPPING: Intel I226 adapter required" -ForegroundColor Red
    exit 1
}

# gPTP execution with Milan configuration - CORRECTED paths
Write-Host ""
Write-Host "=== Milan gPTP Execution ===" -ForegroundColor Cyan
$gptpBuildPath = ".\thirdparty\gptp\build_gptp\Release"  # Use Release directory
$gptpPath = Join-Path $gptpBuildPath "gptp.exe"
$configPath = Join-Path $gptpBuildPath "test_milan_config.ini"  # Config in Release directory

if (-not (Test-Path $gptpPath)) {
    Write-Host "ERROR: gPTP executable not found at $gptpPath" -ForegroundColor Red
    exit 1
}

if (-not (Test-Path $configPath)) {
    Write-Host "ERROR: Milan config not found at $configPath" -ForegroundColor Red
    exit 1
}

Write-Host "Executing gPTP with Milan specification from Release directory..." -ForegroundColor White
Write-Host "Build Directory: $gptpBuildPath" -ForegroundColor Gray
Write-Host "Command: gptp.exe $mac -f test_milan_config.ini" -ForegroundColor Gray

$logFile = Join-Path $gptpBuildPath "milan_admin_test_output.log"

try {
    # Change to build directory and run gPTP with correct syntax
    Push-Location $gptpBuildPath
    $process = Start-Process -FilePath ".\gptp.exe" -ArgumentList $mac, "-f", "test_milan_config.ini" -NoNewWindow -PassThru -RedirectStandardOutput $logFile -RedirectStandardError "milan_admin_error.log"
    
    Write-Host "gPTP process started with Administrator privileges" -ForegroundColor Green
    Write-Host "Process ID: $($process.Id)" -ForegroundColor White
    Write-Host ""
    
    # Monitor for 60 seconds
    $monitorTime = 60
    Write-Host "Monitoring for $monitorTime seconds..." -ForegroundColor Yellow
    
    for ($i = 1; $i -le $monitorTime; $i++) {
        Start-Sleep -Seconds 1
        
        if ($process.HasExited) {
            Write-Host "Process exited early at $i seconds" -ForegroundColor Red
            break
        }
        
        # Check for asCapable success every 10 seconds
        if ($i % 10 -eq 0 -and (Test-Path $logFile)) {
            $asCapableSuccess = Get-Content $logFile | Select-String -Pattern "asCapable.*true|ANNOUNCE.*START" -Quiet
            if ($asCapableSuccess) {
                Write-Host "SUCCESS: asCapable achieved with Admin privileges!" -ForegroundColor Green
                break
            } else {
                Write-Host "Monitoring... $i/$monitorTime seconds (asCapable: not yet)" -ForegroundColor Yellow
            }
        }
    }
    
    # Stop the process
    if (-not $process.HasExited) {
        $process.Kill()
        Write-Host "gPTP process stopped" -ForegroundColor Gray
    }
    
    # Restore directory
    Pop-Location
    
} catch {
    Write-Host "ERROR executing gPTP: $($_.Exception.Message)" -ForegroundColor Red
    Pop-Location  # Restore directory even on error
    exit 1
}

# Analyze results
Write-Host ""
Write-Host "=== Result Analysis ===" -ForegroundColor Cyan

if (Test-Path $logFile) {
    $logContent = Get-Content $logFile
    
    # Check for privilege improvement
    $privilegeStatus = $logContent | Select-String -Pattern "Privileges.*Administrator|Run as Administrator" | Select-Object -First 1
    if ($privilegeStatus) {
        Write-Host "Privilege Status: $($privilegeStatus.Line)" -ForegroundColor Green
    }
    
    # Check for TX timestamping errors
    $txErrors = $logContent | Select-String -Pattern "Error.*TX.*timestamp|error=87"
    if ($txErrors.Count -eq 0) {
        Write-Host "SUCCESS: No TX timestamping errors detected" -ForegroundColor Green
    } else {
        Write-Host "WARNING: TX timestamping errors still present: $($txErrors.Count)" -ForegroundColor Yellow
    }
    
    # Check for PDelay success
    $pdelaySuccess = $logContent | Select-String -Pattern "PDelay.*successful|asCapable.*true"
    if ($pdelaySuccess.Count -gt 0) {
        Write-Host "SUCCESS: PDelay exchanges successful" -ForegroundColor Green
    } else {
        Write-Host "WARNING: No successful PDelay exchanges detected" -ForegroundColor Yellow
    }
    
    # Check for BMCA operation
    $bmcaAnnounce = $logContent | Select-String -Pattern "ANNOUNCE.*START|announce.*message.*sent"
    if ($bmcaAnnounce.Count -gt 0) {
        Write-Host "SUCCESS: BMCA announce messages started" -ForegroundColor Green
    } else {
        Write-Host "INFO: BMCA announce messages not yet started" -ForegroundColor Yellow
    }
    
} else {
    Write-Host "ERROR: No log file generated" -ForegroundColor Red
}

Write-Host ""
Write-Host "Log files:" -ForegroundColor White
Write-Host "- Output: $logFile" -ForegroundColor Gray
Write-Host "- Errors: .\thirdparty\gptp\milan_admin_error.log" -ForegroundColor Gray
Write-Host ""
Write-Host "Next steps if still failing:" -ForegroundColor White
Write-Host "1. Check Intel driver version and PTP support" -ForegroundColor Gray
Write-Host "2. Verify network interface hardware timestamping capability" -ForegroundColor Gray
Write-Host "3. Consider Intel HAL driver installation requirements" -ForegroundColor Gray