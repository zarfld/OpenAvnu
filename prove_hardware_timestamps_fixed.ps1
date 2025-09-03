# Hardware Timestamping Evidence Script - Fixed Version
param(
    [int]$TestDuration = 15,
    [switch]$DetailedAnalysis
)

Write-Host "HARDWARE TIMESTAMPING EVIDENCE COLLECTION" -ForegroundColor Red
Write-Host "Proving actual hardware timestamp usage vs software fallback" -ForegroundColor Yellow
Write-Host ""

# Check for Intel AVB filter driver - Key evidence of hardware timestamping
Write-Host "=== PHASE 1: Intel Driver Analysis ===" -ForegroundColor Cyan

$intelDrivers = Get-WmiObject -Class Win32_SystemDriver | Where-Object { 
    $_.Name -match "Intel.*AVB|I21[0-9]|AVB" 
}

Write-Host "Intel AVB-related drivers:" -ForegroundColor Yellow
if ($intelDrivers) {
    foreach ($driver in $intelDrivers) {
        $status = if ($driver.State -eq "Running") {"ACTIVE"} else {"INACTIVE"}
        $color = if ($driver.State -eq "Running") {"Green"} else {"Red"}
        Write-Host "  $($driver.Name): $status" -ForegroundColor $color
    }
} else {
    Write-Host "  No Intel AVB drivers detected" -ForegroundColor Red
}
Write-Host ""

# Check network adapter advanced properties for timestamping features
Write-Host "=== PHASE 2: Network Adapter Timestamping Features ===" -ForegroundColor Cyan

$adapters = Get-NetAdapter | Where-Object { 
    $_.InterfaceDescription -match "I21[0-9]|I22[5-6]" -and $_.Status -eq 'Up' 
}

foreach ($adapter in $adapters) {
    Write-Host "Adapter: $($adapter.Name) ($($adapter.InterfaceDescription))" -ForegroundColor Yellow
    
    # Get advanced properties related to timestamping
    $timestampProps = Get-NetAdapterAdvancedProperty -Name $adapter.Name -ErrorAction SilentlyContinue | 
                      Where-Object { $_.DisplayName -match "timestamp|PTP|AVB|Precision|Time|Clock" }
    
    if ($timestampProps) {
        Write-Host "  Timestamp-related properties:" -ForegroundColor Green
        foreach ($prop in $timestampProps) {
            Write-Host "    $($prop.DisplayName): $($prop.DisplayValue)" -ForegroundColor White
        }
    } else {
        Write-Host "  No timestamp properties found" -ForegroundColor Red
    }
    Write-Host ""
}

# Run gPTP test and analyze output for hardware timestamp evidence
Write-Host "=== PHASE 3: Live gPTP Hardware Evidence Collection ===" -ForegroundColor Cyan

$testAdapter = $adapters | Select-Object -First 1
if (-not $testAdapter) {
    Write-Host "ERROR: No Intel adapters available for testing" -ForegroundColor Red
    exit 1
}

$mac = $testAdapter.MacAddress.Replace("-", ":")
Write-Host "Testing with: $($testAdapter.Name) ($mac)" -ForegroundColor Yellow

$timestamp = (Get-Date).ToString("yyyyMMdd_HHmmss")
$logFile = "timestamp_evidence_$timestamp.err"

Write-Host "Starting gPTP evidence collection..." -ForegroundColor Gray

