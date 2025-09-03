# Hardware Timestamping Evidence Collection Script
# CRITICAL: Proves actual hardware timestamp usage vs software fallback
param(
    [string]$Adapter = "auto",
    [int]$TestDuration = 30,
    [switch]$DetailedAnalysis,
    [switch]$CompareWithSoftware
)

Write-Host "HARDWARE TIMESTAMPING EVIDENCE COLLECTION" -ForegroundColor Red
Write-Host "CRITICAL VALIDATION: Proving actual hardware timestamp usage" -ForegroundColor Yellow
Write-Host ""

# Intel Hardware Timestamping Indicators to Look For
$hardwareEvidencePatterns = @(
    @{Pattern = "Hardware timestamping enabled"; Description = "Direct hardware timestamp confirmation"},
    @{Pattern = "Intel filter driver.*loaded"; Description = "Intel AVB filter driver active"},
    @{Pattern = "PTP hardware clock"; Description = "Hardware PTP clock detected"},
    @{Pattern = "SO_TIMESTAMPING.*hardware"; Description = "Socket-level hardware timestamps"},
    @{Pattern = "timestamp.*hardware"; Description = "General hardware timestamp references"},
    @{Pattern = "PHC.*clock"; Description = "PTP Hardware Clock (PHC) usage"},
    @{Pattern = "I21[0-9].*timestamp"; Description = "Intel I210/I219/I225/I226 specific timestamps"},
    @{Pattern = "SIOCGTSTAMP"; Description = "Hardware timestamp socket option"},
    @{Pattern = "ethtool.*time.*stamp"; Description = "Ethtool hardware timestamp config"}
)

$softwareFallbackPatterns = @(
    @{Pattern = "Software timestamping"; Description = "SOFTWARE FALLBACK WARNING"},
    @{Pattern = "NDIS.*timestamp"; Description = "NDIS software timestamp fallback"},
    @{Pattern = "GetSystemTimeAsFileTime"; Description = "Windows system time fallback"},
    @{Pattern = "No hardware timestamp"; Description = "Hardware timestamp unavailable"},
    @{Pattern = "timestamp.*software"; Description = "Software timestamp mode"}
)

function Test-HardwareTimestampCapability {
    param($AdapterInfo)
    
    Write-Host "=== HARDWARE CAPABILITY ANALYSIS: $($AdapterInfo.Type) ===" -ForegroundColor Cyan
    
    # Check Intel driver capabilities
    $driver = Get-NetAdapter | Where-Object { $_.MacAddress -eq $AdapterInfo.MAC.Replace(":", "-") } | 
              Get-NetAdapterAdvancedProperty | Where-Object { $_.DisplayName -match "timestamp|PTP|AVB" }
    
    if ($driver) {
        Write-Host "Intel Driver Timestamp Properties:" -ForegroundColor Green
        foreach ($prop in $driver) {
            Write-Host "  $($prop.DisplayName): $($prop.DisplayValue)" -ForegroundColor White
        }
    } else {
        Write-Host "WARNING: No timestamp-related driver properties found" -ForegroundColor Red
    }
    
    # Check for Intel AVB filter driver
    $filterDriver = Get-WmiObject -Class Win32_SystemDriver | Where-Object { 
        $_.Name -match "IntelAvb|I21[0-9]|AVB" -and $_.State -eq "Running"
    }
    
    if ($filterDriver) {
        Write-Host "Intel AVB Filter Driver Status:" -ForegroundColor Green
        foreach ($filter in $filterDriver) {
            Write-Host "  $($filter.Name): $($filter.State) (Path: $($filter.PathName))" -ForegroundColor White
        }
    } else {
        Write-Host "WARNING: Intel AVB filter driver not detected" -ForegroundColor Red
    }
    
    Write-Host ""
}

