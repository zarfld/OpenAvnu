# Simple Hardware Timestamping Evidence Collection
Write-Host "HARDWARE TIMESTAMPING EVIDENCE COLLECTION" -ForegroundColor Red
Write-Host ""

# Step 1: Check for Intel drivers and capabilities
Write-Host "Step 1: Intel Driver Analysis" -ForegroundColor Cyan

$intelDrivers = Get-WmiObject -Class Win32_SystemDriver | Where-Object { 
    $_.Name -match "Intel" -or $_.Name -match "I21[0-9]" -or $_.Name -match "AVB"
}

if ($intelDrivers) {
    Write-Host "Intel-related drivers found:" -ForegroundColor Green
    foreach ($driver in $intelDrivers) {
        Write-Host "  $($driver.Name): $($driver.State)" -ForegroundColor White
    }
} else {
    Write-Host "No Intel drivers detected" -ForegroundColor Red
}
Write-Host ""

# Step 2: Find Intel adapters
Write-Host "Step 2: Intel Adapter Detection" -ForegroundColor Cyan

$adapters = Get-NetAdapter | Where-Object { 
    $_.InterfaceDescription -match "I21[0-9]|I22[5-6]" -and $_.Status -eq 'Up' 
}

if ($adapters) {
    foreach ($adapter in $adapters) {
        Write-Host "Found: $($adapter.Name) - $($adapter.InterfaceDescription)" -ForegroundColor Green
        Write-Host "  MAC: $($adapter.MacAddress)" -ForegroundColor White
        Write-Host "  Status: $($adapter.Status)" -ForegroundColor White
    }
} else {
    Write-Host "No Intel I210/I219/I225/I226 adapters found" -ForegroundColor Red
    exit 1
}
Write-Host ""

# Step 3: Run gPTP and collect evidence
Write-Host "Step 3: gPTP Hardware Evidence Test" -ForegroundColor Cyan

$testAdapter = $adapters | Select-Object -First 1
$mac = $testAdapter.MacAddress.Replace("-", ":")
$timestamp = (Get-Date).ToString("yyyyMMdd_HHmmss")
$logFile = "hardware_test_$timestamp.err"

Write-Host "Testing with: $($testAdapter.Name) ($mac)" -ForegroundColor Yellow
Write-Host "Log file: $logFile" -ForegroundColor Gray

# Start gPTP
$process = Start-Process -FilePath ".\thirdparty\gptp\build\Release\gptp.exe" -ArgumentList $mac -RedirectStandardError $logFile -NoNewWindow -PassThru

Write-Host "Process started (PID: $($process.Id)). Collecting evidence for 10 seconds..." -ForegroundColor Gray

Start-Sleep -Seconds 10

if (-not $process.HasExited) {
    $process.Kill()
    $process.WaitForExit(3000)
}

Write-Host "Test complete. Analyzing evidence..." -ForegroundColor Gray
Write-Host ""

# Step 4: Analyze evidence
Write-Host "Step 4: Evidence Analysis" -ForegroundColor Cyan

if (Test-Path $logFile) {
    $content = Get-Content $logFile -Raw
    
    # Key evidence patterns
    $hardwarePatterns = @(
        "hardware.*timestamp",
        "Intel.*filter",
        "PTP.*hardware", 
        "SO_TIMESTAMPING",
        "PHC.*clock"
    )
    
    $softwarePatterns = @(
        "software.*timestamp",
        "NDIS.*timestamp",
        "fallback.*software",
        "No hardware.*timestamp"
    )
    
    $accessPatterns = @(
        "Access.*denied",
        "ERROR_ACCESS_DENIED",
        "Error 5",
        "Administrator.*privilege"
    )
    
    Write-Host "Hardware Evidence:" -ForegroundColor Green
    $hardwareCount = 0
    foreach ($pattern in $hardwarePatterns) {
        $found = [regex]::Matches($content, $pattern, [System.Text.RegularExpressions.RegexOptions]::IgnoreCase)
        if ($found.Count -gt 0) {
            Write-Host "  Found: $pattern ($($found.Count) matches)" -ForegroundColor Green
            $hardwareCount += $found.Count
        }
    }
    
    Write-Host "Software Fallback Evidence:" -ForegroundColor Red
    $softwareCount = 0
    foreach ($pattern in $softwarePatterns) {
        $found = [regex]::Matches($content, $pattern, [System.Text.RegularExpressions.RegexOptions]::IgnoreCase)
        if ($found.Count -gt 0) {
            Write-Host "  Found: $pattern ($($found.Count) matches)" -ForegroundColor Red
            $softwareCount += $found.Count
        }
    }
    
    Write-Host "Access Issues:" -ForegroundColor Yellow
    $accessCount = 0
    foreach ($pattern in $accessPatterns) {
        $found = [regex]::Matches($content, $pattern, [System.Text.RegularExpressions.RegexOptions]::IgnoreCase)
        if ($found.Count -gt 0) {
            Write-Host "  Found: $pattern ($($found.Count) matches)" -ForegroundColor Red
            $accessCount += $found.Count
        }
    }
    
    # Check if any content was captured
    $totalLines = ($content -split "`n").Count
    Write-Host "Total log lines: $totalLines" -ForegroundColor Gray
    
} else {
    Write-Host "ERROR: No log file generated" -ForegroundColor Red
    $hardwareCount = 0
    $softwareCount = 0
    $accessCount = 0
}

Write-Host ""
Write-Host "FINAL VERDICT:" -ForegroundColor Red

if ($hardwareCount -gt 0 -and $softwareCount -eq 0) {
    Write-Host "HARDWARE TIMESTAMPING CONFIRMED" -ForegroundColor Green
    Write-Host "Evidence: $hardwareCount hardware indicators found" -ForegroundColor Green
} elseif ($accessCount -gt 0) {
    Write-Host "ACCESS DENIED - Hardware timestamps blocked" -ForegroundColor Red  
    Write-Host "Solution: Run as Administrator or install Intel filter driver" -ForegroundColor Yellow
} elseif ($softwareCount -gt 0) {
    Write-Host "SOFTWARE FALLBACK DETECTED" -ForegroundColor Red
    Write-Host "Hardware timestamps not being used" -ForegroundColor Red
} else {
    Write-Host "INCONCLUSIVE - Need more evidence" -ForegroundColor Yellow
    Write-Host "Check log file manually: $logFile" -ForegroundColor Gray
}

Write-Host ""
Write-Host "HONEST ASSESSMENT: Based on actual log analysis" -ForegroundColor Yellow
