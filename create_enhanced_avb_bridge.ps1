# ============================================================================
# Enhanced AVB Audio Bridge Setup - Practical Implementation
# ============================================================================

param(
    [switch]$InstallVirtualCable,
    [switch]$TestOnly,
    [switch]$StartDaemons,
    [string]$AudioDevice = "CABLE Input",
    [int]$Duration = 60
)

Write-Host ""
Write-Host "🌉 Enhanced AVB Audio Bridge Setup 🌉" -ForegroundColor Magenta
Write-Host "=======================================" -ForegroundColor Magenta
Write-Host "Routes AVB network audio to Windows audio system" -ForegroundColor White
Write-Host ""

# Function to check if Virtual Audio Cable is installed
function Test-VirtualAudioCable {
    try {
        $devices = Get-WmiObject -Class Win32_SoundDevice -ErrorAction SilentlyContinue | Where-Object { 
            $_.Name -like "*VB-Audio*" -or $_.Name -like "*CABLE*" 
        }
        return ($devices.Count -gt 0)
    } catch {
        return $false
    }
}

# Function to check if OpenAvnu daemons are running
function Test-OpenAvnuDaemons {
    $daemons = Get-Process -Name "mrpd","maap_daemon" -ErrorAction SilentlyContinue
    return ($daemons.Count -gt 0)
}

# Install Virtual Audio Cable guide
if ($InstallVirtualCable) {
    Write-Host "📦 Virtual Audio Cable Installation Guide:" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "🔗 Step 1: Download VB-Audio Virtual Cable" -ForegroundColor Yellow
    Write-Host "   URL: https://vb-audio.com/Cable/" -ForegroundColor Cyan
    Write-Host "   File: VBCABLE_Driver_Pack43.zip (or latest)" -ForegroundColor Gray
    Write-Host ""
    Write-Host "🔧 Step 2: Installation Process" -ForegroundColor Yellow
    Write-Host "   1. Extract the downloaded ZIP file" -ForegroundColor White
    Write-Host "   2. Run VBCABLE_Setup_x64.exe as Administrator" -ForegroundColor White
    Write-Host "   3. Follow installation wizard" -ForegroundColor White
    Write-Host "   4. Restart your computer when prompted" -ForegroundColor White
    Write-Host ""
    Write-Host "✅ Step 3: Verification After Restart" -ForegroundColor Yellow
    Write-Host "   Check Windows Sound Settings for:" -ForegroundColor White
    Write-Host "   - Recording: 'CABLE Output' device" -ForegroundColor Green
    Write-Host "   - Playback: 'CABLE Input' device" -ForegroundColor Green
    Write-Host ""
    Write-Host "🎵 Step 4: Run this script again" -ForegroundColor Yellow
    Write-Host "   .\create_enhanced_avb_bridge.ps1" -ForegroundColor Cyan
    Write-Host ""
    return
}

# Check system requirements
Write-Host "🔍 Checking System Requirements..." -ForegroundColor Yellow
Write-Host ""

# Check Virtual Audio Cable
if (Test-VirtualAudioCable) {
    Write-Host "✅ Virtual Audio Cable detected" -ForegroundColor Green
    $vbAudioOK = $true
} else {
    Write-Host "❌ Virtual Audio Cable not found!" -ForegroundColor Red
    Write-Host "   Run: .\create_enhanced_avb_bridge.ps1 -InstallVirtualCable" -ForegroundColor Yellow
    $vbAudioOK = $false
}

# Check OpenAvnu daemons
if (Test-OpenAvnuDaemons) {
    Write-Host "✅ OpenAvnu daemons running" -ForegroundColor Green
    $daemonsOK = $true
} else {
    Write-Host "⚠️  OpenAvnu daemons not running" -ForegroundColor Yellow
    if ($StartDaemons) {
        Write-Host "🚀 Starting OpenAvnu daemons..." -ForegroundColor Cyan
        try {
            Start-Process -FilePath ".\start_openavnu_final.ps1" -NoNewWindow -Wait
            Start-Sleep -Seconds 3
            if (Test-OpenAvnuDaemons) {
                Write-Host "✅ OpenAvnu daemons started successfully" -ForegroundColor Green
                $daemonsOK = $true
            } else {
                Write-Host "❌ Failed to start OpenAvnu daemons" -ForegroundColor Red
                $daemonsOK = $false
            }
        } catch {
            Write-Host "❌ Error starting daemons: $($_.Exception.Message)" -ForegroundColor Red
            $daemonsOK = $false
        }
    } else {
        Write-Host "   Run: .\create_enhanced_avb_bridge.ps1 -StartDaemons" -ForegroundColor Yellow
        $daemonsOK = $false
    }
}

# Check for compiled bridge
$simpleBridge = ".\build\examples\Release\simple_avb_audio_bridge.exe"
$advancedBridge = ".\build\examples\Release\avb_audio_bridge.exe"