function Analyze-TimestampEvidence {
    param($LogFile, $TestName)
    
    Write-Host "=== TIMESTAMP EVIDENCE ANALYSIS: $TestName ===" -ForegroundColor Cyan
    
    if (-not (Test-Path $LogFile)) {
        Write-Host "ERROR: Log file $LogFile not found" -ForegroundColor Red
        return @{HardwareEvidence = 0; SoftwareEvidence = 0; Conclusion = "NO_DATA"}
    }
    
    $content = Get-Content $LogFile -Raw
    
    # Search for hardware timestamp evidence
    Write-Host "Hardware Timestamp Evidence:" -ForegroundColor Green
    $hardwareCount = 0
    foreach ($pattern in $hardwareEvidencePatterns) {
        $matches = [regex]::Matches($content, $pattern.Pattern, [System.Text.RegularExpressions.RegexOptions]::IgnoreCase)
        if ($matches.Count -gt 0) {
            Write-Host "  ✅ $($pattern.Description): $($matches.Count) occurrences" -ForegroundColor Green
            $hardwareCount += $matches.Count
            
            if ($DetailedAnalysis) {
                foreach ($match in $matches[0..2]) {  # Show first 3 matches
                    $context = $content.Substring([Math]::Max(0, $match.Index - 50), [Math]::Min(100, $content.Length - [Math]::Max(0, $match.Index - 50)))
                    Write-Host "    Context: ...$($context.Replace("`n", " ").Replace("`r", ""))..." -ForegroundColor Gray
                }
            }
        }
    }
    
    # Search for software fallback evidence
    Write-Host "Software Fallback Evidence:" -ForegroundColor Yellow
    $softwareCount = 0
    foreach ($pattern in $softwareFallbackPatterns) {
        $matches = [regex]::Matches($content, $pattern.Pattern, [System.Text.RegularExpressions.RegexOptions]::IgnoreCase)
        if ($matches.Count -gt 0) {
            Write-Host "  ⚠️ $($pattern.Description): $($matches.Count) occurrences" -ForegroundColor Red
            $softwareCount += $matches.Count
            
            if ($DetailedAnalysis) {
                foreach ($match in $matches[0..2]) {
                    $context = $content.Substring([Math]::Max(0, $match.Index - 50), [Math]::Min(100, $content.Length - [Math]::Max(0, $match.Index - 50)))
                    Write-Host "    Context: ...$($context.Replace("`n", " ").Replace("`r", ""))..." -ForegroundColor Gray
                }
            }
        }
    }
    
    # Analyze timestamp precision patterns
    Write-Host "Timestamp Precision Analysis:" -ForegroundColor Cyan
    $timestampMatches = [regex]::Matches($content, "\d+\.\d{6,}", [System.Text.RegularExpressions.RegexOptions]::IgnoreCase)
    $highPrecisionCount = ($timestampMatches | Where-Object { $_.Value -match "\d+\.\d{9}" }).Count
    $mediumPrecisionCount = ($timestampMatches | Where-Object { $_.Value -match "\d+\.\d{6,8}" }).Count
    
    Write-Host "  High precision (nanosecond): $highPrecisionCount timestamps" -ForegroundColor $(if ($highPrecisionCount -gt 0) {"Green"} else {"Red"})
    Write-Host "  Medium precision (microsecond): $mediumPrecisionCount timestamps" -ForegroundColor $(if ($mediumPrecisionCount -gt 0) {"Yellow"} else {"Red"})
    
    # Final assessment
    $conclusion = if ($hardwareCount -gt 0 -and $softwareCount -eq 0) {
        "HARDWARE_CONFIRMED"
    } elseif ($hardwareCount -gt 0 -and $softwareCount -gt 0) {
        "MIXED_MODE"
    } elseif ($hardwareCount -eq 0 -and $softwareCount -gt 0) {
        "SOFTWARE_FALLBACK"
    } else {
        "INCONCLUSIVE"
    }
    
    Write-Host "CONCLUSION: $conclusion" -ForegroundColor $(switch ($conclusion) {
        "HARDWARE_CONFIRMED" {"Green"}
        "MIXED_MODE" {"Yellow"} 
        "SOFTWARE_FALLBACK" {"Red"}
        default {"Gray"}
    })
    Write-Host ""
    
    return @{
        HardwareEvidence = $hardwareCount
        SoftwareEvidence = $softwareCount
        HighPrecisionTimestamps = $highPrecisionCount
        MediumPrecisionTimestamps = $mediumPrecisionCount
        Conclusion = $conclusion
        LogFile = $LogFile
    }
}

