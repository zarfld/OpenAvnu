# ============================================================================
# Verify ASIO Integration Environment Script
# ============================================================================

Write-Host ""
Write-Host "🔍 ASIO Integration Environment Verification" -ForegroundColor Cyan
Write-Host "==============================================" -ForegroundColor Cyan
Write-Host ""

# 1. Intel Network Adapter Detection
Write-Host "1. Intel Network Adapter Detection:" -ForegroundColor White
Write-Host "───────────────────────────────────────" -ForegroundColor Gray

$intelAdapters = Get-NetAdapter | Where-Object { $_.InterfaceDescription -like '*Intel*' }
if ($intelAdapters) {
    $intelAdapters | Select-Object Name, InterfaceDescription, Status, LinkSpeed | Format-Table -AutoSize
    Write-Host "✅ Found $($intelAdapters.Count) Intel adapter(s)" -ForegroundColor Green
} else {
    Write-Host "❌ No Intel adapters found" -ForegroundColor Red
}

Write-Host ""

# 2. OpenAvnu Executable Status
Write-Host "2. OpenAvnu Executable Status:" -ForegroundColor White
Write-Host "─────────────────────────────────" -ForegroundColor Gray

$executables = @{
    "MRPD" = "daemons\mrpd\mrpd.exe"
    "MAAP" = "daemons\maap\daemon\maap_daemon.exe"
}

foreach ($exe in $executables.GetEnumerator()) {
    if (Test-Path $exe.Value) {
        $fileInfo = Get-Item $exe.Value
        Write-Host "✅ $($exe.Key): $($exe.Value)" -ForegroundColor Green
        Write-Host "   Size: $([math]::Round($fileInfo.Length/1KB,2)) KB, Modified: $($fileInfo.LastWriteTime)" -ForegroundColor Gray
    } else {
        Write-Host "❌ $($exe.Key): $($exe.Value) [MISSING]" -ForegroundColor Red
    }
}

Write-Host ""

# 3. ASIO Listener Build Status
Write-Host "3. ASIO Listener Build Status:" -ForegroundColor White
Write-Host "─────────────────────────────────" -ForegroundColor Gray

$asioPath = "examples\asio-listener\asio_listener.exe"
if (Test-Path $asioPath) {
    $fileInfo = Get-Item $asioPath
    Write-Host "✅ ASIO Listener built: $asioPath" -ForegroundColor Green
    Write-Host "   Size: $([math]::Round($fileInfo.Length/1KB,2)) KB, Modified: $($fileInfo.LastWriteTime)" -ForegroundColor Gray
} else {
    Write-Host "❌ ASIO Listener needs building: $asioPath" -ForegroundColor Yellow
    
    # Check for build scripts
    $buildScripts = @("examples\asio-listener\build.bat", "examples\asio-listener\build_simple.bat")
    foreach ($script in $buildScripts) {
        if (Test-Path $script) {
            Write-Host "   Build script available: $script" -ForegroundColor Blue
        }
    }
}

Write-Host ""

# 4. PCAP Library Status
Write-Host "4. PCAP Library Status:" -ForegroundColor White
Write-Host "─────────────────────────" -ForegroundColor Gray

$pcapEnvVars = @("NPCAP_DIR", "NPCAP_SDK_DIR", "WPCAP_DIR")
$pcapFound = $false

foreach ($envVar in $pcapEnvVars) {
    $value = [Environment]::GetEnvironmentVariable($envVar)
    if ($value) {
        Write-Host "✅ $envVar = $value" -ForegroundColor Green
        if (Test-Path $value) {
            Write-Host "   Directory exists" -ForegroundColor Gray
        } else {
            Write-Host "   ⚠️  Directory not found" -ForegroundColor Yellow
        }
        $pcapFound = $true
    }
}

if (-not $pcapFound) {
    Write-Host "⚠️  No PCAP environment variables set" -ForegroundColor Yellow
    Write-Host "   OpenAvnu may still work with auto-detection" -ForegroundColor Gray
}

Write-Host ""

# 5. Current Daemon Status
Write-Host "5. Current Daemon Status:" -ForegroundColor White
Write-Host "──────────────────────────" -ForegroundColor Gray

$runningDaemons = Get-Process -Name "mrpd","maap_daemon" -ErrorAction SilentlyContinue
if ($runningDaemons) {
    Write-Host "✅ Daemons currently running:" -ForegroundColor Green
    $runningDaemons | Select-Object ProcessName, Id, @{Name='Memory(MB)';Expression={[math]::Round($_.WorkingSet/1MB,2)}} | Format-Table -AutoSize
} else {
    Write-Host "ℹ️  No OpenAvnu daemons currently running" -ForegroundColor Blue
}

Write-Host ""

# 6. Build System Status
Write-Host "6. Build System Status:" -ForegroundColor White
Write-Host "─────────────────────────" -ForegroundColor Gray

if (Test-Path "build") {
    Write-Host "✅ Build directory exists" -ForegroundColor Green
    
    # Check for CMake cache
    if (Test-Path "build\CMakeCache.txt") {
        Write-Host "✅ CMake configured" -ForegroundColor Green
    } else {
        Write-Host "⚠️  CMake not configured" -ForegroundColor Yellow
    }
} else {
    Write-Host "❌ Build directory missing" -ForegroundColor Red
}

Write-Host ""

# Summary
Write-Host "🔍 Environment Verification Summary:" -ForegroundColor Cyan
Write-Host "────────────────────────────────────" -ForegroundColor Gray

$issues = 0
if (-not $intelAdapters) { $issues++ }
if (-not (Test-Path "daemons\mrpd\mrpd.exe")) { $issues++ }
if (-not (Test-Path "daemons\maap\daemon\maap_daemon.exe")) { $issues++ }

if ($issues -eq 0) {
    Write-Host "✅ Environment looks good! Ready for ASIO testing." -ForegroundColor Green
} elseif ($issues -eq 1) {
    Write-Host "⚠️  1 issue found - may need attention" -ForegroundColor Yellow
} else {
    Write-Host "❌ $issues issues found - please resolve before testing" -ForegroundColor Red
}

Write-Host ""
Write-Host "🔍 Verification complete!" -ForegroundColor Cyan