if (Test-Path $simpleBridge) {
    Write-Host "✅ Simple AVB Audio Bridge available" -ForegroundColor Green
    $bridgeOK = $true
    $bridgeExe = $simpleBridge
} elseif (Test-Path $advancedBridge) {
    Write-Host "✅ Advanced AVB Audio Bridge available" -ForegroundColor Green
    $bridgeOK = $true
    $bridgeExe = $advancedBridge
} else {
    Write-Host "❌ AVB Audio Bridge not compiled!" -ForegroundColor Red
    Write-Host "   Run CMake Configure and Build tasks in VS Code" -ForegroundColor Yellow
    $bridgeOK = $false
}

if ($TestOnly) {
    Write-Host ""
    Write-Host "📋 System Status Summary:" -ForegroundColor Cyan
    Write-Host "   Virtual Audio Cable: $(if($vbAudioOK){'✅ Ready'}else{'❌ Missing'})" -ForegroundColor $(if($vbAudioOK){'Green'}else{'Red'})
    Write-Host "   OpenAvnu Daemons: $(if($daemonsOK){'✅ Running'}else{'❌ Stopped'})" -ForegroundColor $(if($daemonsOK){'Green'}else{'Red'})
    Write-Host "   AVB Audio Bridge: $(if($bridgeOK){'✅ Available'}else{'❌ Missing'})" -ForegroundColor $(if($bridgeOK){'Green'}else{'Red'})
    Write-Host ""
    if ($vbAudioOK -and $daemonsOK -and $bridgeOK) {
        Write-Host "🎉 System ready for AVB Audio Bridge!" -ForegroundColor Green
    } else {
        Write-Host "⚠️  System requires setup before running AVB Audio Bridge" -ForegroundColor Yellow
    }
    return
}

# Proceed only if basic requirements are met
if (-not $bridgeOK) {
    Write-Host ""
    Write-Host "❌ Cannot proceed without compiled AVB Audio Bridge" -ForegroundColor Red
    Write-Host "Please build the project first using VS Code tasks" -ForegroundColor Yellow
    return
}

Write-Host ""
Write-Host "🌉 Starting AVB Audio Bridge..." -ForegroundColor Cyan
Write-Host ""
Write-Host "Configuration:" -ForegroundColor White
Write-Host "  Bridge: $bridgeExe" -ForegroundColor Gray
Write-Host "  Duration: $Duration seconds" -ForegroundColor Gray
Write-Host "  Virtual Audio: $(if($vbAudioOK){'Available'}else{'Not Available'})" -ForegroundColor Gray
Write-Host ""

if (-not $vbAudioOK) {
    Write-Host "⚠️  Running without Virtual Audio Cable" -ForegroundColor Yellow
    Write-Host "   Audio will be demonstrated but not routed to Windows" -ForegroundColor Yellow
    Write-Host ""
}

Write-Host "🚀 Launching AVB Audio Bridge..." -ForegroundColor Green
Write-Host "   Press Ctrl+C in the bridge window to stop" -ForegroundColor Gray
Write-Host ""

# Start the AVB Audio Bridge
try {
    if ($Duration -eq 60) {
        # Run indefinitely
        & $bridgeExe
    } else {
        # Run for specified duration
        $job = Start-Job -ScriptBlock { 
            param($exe)
            & $exe
        } -ArgumentList $bridgeExe
        
        Wait-Job $job -Timeout $Duration
        Stop-Job $job -Force
        Remove-Job $job -Force
        
        Write-Host ""
        Write-Host "⏰ Bridge stopped after $Duration seconds" -ForegroundColor Yellow
    }
} catch {
    Write-Host "❌ Error running AVB Audio Bridge: $($_.Exception.Message)" -ForegroundColor Red
}

Write-Host ""
Write-Host "🎵 AVB Audio Bridge Session Complete!" -ForegroundColor Magenta
Write-Host ""

if ($vbAudioOK) {
    Write-Host "📝 Next Steps for Audio Applications:" -ForegroundColor Cyan
    Write-Host "  1. Open your DAW or audio application" -ForegroundColor White
    Write-Host "  2. Set audio input to 'CABLE Output'" -ForegroundColor White  
    Write-Host "  3. Set audio output to 'CABLE Input'" -ForegroundColor White
    Write-Host "  4. AVB audio streams will be available!" -ForegroundColor White
} else {
    Write-Host "📝 To Enable Full Windows Audio Integration:" -ForegroundColor Cyan
    Write-Host "  1. Install Virtual Audio Cable" -ForegroundColor White
    Write-Host "  2. Run: .\create_enhanced_avb_bridge.ps1 -InstallVirtualCable" -ForegroundColor White
    Write-Host "  3. Follow installation instructions" -ForegroundColor White
    Write-Host "  4. Restart and run this script again" -ForegroundColor White
}

Write-Host ""
Write-Host "🔗 Useful Commands:" -ForegroundColor Cyan
Write-Host "  Test system: .\create_enhanced_avb_bridge.ps1 -TestOnly" -ForegroundColor White
Write-Host "  Start daemons: .\create_enhanced_avb_bridge.ps1 -StartDaemons" -ForegroundColor White
Write-Host "  Install guide: .\create_enhanced_avb_bridge.ps1 -InstallVirtualCable" -ForegroundColor White
Write-Host ""
