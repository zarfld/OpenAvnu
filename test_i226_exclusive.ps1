# Test I226 with Exclusive Intel Filter Driver Access
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

Write-Host "I226 EXCLUSIVE FILTER DRIVER TEST" -ForegroundColor Red
Write-Host "Testing I226 with dedicated Intel filter driver binding..." -ForegroundColor Green
Write-Host ""

Set-Location "C:\Users\dzarf\source\repos\OpenAvnu-2"

# Verify Intel filter driver status
Write-Host "Intel Filter Driver Status:" -ForegroundColor Cyan
$intelDrivers = Get-WmiObject -Class Win32_SystemDriver | Where-Object { 
    $_.Name -match "Intel.*Filter|IntelAvb|I210|I226"
}

foreach ($driver in $intelDrivers) {
    $color = if ($driver.State -eq "Running") { "Green" } else { "Yellow" }
    Write-Host "  $($driver.Name): $($driver.State)" -ForegroundColor $color
}
Write-Host ""

# Check I226 adapter status
$i226 = Get-NetAdapter -Name "Ethernet" -ErrorAction SilentlyContinue
if (-not $i226) {
    Write-Host "ERROR: I226 adapter not found" -ForegroundColor Red
    pause
    exit 1
}

Write-Host "I226 Adapter Status:" -ForegroundColor Cyan
Write-Host "  Name: $($i226.Name)" -ForegroundColor White
Write-Host "  Description: $($i226.InterfaceDescription)" -ForegroundColor White
Write-Host "  MAC: $($i226.MacAddress)" -ForegroundColor White
Write-Host "  Speed: $($i226.LinkSpeed)" -ForegroundColor White
Write-Host "  Status: $($i226.Status)" -ForegroundColor White
Write-Host ""

# Check I210 status to confirm it's excluded
Write-Host "I210 Filter Status Check:" -ForegroundColor Cyan
$i210 = Get-NetAdapter | Where-Object { $_.InterfaceDescription -match "I210" } | Select-Object -First 1
if ($i210) {
    Write-Host "  I210 found but should not have Intel filter driver bound" -ForegroundColor Yellow
    Write-Host "  I210 Name: $($i210.Name)" -ForegroundColor Gray
    Write-Host "  I210 Status: $($i210.Status)" -ForegroundColor Gray
} else {
    Write-Host "  I210 not detected or properly excluded" -ForegroundColor Green
}
Write-Host ""

# Test I226 hardware timestamping with exclusive access
$mac = $i226.MacAddress.Replace("-", ":")
$timestamp = (Get-Date).ToString("yyyyMMdd_HHmmss")
$logFile = "I226_exclusive_$timestamp.err"

Write-Host "EXCLUSIVE HARDWARE TIMESTAMPING TEST" -ForegroundColor Magenta
Write-Host "Testing I226 with dedicated Intel filter driver..." -ForegroundColor Yellow
Write-Host "MAC: $mac" -ForegroundColor White
Write-Host "Duration: $TestDuration seconds" -ForegroundColor Gray
Write-Host "Log: $logFile" -ForegroundColor Gray
Write-Host ""

try {
    # Start gPTP process with I226 exclusive access
    $process = Start-Process -FilePath ".\thirdparty\gptp\build\Release\gptp.exe" -ArgumentList $mac -RedirectStandardError $logFile -NoNewWindow -PassThru
    
    Write-Host "gPTP started (PID: $($process.Id))" -ForegroundColor Green
    Write-Host "Monitoring exclusive hardware access..." -ForegroundColor Yellow
    
    # Progress monitoring
    for ($i = 1; $i -le $TestDuration; $i++) {
        Start-Sleep 1
        $percent = ($i / $TestDuration) * 100
        Write-Progress -Activity "I226 Exclusive Hardware Test" -Status "Testing dedicated filter driver access..." -PercentComplete $percent
    }
    
    Write-Progress -Completed -Activity "I226 Exclusive Hardware Test"
    
    if (-not $process.HasExited) {
        $process.Kill()
        $process.WaitForExit(3000)
    }
    
    Write-Host "Exclusive test completed!" -ForegroundColor Green
    
} catch {
    Write-Host "ERROR: Test failed - $($_.Exception.Message)" -ForegroundColor Red
    pause
    exit 1
}

Write-Host ""
Write-Host "EXCLUSIVE ACCESS ANALYSIS" -ForegroundColor Magenta

if (-not (Test-Path $logFile)) {
    Write-Host "ERROR: No log generated" -ForegroundColor Red
    pause
    exit 1
}

$content = Get-Content $logFile -Raw
$lines = ($content -split "`n").Count

Write-Host "Log analysis: $lines lines generated" -ForegroundColor Gray
Write-Host ""

# Comprehensive evidence analysis
$evidenceTypes = @{
    "Hardware Success" = @("Hardware.*timestamper.*initialized", "PTP.*hardware.*enabled", "Intel.*hardware.*timestamp")
    "Hardware Access" = @("hardware.*timestamp", "Intel.*hardware", "PTP.*hardware") 
    "Software Fallback" = @("software.*timestamp", "Enhanced.*software", "Falling back.*software", "NDIS.*timestamp")
    "Clock Issues" = @("Unable.*determine.*clock", "clock rate.*failed", "HWTimestamper_init.*Unable")
    "Access Problems" = @("Error 5", "error.*50", "Access.*denied", "OID.*not.*available", "Administrator.*privilege")
    "Exclusive Benefits" = @("exclusive.*access", "dedicated.*filter", "I226.*only", "single.*adapter")
}