$process = Start-Process -FilePath ".\thirdparty\gptp\build\Release\gptp.exe" `
                        -ArgumentList $mac `
                        -RedirectStandardError $logFile `
                        -NoNewWindow -PassThru

Start-Sleep -Seconds $TestDuration

if (-not $process.HasExited) {
    $process.Kill()
    $process.WaitForExit(3000)
}

Write-Host "Evidence collection complete. Analyzing logs..." -ForegroundColor Gray
Write-Host ""

# Analyze the log for hardware vs software timestamp evidence
Write-Host "=== PHASE 4: Log Analysis Results ===" -ForegroundColor Cyan

if (Test-Path $logFile) {
    $content = Get-Content $logFile -Raw
    
    # Hardware timestamp indicators
    $hardwareIndicators = @(
        "hardware.*timestamp",
        "Intel.*filter",
        "PTP.*hardware",
        "SO_TIMESTAMPING",
        "SIOCGTSTAMP",
        "PHC.*clock"
    )
    
    # Software fallback indicators  
    $softwareIndicators = @(
        "software.*timestamp",
        "NDIS.*timestamp", 
        "GetSystemTimeAsFileTime",
        "fallback.*software",
        "No hardware.*timestamp"
    )
    
    Write-Host "Hardware Timestamp Evidence:" -ForegroundColor Green
    $hardwareCount = 0
    foreach ($pattern in $hardwareIndicators) {
        $patternMatches = [regex]::Matches($content, $pattern, [System.Text.RegularExpressions.RegexOptions]::IgnoreCase)
        if ($patternMatches.Count -gt 0) {
            Write-Host "  ✅ $pattern : $($patternMatches.Count) matches" -ForegroundColor Green
            $hardwareCount += $patternMatches.Count
        }
    }
    
    Write-Host "Software Fallback Evidence:" -ForegroundColor Red
    $softwareCount = 0
    foreach ($pattern in $softwareIndicators) {
        $patternMatches = [regex]::Matches($content, $pattern, [System.Text.RegularExpressions.RegexOptions]::IgnoreCase)
        if ($patternMatches.Count -gt 0) {
            Write-Host "  ⚠️ $pattern : $($patternMatches.Count) matches" -ForegroundColor Red
            $softwareCount += $patternMatches.Count
        }
    }
    
    # Look for specific error messages that indicate hardware access issues
    Write-Host "Access Permission Analysis:" -ForegroundColor Yellow
    $permissionIssues = @(
        "Access.*denied",
        "ERROR_ACCESS_DENIED", 
        "Error 5",
        "Administrator.*privilege",
        "filter.*driver.*not.*loaded"
    )
    
    $accessIssues = 0
    foreach ($pattern in $permissionIssues) {
        $patternMatches = [regex]::Matches($content, $pattern, [System.Text.RegularExpressions.RegexOptions]::IgnoreCase)
        if ($patternMatches.Count -gt 0) {
            Write-Host "  🔒 $pattern : $($patternMatches.Count) matches" -ForegroundColor Red
            $accessIssues += $patternMatches.Count
        }
    }
    
    # Analyze timestamp precision
    Write-Host "Timestamp Precision Analysis:" -ForegroundColor Cyan
    $nanoTimestamps = [regex]::Matches($content, "\d+\.\d{9}", [System.Text.RegularExpressions.RegexOptions]::IgnoreCase).Count
    $microTimestamps = [regex]::Matches($content, "\d+\.\d{6}", [System.Text.RegularExpressions.RegexOptions]::IgnoreCase).Count
    
    Write-Host "  Nanosecond precision: $nanoTimestamps occurrences" -ForegroundColor $(if ($nanoTimestamps -gt 0) {"Green"} else {"Yellow"})
    Write-Host "  Microsecond precision: $microTimestamps occurrences" -ForegroundColor $(if ($microTimestamps -gt 0) {"Green"} else {"Yellow"})
    
} else {
    Write-Host "ERROR: Log file not found - cannot analyze evidence" -ForegroundColor Red
    $hardwareCount = 0
    $softwareCount = 0
    $accessIssues = 0
}

Write-Host ""
Write-Host "=== FINAL HONEST VERDICT ===" -ForegroundColor Red

# Determine verdict based on evidence
if ($hardwareCount -gt 0 -and $softwareCount -eq 0 -and $accessIssues -eq 0) {
    Write-Host "✅ HARDWARE TIMESTAMPING CONFIRMED" -ForegroundColor Green
    Write-Host "Evidence: $hardwareCount hardware indicators, 0 software fallback indicators" -ForegroundColor Green
    $verdict = "CONFIRMED"
} elseif ($hardwareCount -gt 0 -and $softwareCount -gt 0) {
    Write-Host "⚠️ MIXED MODE DETECTED" -ForegroundColor Yellow
    Write-Host "Evidence: $hardwareCount hardware + $softwareCount software indicators" -ForegroundColor Yellow
    $verdict = "MIXED"
} elseif ($accessIssues -gt 0) {
    Write-Host "🔒 ACCESS DENIED - CANNOT ACCESS HARDWARE TIMESTAMPS" -ForegroundColor Red
    Write-Host "Evidence: $accessIssues permission issues detected" -ForegroundColor Red
    Write-Host "SOLUTION: Run as Administrator or install Intel filter driver" -ForegroundColor Yellow
    $verdict = "ACCESS_DENIED"
} elseif ($softwareCount -gt 0) {
    Write-Host "❌ SOFTWARE FALLBACK ACTIVE" -ForegroundColor Red
    Write-Host "Evidence: $softwareCount software fallback indicators" -ForegroundColor Red
    $verdict = "SOFTWARE_FALLBACK"
} else {
    Write-Host "❓ INCONCLUSIVE - INSUFFICIENT EVIDENCE" -ForegroundColor Gray
    Write-Host "Log analysis did not find definitive timestamp method indicators" -ForegroundColor Gray
    $verdict = "INCONCLUSIVE"
}

Write-Host ""
Write-Host "HONEST ASSESSMENT COMPLETE" -ForegroundColor Yellow
Write-Host "Log file for independent verification: $logFile" -ForegroundColor Gray
Write-Host ""

# Provide actionable recommendations
Write-Host "=== RECOMMENDATIONS ===" -ForegroundColor Cyan
switch ($verdict) {
    "CONFIRMED" {
        Write-Host "✅ Hardware timestamping is working - you can confidently advertise this capability" -ForegroundColor Green
    }
    "ACCESS_DENIED" {
        Write-Host "🔧 Install Intel AVB filter driver and run as Administrator to enable hardware timestamps" -ForegroundColor Yellow
        Write-Host "🔧 Alternative: Use software timestamps for development/testing" -ForegroundColor Yellow
    }
    "SOFTWARE_FALLBACK" {
        Write-Host "⚠️ Currently using software fallback - hardware timestamps not active" -ForegroundColor Red
        Write-Host "🔧 Check Intel driver installation and administrator privileges" -ForegroundColor Yellow
    }
    "MIXED" {
        Write-Host "🔍 Further investigation needed to determine primary timestamp method" -ForegroundColor Yellow
    }
    default {
        Write-Host "🔍 Run with -DetailedAnalysis for more information" -ForegroundColor Yellow
    }
}

return @{
    Verdict = $verdict
    HardwareEvidence = $hardwareCount
    SoftwareEvidence = $softwareCount  
    AccessIssues = $accessIssues
    LogFile = $logFile
}