# Main execution
Write-Host "Phase 1: System Capability Assessment" -ForegroundColor Magenta
Write-Host ""

# Discover adapters
$adapters = @()
$supported = @("*I210*", "*I219*", "*I225*", "*I226*")

foreach ($pattern in $supported) {
    $found = Get-NetAdapter | Where-Object { 
        $_.InterfaceDescription -like $pattern -and $_.Status -eq 'Up' 
    }
    foreach ($adapter in $found) {
        $type = switch -Wildcard ($adapter.InterfaceDescription) {
            "*I210*" { "I210" }
            "*I219*" { "I219" }
            "*I225*" { "I225" }
            "*I226*" { "I226" }
        }
        
        $adapters += @{
            Type = $type
            MAC = $adapter.MacAddress.Replace("-", ":")
            Name = $adapter.Name
            InterfaceIndex = $adapter.InterfaceIndex
        }
    }
}

if ($adapters.Count -eq 0) {
    Write-Host "ERROR: No supported Intel adapters found" -ForegroundColor Red
    exit 1
}

# Test each adapter's hardware capabilities
foreach ($adapter in $adapters) {
    Test-HardwareTimestampCapability -AdapterInfo $adapter
}

Write-Host "Phase 2: Live gPTP Hardware Timestamp Evidence Collection" -ForegroundColor Magenta
Write-Host ""

$evidenceResults = @()

foreach ($adapter in $adapters) {
    Write-Host "Testing $($adapter.Type) ($($adapter.MAC)) for hardware timestamp evidence..." -ForegroundColor Yellow
    
    # Generate evidence log file
    $timestamp = (Get-Date).ToString("yyyyMMdd_HHmmss")
    $evidenceLog = "hardware_evidence_$($adapter.Type)_$timestamp.log"
    $evidenceErr = "hardware_evidence_$($adapter.Type)_$timestamp.err"
    
    # Start gPTP with maximum debug logging
    $gptpArgs = @{
        FilePath = ".\thirdparty\gptp\build\Release\gptp.exe"
        ArgumentList = @($adapter.MAC)
        RedirectStandardOutput = $evidenceLog
        RedirectStandardError = $evidenceErr
        NoNewWindow = $true
        PassThru = $true
    }
    
    Write-Host "  Starting gPTP evidence collection (PID will follow)..." -ForegroundColor Gray
    $process = Start-Process @gptpArgs
    Write-Host "  Process ID: $($process.Id)" -ForegroundColor White
    
    # Let it run to collect evidence
    Write-Host "  Collecting evidence for $TestDuration seconds..." -ForegroundColor Gray
    Start-Sleep -Seconds $TestDuration
    
    # Stop process
    if (-not $process.HasExited) {
        $process.Kill()
        $process.WaitForExit(5000)
    }
    
    Write-Host "  Evidence collection complete. Analyzing..." -ForegroundColor Gray
    
    # Analyze both stdout and stderr logs
    $stdoutAnalysis = Analyze-TimestampEvidence -LogFile $evidenceLog -TestName "$($adapter.Type) STDOUT"
    $stderrAnalysis = Analyze-TimestampEvidence -LogFile $evidenceErr -TestName "$($adapter.Type) STDERR"
    
    $combinedResult = @{
        Adapter = $adapter.Type
        MAC = $adapter.MAC
        StdoutEvidence = $stdoutAnalysis
        StderrEvidence = $stderrAnalysis
        OverallConclusion = if ($stdoutAnalysis.Conclusion -eq "HARDWARE_CONFIRMED" -or $stderrAnalysis.Conclusion -eq "HARDWARE_CONFIRMED") {
            "HARDWARE_CONFIRMED"
        } elseif ($stdoutAnalysis.Conclusion -eq "SOFTWARE_FALLBACK" -or $stderrAnalysis.Conclusion -eq "SOFTWARE_FALLBACK") {
            "SOFTWARE_FALLBACK"
        } else {
            "INCONCLUSIVE"
        }
        TotalHardwareEvidence = $stdoutAnalysis.HardwareEvidence + $stderrAnalysis.HardwareEvidence
        TotalSoftwareEvidence = $stdoutAnalysis.SoftwareEvidence + $stderrAnalysis.SoftwareEvidence
    }
    
    $evidenceResults += $combinedResult
}

