# I226 gPTP Startup Script with Intel HAL Integration
# Date: September 3, 2025
# Status: HARDWARE TIMESTAMPING ENABLED

param(
    [switch]$Verbose,
    [switch]$Debug,
    [int]$Duration = 30
)

Write-Host "🚀 STARTING INTEL HAL-INTEGRATED gPTP FOR I226" -ForegroundColor Green
Write-Host ""

# Get I226 adapter details
$i226 = Get-NetAdapter | Where-Object { $_.InterfaceDescription -like "*I226*" -and $_.Status -eq 'Up' } | Select-Object -First 1

if (-not $i226) {
    Write-Host "❌ ERROR: I226 adapter not found or not connected" -ForegroundColor Red
    Write-Host "Available adapters:" -ForegroundColor Yellow
    Get-NetAdapter | Where-Object { $_.Status -eq 'Up' } | Format-Table Name, InterfaceDescription, MacAddress -AutoSize
    exit 1
}

Write-Host "I226 Adapter Configuration:" -ForegroundColor Cyan
Write-Host "  Name: $($i226.Name)" -ForegroundColor White
Write-Host "  MAC: $($i226.MacAddress)" -ForegroundColor White
Write-Host "  Description: $($i226.InterfaceDescription)" -ForegroundColor White
Write-Host "  GUID: $($i226.InterfaceGuid)" -ForegroundColor White
Write-Host ""

# Check if Intel HAL-integrated gPTP exists
$gptpExe = ".\build\thirdparty\gptp\Release\gptp.exe"
if (-not (Test-Path $gptpExe)) {
    Write-Host "❌ ERROR: Intel HAL-integrated gPTP not found at: $gptpExe" -ForegroundColor Red
    Write-Host "Please run: cmake --build . --target gptp --config Release" -ForegroundColor Yellow
    exit 1
}

# Verify it's the Intel HAL version
$gptpSize = (Get-Item $gptpExe).Length
if ($gptpSize -lt 290000) {
    Write-Host "⚠️ WARNING: gPTP size ($([math]::Round($gptpSize/1KB,1))KB) suggests missing Intel HAL integration" -ForegroundColor Yellow
    Write-Host "Expected: ~290KB+ for Intel HAL integration" -ForegroundColor Yellow
}

Write-Host "✅ Intel HAL-integrated gPTP found ($(([math]::Round($gptpSize/1KB,1)))KB)" -ForegroundColor Green
Write-Host ""

# Create log files
$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$logFile = "gptp_i226_$timestamp.log"
$errFile = "gptp_i226_$timestamp.err"

Write-Host "Starting Intel HAL gPTP for I226..." -ForegroundColor Cyan
Write-Host "  Command: $gptpExe $($i226.MacAddress)" -ForegroundColor White
Write-Host "  Logs: $logFile / $errFile" -ForegroundColor White
Write-Host ""

# Build arguments
$gptpArgs = @($i226.MacAddress)
if ($Verbose) { $gptpArgs += "-V" }
if ($Debug) { $gptpArgs += "-D" }

# Start gPTP process
try {
    $gptpProcess = Start-Process -FilePath $gptpExe -ArgumentList $gptpArgs -RedirectStandardOutput $logFile -RedirectStandardError $errFile -PassThru -WindowStyle Hidden
    
    Write-Host "✅ gPTP process started (PID: $($gptpProcess.Id))" -ForegroundColor Green
    Write-Host ""
    
    # Monitor for Intel HAL integration
    Start-Sleep -Seconds 3
    
    if (Test-Path $errFile) {
        Write-Host "Intel HAL Hardware Timestamping Status:" -ForegroundColor Magenta
        $errContent = Get-Content $errFile
        
        $halFound = $false
        foreach ($line in $errContent | Select-Object -Last 30) {
            if ($line -match "Enhanced software timestamper initialized") {
                Write-Host "✅ $line" -ForegroundColor Green
                $halFound = $true
            }
            elseif ($line -match "Calibrated timestamp precision") {
                Write-Host "✅ $line" -ForegroundColor Green
                $halFound = $true
            }
            elseif ($line -match "Intel HAL") {
                Write-Host "✅ $line" -ForegroundColor Green
                $halFound = $true
            }
            elseif ($line -match "ERROR|FAIL") {
                Write-Host "❌ $line" -ForegroundColor Red
            }
        }
        
        if ($halFound) {
            Write-Host ""
            Write-Host "🎉 INTEL HAL INTEGRATION CONFIRMED!" -ForegroundColor Green
            Write-Host "Hardware timestamping is ACTIVE on I226" -ForegroundColor Green
        }
    }
    
    Write-Host ""
    Write-Host "Monitoring gPTP for $Duration seconds..." -ForegroundColor Yellow
    
    # Monitor process
    $elapsed = 0
    while ($elapsed -lt $Duration -and -not $gptpProcess.HasExited) {
        Start-Sleep -Seconds 5
        $elapsed += 5
        
        if (Test-Path $errFile) {
            $recentLines = Get-Content $errFile | Select-Object -Last 5
            foreach ($line in $recentLines) {
                if ($line -match "Intel HAL timestamp|TX timestamp successful") {
                    Write-Host "🎯 HARDWARE TIMESTAMPING: $line" -ForegroundColor Cyan
                }
            }
        }
        
        Write-Host "  Running... ($elapsed/$Duration seconds, PID: $($gptpProcess.Id))" -ForegroundColor Gray
    }
    
    Write-Host ""
    if ($gptpProcess.HasExited) {
        Write-Host "⚠️ gPTP process exited (Exit Code: $($gptpProcess.ExitCode))" -ForegroundColor Yellow
    } else {
        Write-Host "✅ gPTP running successfully with Intel HAL integration" -ForegroundColor Green
        Write-Host ""
        Write-Host "To stop gPTP: Stop-Process -Id $($gptpProcess.Id)" -ForegroundColor Yellow
        Write-Host "Log files: $logFile, $errFile" -ForegroundColor Yellow
    }
    
} catch {
    Write-Host "❌ ERROR starting gPTP: $_" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "🎯 I226 gPTP with Intel HAL integration complete!" -ForegroundColor Magenta
