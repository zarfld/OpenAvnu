# Compare I210 vs I226 Hardware Timestamping (Administrator)
param(
    [int]$TestDuration = 20
)

# Check if running as Administrator
$isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")

if (-not $isAdmin) {
    Write-Host "Requesting Administrator privileges..." -ForegroundColor Yellow
    $scriptPath = $MyInvocation.MyCommand.Path
    $arguments = "-TestDuration $TestDuration"
    Start-Process PowerShell -ArgumentList "-ExecutionPolicy Bypass", "-File", "`"$scriptPath`"", $arguments -Verb RunAs
    exit
}

Write-Host "I210 vs I226 HARDWARE TIMESTAMPING COMPARISON" -ForegroundColor Red
Write-Host "Running with Administrator privileges..." -ForegroundColor Green
Write-Host ""

Set-Location "C:\Users\dzarf\source\repos\OpenAvnu-2"

# Find both adapters
$adapters = Get-NetAdapter | Where-Object { 
    $_.InterfaceDescription -match "I21[0-9]|I22[5-6]" -and $_.Status -eq 'Up' 
}

$i210 = $adapters | Where-Object { $_.InterfaceDescription -match "I210" } | Select-Object -First 1
$i226 = $adapters | Where-Object { $_.InterfaceDescription -match "I226" } | Select-Object -First 1

if (-not $i210 -and -not $i226) {
    Write-Host "ERROR: No Intel adapters found" -ForegroundColor Red
    pause
    exit 1
}

Write-Host "Found adapters:" -ForegroundColor Cyan
if ($i210) {
    Write-Host "  I210: $($i210.Name) - $($i210.MacAddress)" -ForegroundColor Green
}
if ($i226) {
    Write-Host "  I226: $($i226.Name) - $($i226.MacAddress)" -ForegroundColor Green
}
Write-Host ""

# Test function
function Test-AdapterTimestamping {
    param($adapter, $adapterType)
    
    if (-not $adapter) {
        Write-Host "$adapterType`: Not available" -ForegroundColor Gray
        return @{ Available = $false }
    }
    
    $mac = $adapter.MacAddress.Replace("-", ":")
    $timestamp = (Get-Date).ToString("yyyyMMdd_HHmmss")
    $logFile = "$adapterType`_admin_$timestamp.err"
    
    Write-Host "$adapterType Test:" -ForegroundColor Yellow
    Write-Host "  Adapter: $($adapter.Name)" -ForegroundColor White
    Write-Host "  MAC: $mac" -ForegroundColor White
    Write-Host "  Log: $logFile" -ForegroundColor Gray
    
    try {
        $process = Start-Process -FilePath ".\thirdparty\gptp\build\Release\gptp.exe" -ArgumentList $mac -RedirectStandardError $logFile -NoNewWindow -PassThru
        
        Write-Host "  Process started (PID: $($process.Id))" -ForegroundColor Green
        
        # Wait for test
        for ($i = 1; $i -le $TestDuration; $i++) {
            Start-Sleep 1
            $percent = ($i / $TestDuration) * 100
            Write-Progress -Activity "$adapterType Hardware Test" -Status "Collecting evidence..." -PercentComplete $percent
        }
        
        Write-Progress -Completed -Activity "$adapterType Hardware Test"
        
        if (-not $process.HasExited) {
            $process.Kill()
            $process.WaitForExit(3000)
        }
        
        Write-Host "  Test complete!" -ForegroundColor Green
        
    } catch {
        Write-Host "  ERROR: $($_.Exception.Message)" -ForegroundColor Red
        return @{ Available = $true; Success = $false; Error = $_.Exception.Message }
    }
    
    # Analyze results
    if (-not (Test-Path $logFile)) {
        Write-Host "  ERROR: No log generated" -ForegroundColor Red
        return @{ Available = $true; Success = $false; Error = "No log file" }
    }
    
    $content = Get-Content $logFile -Raw
    
    # Count evidence
    $hardwareCount = 0
    $softwareCount = 0
    $accessCount = 0
    $errorCount = 0
    
    # Hardware evidence
    $hardwarePatterns = @("hardware.*timestamp", "Intel.*hardware", "PTP.*hardware")
    foreach ($pattern in $hardwarePatterns) {
        $found = [regex]::Matches($content, $pattern, [System.Text.RegularExpressions.RegexOptions]::IgnoreCase)
        $hardwareCount += $found.Count
    }
    
    # Software fallback evidence  
    $softwarePatterns = @("software.*timestamp", "Enhanced.*software", "Falling back.*software", "NDIS.*timestamp")
    foreach ($pattern in $softwarePatterns) {
        $found = [regex]::Matches($content, $pattern, [System.Text.RegularExpressions.RegexOptions]::IgnoreCase)
        $softwareCount += $found.Count
    }
    
    # Access issues
    $accessPatterns = @("Administrator.*privilege", "OID.*not.*available", "Access.*denied")
    foreach ($pattern in $accessPatterns) {
        $found = [regex]::Matches($content, $pattern, [System.Text.RegularExpressions.RegexOptions]::IgnoreCase)
        $accessCount += $found.Count
    }
    
    # Error patterns
    $errorPatterns = @("Error 5", "error.*50", "Failed.*query.*timestamp")
    foreach ($pattern in $errorPatterns) {
        $found = [regex]::Matches($content, $pattern, [System.Text.RegularExpressions.RegexOptions]::IgnoreCase)
        $errorCount += $found.Count
    }
    
    # Key log lines
    $keyLines = Get-Content $logFile | Where-Object { 
        $_ -match "timestamp|hardware|software|Intel|OID|Error|Failed.*query" -and
        $_ -notmatch "DEBUG.*Enhanced packet|Parsing.*address"
    } | Select-Object -First 5
    
    Write-Host "  Hardware evidence: $hardwareCount" -ForegroundColor Green
    Write-Host "  Software evidence: $softwareCount" -ForegroundColor Red
    Write-Host "  Access issues: $accessCount" -ForegroundColor Yellow
    Write-Host "  Errors: $errorCount" -ForegroundColor Red
    
    if ($keyLines) {
        Write-Host "  Key evidence:" -ForegroundColor Gray
        foreach ($line in $keyLines) {
            $shortLine = $line.Substring(0, [Math]::Min(80, $line.Length))
            Write-Host "    $shortLine" -ForegroundColor Gray
        }
    }
    
    Write-Host ""
    
    return @{
        Available = $true
        Success = ($hardwareCount -gt 0 -and $softwareCount -eq 0 -and $errorCount -eq 0)
        HardwareEvidence = $hardwareCount
        SoftwareEvidence = $softwareCount
        AccessIssues = $accessCount
        Errors = $errorCount
        LogFile = $logFile
        KeyLines = $keyLines
    }
}