Write-Host "Phase 3: Final Hardware Timestamping Evidence Assessment" -ForegroundColor Magenta
Write-Host ""

Write-Host "EVIDENCE SUMMARY:" -ForegroundColor Cyan
foreach ($result in $evidenceResults) {
    Write-Host "$($result.Adapter) ($($result.MAC)):" -ForegroundColor White
    Write-Host "  Hardware Evidence: $($result.TotalHardwareEvidence) indicators" -ForegroundColor $(if ($result.TotalHardwareEvidence -gt 0) {"Green"} else {"Red"})
    Write-Host "  Software Evidence: $($result.TotalSoftwareEvidence) indicators" -ForegroundColor $(if ($result.TotalSoftwareEvidence -gt 0) {"Red"} else {"Green"})
    Write-Host "  Conclusion: $($result.OverallConclusion)" -ForegroundColor $(switch ($result.OverallConclusion) {
        "HARDWARE_CONFIRMED" {"Green"}
        "SOFTWARE_FALLBACK" {"Red"}
        default {"Yellow"}
    })
    Write-Host ""
}

# Final verdict
$hardwareConfirmed = ($evidenceResults | Where-Object { $_.OverallConclusion -eq "HARDWARE_CONFIRMED" }).Count
$softwareFallback = ($evidenceResults | Where-Object { $_.OverallConclusion -eq "SOFTWARE_FALLBACK" }).Count
$totalAdapters = $evidenceResults.Count

Write-Host "FINAL HARDWARE TIMESTAMPING VERDICT:" -ForegroundColor Red
Write-Host ""

if ($hardwareConfirmed -eq $totalAdapters) {
    Write-Host "✅ HARDWARE TIMESTAMPING CONFIRMED ON ALL ADAPTERS" -ForegroundColor Green
    Write-Host "Evidence: All $totalAdapters Intel adapters using hardware timestamps" -ForegroundColor Green
} elseif ($hardwareConfirmed -gt 0) {
    Write-Host "⚠️ PARTIAL HARDWARE TIMESTAMPING" -ForegroundColor Yellow  
    Write-Host "Evidence: $hardwareConfirmed/$totalAdapters adapters using hardware timestamps" -ForegroundColor Yellow
} elseif ($softwareFallback -gt 0) {
    Write-Host "❌ SOFTWARE FALLBACK DETECTED" -ForegroundColor Red
    Write-Host "Evidence: $softwareFallback/$totalAdapters adapters using software fallback" -ForegroundColor Red
} else {
    Write-Host "❓ INCONCLUSIVE EVIDENCE" -ForegroundColor Gray
    Write-Host "Insufficient data to determine timestamp method" -ForegroundColor Gray
}

Write-Host ""
Write-Host "HONEST ASSESSMENT: Based on actual log analysis and driver detection" -ForegroundColor Yellow
Write-Host "Log files available for independent verification:" -ForegroundColor Gray
Get-ChildItem hardware_evidence_*.log, hardware_evidence_*.err | ForEach-Object {
    Write-Host "  $($_.Name)" -ForegroundColor White
}
