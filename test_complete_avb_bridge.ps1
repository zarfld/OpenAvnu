# ============================================================================
# Complete AVB Audio Bridge Test Script
# ============================================================================
# Tests the full AVB audio pipeline with Windows integration

param(
    [switch]$StartDaemons,
    [switch]$TestOnly,
    [switch]$StopAll
)

Write-Host ""
Write-Host "🌉 Complete AVB Audio Bridge Test 🌉" -ForegroundColor Cyan
Write-Host "=====================================" -ForegroundColor Cyan
Write-Host ""

if ($StopAll) {
    Write-Host "🛑 Stopping all AVB processes..." -ForegroundColor Yellow
    Get-Process | Where-Object {$_.ProcessName -in @("simple_avb_audio_bridge", "mrpd", "maap_daemon")} | Stop-Process -Force -ErrorAction SilentlyContinue
    Write-Host "✅ All AVB processes stopped" -ForegroundColor Green
    return
}

# Test system components
Write-Host "🔍 System Component Check:" -ForegroundColor Yellow

# Check network adapters
$intelAdapter = Get-NetAdapter | Where-Object {$_.InterfaceDescription -like "*Intel*I219*" -and $_.Status -eq "Up"}
if ($intelAdapter) {
    Write-Host "✅ Intel I219-LM adapter detected: $($intelAdapter.Name)" -ForegroundColor Green
    Write-Host "   Status: $($intelAdapter.Status), Speed: $($intelAdapter.LinkSpeed)" -ForegroundColor Gray
} else {
    Write-Host "⚠️  Intel I219-LM adapter not found or not connected" -ForegroundColor Yellow
}

# Check Virtual Audio Cable
$vbAudio = Get-WmiObject -Class Win32_SoundDevice | Where-Object {$_.Name -like "*CABLE*"}
if ($vbAudio) {
    Write-Host "✅ Virtual Audio Cable detected: $($vbAudio.Name)" -ForegroundColor Green
} else {
    Write-Host "❌ Virtual Audio Cable not installed" -ForegroundColor Red
    Write-Host "   Download from: https://vb-audio.com/Cable/" -ForegroundColor Cyan
}

# Check built executables
$bridgeExe = ".\build\examples\Release\simple_avb_audio_bridge.exe"
$mrpdExe = ".\build\daemons\mrpd\Release\mrpd.exe"
$maapExe = ".\build\daemons\maap\Release\maap_daemon.exe"

if (Test-Path $bridgeExe) {
    Write-Host "✅ AVB Audio Bridge built successfully" -ForegroundColor Green
} else {
    Write-Host "❌ AVB Audio Bridge not built" -ForegroundColor Red
    Write-Host "   Run: cmake --build build --config Release --target simple_avb_audio_bridge" -ForegroundColor Cyan
}

if (Test-Path $mrpdExe) {
    Write-Host "✅ MRPD daemon available" -ForegroundColor Green
} else {
    Write-Host "⚠️  MRPD daemon not built" -ForegroundColor Yellow
}

if (Test-Path $maapExe) {
    Write-Host "✅ MAAP daemon available" -ForegroundColor Green
} else {
    Write-Host "⚠️  MAAP daemon not built" -ForegroundColor Yellow
}

if ($TestOnly) {
    Write-Host ""
    Write-Host "📋 Test Summary:" -ForegroundColor Cyan
    Write-Host "  - Network: $(if($intelAdapter){'Ready'}else{'Check connection'})" -ForegroundColor $(if($intelAdapter){'Green'}else{'Yellow'})
    Write-Host "  - Audio: $(if($vbAudio){'Ready'}else{'Install VB-Audio Cable'})" -ForegroundColor $(if($vbAudio){'Green'}else{'Red'})
    Write-Host "  - Bridge: $(if(Test-Path $bridgeExe){'Ready'}else{'Build required'})" -ForegroundColor $(if(Test-Path $bridgeExe){'Green'}else{'Red'})
    return
}

Write-Host ""
Write-Host "🚀 Starting AVB Audio Bridge Demo..." -ForegroundColor Green

# Start daemons if requested
if ($StartDaemons) {
    Write-Host "🔧 Starting OpenAvnu daemons..." -ForegroundColor Yellow
    
    if (Test-Path $mrpdExe) {
        Write-Host "   Starting MRPD daemon..." -ForegroundColor Gray
        Start-Process -FilePath $mrpdExe -WorkingDirectory ".\build\daemons\mrpd\Release" -WindowStyle Minimized
        Start-Sleep 2
    }
    
    if (Test-Path $maapExe) {
        Write-Host "   Starting MAAP daemon..." -ForegroundColor Gray
        Start-Process -FilePath $maapExe -WorkingDirectory ".\build\daemons\maap\Release" -WindowStyle Minimized
        Start-Sleep 2
    }
}

# Start AVB Audio Bridge
if (Test-Path $bridgeExe) {
    Write-Host "🌉 Starting AVB Audio Bridge..." -ForegroundColor Cyan
    Write-Host "   Listening for AVB streams on 239.69.69.69:17220" -ForegroundColor Gray
    Write-Host "   Audio will be routed to Virtual Audio Cable" -ForegroundColor Gray
    Write-Host "   Press Ctrl+C in bridge window to stop" -ForegroundColor Gray
    Write-Host ""
    
    # Start the bridge in a new window
    Start-Process -FilePath $bridgeExe -WorkingDirectory ".\build\examples\Release"
    
    Write-Host "✅ AVB Audio Bridge started successfully!" -ForegroundColor Green
    Write-Host ""
    Write-Host "🎵 Next Steps:" -ForegroundColor Cyan
    Write-Host "  1. Configure your DAW to use Virtual Audio Cable:" -ForegroundColor White
    Write-Host "     - Input: 'CABLE Output' (receives from AVB)" -ForegroundColor Green
    Write-Host "     - Output: 'CABLE Input' (sends to AVB - future feature)" -ForegroundColor Green
    Write-Host "  2. Start an AVB talker on your network" -ForegroundColor White
    Write-Host "  3. Configure talker to send to 239.69.69.69:17220" -ForegroundColor White
    Write-Host "  4. Audio should appear in your DAW!" -ForegroundColor White
    Write-Host ""
    Write-Host "🛑 To stop: .\test_complete_avb_bridge.ps1 -StopAll" -ForegroundColor Yellow
} else {
    Write-Host "❌ Cannot start bridge - executable not found" -ForegroundColor Red
    Write-Host "   Build it first with: cmake --build build --config Release" -ForegroundColor Cyan
}

Write-Host ""
