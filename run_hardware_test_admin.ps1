# Run Hardware Timestamping Test as Administrator
param(
    [int]$TestDuration = 15
)

# Check if running as Administrator
$isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")

if (-not $isAdmin) {
    Write-Host "Requesting Administrator privileges..." -ForegroundColor Yellow
    Write-Host "This will restart the script with elevated permissions." -ForegroundColor Gray
    
    # Restart script as Administrator
    $scriptPath = $MyInvocation.MyCommand.Path
    $arguments = "-TestDuration $TestDuration"
    Start-Process PowerShell -ArgumentList "-ExecutionPolicy Bypass", "-File", "`"$scriptPath`"", $arguments -Verb RunAs
    exit
}

Write-Host "ADMINISTRATOR HARDWARE TIMESTAMPING TEST" -ForegroundColor Red
Write-Host "Running with elevated privileges..." -ForegroundColor Green
Write-Host ""

# Change to correct directory
Set-Location "C:\Users\dzarf\source\repos\OpenAvnu-2"

# Step 1: Verify Intel drivers with admin access
Write-Host "Step 1: Intel Driver Status (Admin Mode)" -ForegroundColor Cyan

$intelDrivers = Get-WmiObject -Class Win32_SystemDriver | Where-Object { 
    $_.Name -match "Intel" -or $_.Name -match "I21[0-9]" -or $_.Name -match "AVB"
}

foreach ($driver in $intelDrivers) {
    $color = if ($driver.State -eq "Running") { "Green" } else { "Yellow" }
    Write-Host "  $($driver.Name): $($driver.State)" -ForegroundColor $color
}
Write-Host ""

# Step 2: Find best Intel adapter
Write-Host "Step 2: Intel Adapter Selection" -ForegroundColor Cyan

$adapters = Get-NetAdapter | Where-Object { 
    $_.InterfaceDescription -match "I21[0-9]|I22[5-6]" -and $_.Status -eq 'Up' 
}

$testAdapter = $adapters | Where-Object { $_.InterfaceDescription -match "I210" } | Select-Object -First 1
if (-not $testAdapter) {
    $testAdapter = $adapters | Select-Object -First 1
}

if (-not $testAdapter) {
    Write-Host "ERROR: No Intel adapters found" -ForegroundColor Red
    pause
    exit 1
}

$mac = $testAdapter.MacAddress.Replace("-", ":")
Write-Host "Selected: $($testAdapter.Name) - $($testAdapter.InterfaceDescription)" -ForegroundColor Green
Write-Host "MAC: $mac" -ForegroundColor White
Write-Host ""

# Step 3: Run gPTP with admin privileges
Write-Host "Step 3: gPTP Hardware Test (Administrator Mode)" -ForegroundColor Cyan

$timestamp = (Get-Date).ToString("yyyyMMdd_HHmmss")
$logFile = "admin_test_$timestamp.err"

Write-Host "Starting gPTP with hardware access..." -ForegroundColor Yellow
Write-Host "Duration: $TestDuration seconds" -ForegroundColor Gray
Write-Host "Log: $logFile" -ForegroundColor Gray

try {
    # Start gPTP process
    $process = Start-Process -FilePath ".\thirdparty\gptp\build\Release\gptp.exe" -ArgumentList $mac -RedirectStandardError $logFile -NoNewWindow -PassThru
    
    Write-Host "Process started (PID: $($process.Id))" -ForegroundColor Green
    Write-Host "Collecting evidence..." -ForegroundColor Gray
    
    # Wait for test duration
    for ($i = 1; $i -le $TestDuration; $i++) {
        Start-Sleep 1
        Write-Progress -Activity "Hardware Timestamp Test" -Status "Collecting evidence..." -PercentComplete (($i / $TestDuration) * 100)
    }
    
    Write-Progress -Completed -Activity "Hardware Timestamp Test"
    
    # Stop process
    if (-not $process.HasExited) {
        $process.Kill()
        $process.WaitForExit(3000)
        Write-Host "Process terminated" -ForegroundColor Gray
    }
} catch {
    Write-Host "ERROR: Failed to start gPTP - $($_.Exception.Message)" -ForegroundColor Red
    pause
    exit 1
}

Write-Host ""

# Step 4: Advanced evidence analysis
Write-Host "Step 4: Administrator Evidence Analysis" -ForegroundColor Cyan

if (-not (Test-Path $logFile)) {
    Write-Host "ERROR: No log file generated!" -ForegroundColor Red
    pause
    exit 1
}

$content = Get-Content $logFile -Raw
$lines = ($content -split "`n").Count

Write-Host "Log file size: $lines lines" -ForegroundColor Gray
Write-Host ""

# Critical evidence patterns
$hardwareEvidence = @()
$softwareEvidence = @()
$accessEvidence = @()

# Hardware timestamping indicators
$hardwarePatterns = @{
    "Hardware Timestamping" = "hardware.*timestamp"
    "Intel Hardware Access" = "Intel.*hardware|hardware.*Intel"
    "PTP Hardware" = "PTP.*hardware"
    "SO_TIMESTAMPING" = "SO_TIMESTAMPING"
    "Hardware PHC" = "PHC.*clock|hardware.*clock"
}

# Software fallback indicators  
$softwarePatterns = @{
    "Software Timestamping" = "software.*timestamp"
    "NDIS Fallback" = "NDIS.*timestamp|fallback.*NDIS"
    "Enhanced Software" = "Enhanced.*software"
    "Falling Back" = "Falling back.*software"
}

# Access and privilege indicators
$accessPatterns = @{
    "Access Denied" = "Access.*denied|ERROR_ACCESS_DENIED"
    "Privilege Required" = "Administrator.*privilege|privilege.*required"
    "OID Access" = "OID.*not.*available|custom.*OID"
    "Error 5" = "Error 5|error.*5"
}

Write-Host "=== HARDWARE EVIDENCE ===" -ForegroundColor Green
foreach ($key in $hardwarePatterns.Keys) {
    $patternMatches = [regex]::Matches($content, $hardwarePatterns[$key], [System.Text.RegularExpressions.RegexOptions]::IgnoreCase)
    if ($patternMatches.Count -gt 0) {
        Write-Host "✅ $key`: $($patternMatches.Count) matches" -ForegroundColor Green
        $hardwareEvidence += $patternMatches.Count
    }
}

Write-Host ""
Write-Host "=== SOFTWARE FALLBACK EVIDENCE ===" -ForegroundColor Red
foreach ($key in $softwarePatterns.Keys) {
    $patternMatches = [regex]::Matches($content, $softwarePatterns[$key], [System.Text.RegularExpressions.RegexOptions]::IgnoreCase)
    if ($patternMatches.Count -gt 0) {
        Write-Host "❌ $key`: $($patternMatches.Count) matches" -ForegroundColor Red
        $softwareEvidence += $patternMatches.Count
    }
}

Write-Host ""
Write-Host "=== ACCESS ISSUES ===" -ForegroundColor Yellow
foreach ($key in $accessPatterns.Keys) {
    $patternMatches = [regex]::Matches($content, $accessPatterns[$key], [System.Text.RegularExpressions.RegexOptions]::IgnoreCase)
    if ($patternMatches.Count -gt 0) {
        Write-Host "⚠️  $key`: $($patternMatches.Count) matches" -ForegroundColor Yellow
        $accessEvidence += $patternMatches.Count
    }
}

# Calculate scores
$hardwareScore = ($hardwareEvidence | Measure-Object -Sum).Sum
$softwareScore = ($softwareEvidence | Measure-Object -Sum).Sum  
$accessScore = ($accessEvidence | Measure-Object -Sum).Sum

Write-Host ""
Write-Host "=== FINAL ADMINISTRATOR VERDICT ===" -ForegroundColor Magenta

Write-Host "Hardware Evidence Score: $hardwareScore" -ForegroundColor Green
Write-Host "Software Fallback Score: $softwareScore" -ForegroundColor Red
Write-Host "Access Issues Score: $accessScore" -ForegroundColor Yellow

Write-Host ""

if ($hardwareScore -gt $softwareScore -and $accessScore -eq 0) {
    Write-Host "🎉 HARDWARE TIMESTAMPING CONFIRMED!" -ForegroundColor Green
    Write-Host "✅ Administrator privileges enabled hardware access" -ForegroundColor Green
    Write-Host "✅ Intel hardware timestamping is working" -ForegroundColor Green
} elseif ($accessScore -gt 0) {
    Write-Host "⚠️  PARTIAL SUCCESS - Still some access issues" -ForegroundColor Yellow
    Write-Host "Check: Intel filter driver installation" -ForegroundColor Gray
} elseif ($softwareScore -gt $hardwareScore) {
    Write-Host "❌ STILL USING SOFTWARE FALLBACK" -ForegroundColor Red
    Write-Host "Hardware present but not accessible" -ForegroundColor Gray
} else {
    Write-Host "❓ INCONCLUSIVE RESULTS" -ForegroundColor Yellow
    Write-Host "Review log file: $logFile" -ForegroundColor Gray
}

Write-Host ""
Write-Host "HONEST ASSESSMENT: Administrator test complete" -ForegroundColor Cyan
Write-Host "Press any key to view detailed log..."
pause > $null

# Show key log lines
Write-Host ""
Write-Host "=== KEY LOG EVIDENCE ===" -ForegroundColor White
Get-Content $logFile | Where-Object { 
    $_ -match "timestamp|hardware|software|Intel|OID|Error|WARNING|access" -and
    $_ -notmatch "DEBUG.*Enhanced packet|Parsing.*address"
} | Select-Object -First 20

Write-Host ""
Write-Host "Full log saved to: $logFile" -ForegroundColor Gray
pause