# Test both adapters
Write-Host "COMPARATIVE TESTING" -ForegroundColor Magenta
Write-Host ""

$i210Results = Test-AdapterTimestamping $i210 "I210"
$i226Results = Test-AdapterTimestamping $i226 "I226"

# Comparison results
Write-Host "COMPARISON RESULTS" -ForegroundColor Magenta
Write-Host ""

Write-Host "I210 Results:" -ForegroundColor Cyan
if ($i210Results.Available) {
    if ($i210Results.Success) {
        Write-Host "  ✅ HARDWARE TIMESTAMPING WORKING" -ForegroundColor Green
    } else {
        Write-Host "  ❌ SOFTWARE FALLBACK (Errors: $($i210Results.Errors))" -ForegroundColor Red
    }
} else {
    Write-Host "  ❓ NOT AVAILABLE" -ForegroundColor Gray
}

Write-Host "I226 Results:" -ForegroundColor Cyan  
if ($i226Results.Available) {
    if ($i226Results.Success) {
        Write-Host "  ✅ HARDWARE TIMESTAMPING WORKING" -ForegroundColor Green
    } else {
        Write-Host "  ❌ SOFTWARE FALLBACK (Errors: $($i226Results.Errors))" -ForegroundColor Red
    }
} else {
    Write-Host "  ❓ NOT AVAILABLE" -ForegroundColor Gray
}

Write-Host ""
Write-Host "FINAL VERDICT:" -ForegroundColor Red

if ($i210Results.Success -or $i226Results.Success) {
    Write-Host "🎉 AT LEAST ONE ADAPTER HAS HARDWARE TIMESTAMPING!" -ForegroundColor Green
    if ($i210Results.Success -and $i226Results.Success) {
        Write-Host "Both I210 and I226 working with hardware timestamps" -ForegroundColor Green
    } elseif ($i210Results.Success) {
        Write-Host "I210 has hardware timestamping, I226 falls back to software" -ForegroundColor Yellow
    } else {
        Write-Host "I226 has hardware timestamping, I210 falls back to software" -ForegroundColor Yellow
    }
} else {
    Write-Host "❌ BOTH ADAPTERS USING SOFTWARE FALLBACK" -ForegroundColor Red
    Write-Host "Hardware timestamping not accessible on either adapter" -ForegroundColor Red
}

Write-Host ""
Write-Host "Log files generated:" -ForegroundColor Gray
if ($i210Results.LogFile) { Write-Host "  I210: $($i210Results.LogFile)" -ForegroundColor Gray }
if ($i226Results.LogFile) { Write-Host "  I226: $($i226Results.LogFile)" -ForegroundColor Gray }

Write-Host ""
Write-Host "Press any key to continue..."
pause > $null