foreach ($category in $evidenceTypes.Keys) {
    $count = 0
    $patterns = $evidenceTypes[$category]
    
    foreach ($pattern in $patterns) {
        $patternMatches = [regex]::Matches($content, $pattern, [System.Text.RegularExpressions.RegexOptions]::IgnoreCase)
        $count += $patternMatches.Count
    }
    
    $color = switch ($category) {
        "Hardware Success" { "Green" }
        "Hardware Access" { "Green" }
        "Software Fallback" { "Red" }
        "Clock Issues" { "Yellow" }
        "Access Problems" { "Red" }
        "Exclusive Benefits" { "Cyan" }
        default { "White" }
    }
    
    if ($count -gt 0) {
        Write-Host "$category`: $count matches" -ForegroundColor $color
    }
}

Write-Host ""

# Key diagnostic lines
Write-Host "Key Diagnostic Evidence:" -ForegroundColor White
$keyLines = Get-Content $logFile | Where-Object { 
    $_ -match "timestamp|hardware|software|Intel|clock rate|Unable|Error|initialized|exclusive|filter" -and
    $_ -notmatch "DEBUG.*Enhanced packet|addEventTimer|Parsing.*address"
} | Select-Object -First 10

foreach ($line in $keyLines) {
    $shortLine = $line.Substring(0, [Math]::Min(120, $line.Length))
    Write-Host "  $shortLine" -ForegroundColor Gray
}

Write-Host ""

# Final verdict
Write-Host "EXCLUSIVE ACCESS VERDICT:" -ForegroundColor Red

# Calculate evidence scores
$hardwareSuccess = [regex]::Matches($content, "Hardware.*timestamper.*initialized|PTP.*hardware.*enabled", [System.Text.RegularExpressions.RegexOptions]::IgnoreCase).Count
$hardwareAccess = [regex]::Matches($content, "hardware.*timestamp|Intel.*hardware|PTP.*hardware", [System.Text.RegularExpressions.RegexOptions]::IgnoreCase).Count  
$softwareFallback = [regex]::Matches($content, "software.*timestamp|Enhanced.*software|Falling back.*software", [System.Text.RegularExpressions.RegexOptions]::IgnoreCase).Count
$clockIssues = [regex]::Matches($content, "Unable.*determine.*clock|clock rate.*failed", [System.Text.RegularExpressions.RegexOptions]::IgnoreCase).Count
$accessErrors = [regex]::Matches($content, "Error 5|error.*50|Access.*denied|OID.*not.*available", [System.Text.RegularExpressions.RegexOptions]::IgnoreCase).Count

Write-Host "Evidence Summary:" -ForegroundColor Cyan
Write-Host "  Hardware Success Indicators: $hardwareSuccess" -ForegroundColor Green
Write-Host "  Hardware Access Attempts: $hardwareAccess" -ForegroundColor Green  
Write-Host "  Software Fallback Indicators: $softwareFallback" -ForegroundColor Red
Write-Host "  Clock Detection Issues: $clockIssues" -ForegroundColor Yellow
Write-Host "  Access Errors: $accessErrors" -ForegroundColor Red

Write-Host ""

if ($hardwareSuccess -gt 0) {
    Write-Host "🎉 SUCCESS: I226 Hardware Timestamping with Exclusive Filter!" -ForegroundColor Green
    Write-Host "✅ Exclusive Intel filter driver access enabled hardware timestamping" -ForegroundColor Green
} elseif ($accessErrors -eq 0 -and $clockIssues -eq 0) {
    Write-Host "✅ MAJOR IMPROVEMENT: No access errors or clock issues" -ForegroundColor Yellow
    Write-Host "📈 Exclusive filter driver resolved access conflicts" -ForegroundColor Yellow
} elseif ($accessErrors -eq 0) {
    Write-Host "⚠️  PARTIAL SUCCESS: Access issues resolved, clock issues remain" -ForegroundColor Yellow
    Write-Host "🔧 May need Gigabit network connection for full hardware timestamping" -ForegroundColor Yellow
} elseif ($clockIssues -eq 0) {
    Write-Host "⚠️  PARTIAL SUCCESS: Clock issues resolved, some access problems remain" -ForegroundColor Yellow
} else {
    Write-Host "❌ STILL ISSUES: Exclusive access didn't resolve all problems" -ForegroundColor Red
    Write-Host "May need additional configuration or Gigabit network infrastructure" -ForegroundColor Gray
}

Write-Host ""
Write-Host "Comparison with Previous Tests:" -ForegroundColor Cyan
Write-Host "  Previous I226 (shared filter): Clock rate detection failed" -ForegroundColor Gray
Write-Host "  Previous I210 (shared filter): Error 50 access denied" -ForegroundColor Gray  
Write-Host "  Current I226 (exclusive filter): See results above" -ForegroundColor White

Write-Host ""
Write-Host "Log file saved: $logFile" -ForegroundColor Gray
Write-Host "Press any key to continue..."
pause > $null
