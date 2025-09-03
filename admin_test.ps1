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

# Find Intel adapter
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
Write-Host "Testing with: $($testAdapter.Name) - $($testAdapter.InterfaceDescription)" -ForegroundColor Green
Write-Host "MAC: $mac" -ForegroundColor White
Write-Host ""

# Run gPTP test
$timestamp = (Get-Date).ToString("yyyyMMdd_HHmmss")
$logFile = "admin_test_$timestamp.err"

Write-Host "Starting gPTP with Administrator privileges..." -ForegroundColor Yellow
Write-Host "Duration: $TestDuration seconds" -ForegroundColor Gray

try {
    $process = Start-Process -FilePath ".\thirdparty\gptp\build\Release\gptp.exe" -ArgumentList $mac -RedirectStandardError $logFile -NoNewWindow -PassThru
    
    Write-Host "Process started (PID: $($process.Id))" -ForegroundColor Green
    
    # Wait with progress
    for ($i = 1; $i -le $TestDuration; $i++) {
        Start-Sleep 1
        $percent = ($i / $TestDuration) * 100
        Write-Progress -Activity "Hardware Timestamp Test" -Status "Collecting evidence..." -PercentComplete $percent
    }
    
    Write-Progress -Completed -Activity "Hardware Timestamp Test"
    
    if (-not $process.HasExited) {
        $process.Kill()
        $process.WaitForExit(3000)
    }
    
    Write-Host "Test complete!" -ForegroundColor Green
    
} catch {
    Write-Host "ERROR: Failed to start gPTP - $($_.Exception.Message)" -ForegroundColor Red
    pause
    exit 1
}

# Analyze results
Write-Host ""
Write-Host "EVIDENCE ANALYSIS" -ForegroundColor Cyan

if (-not (Test-Path $logFile)) {
    Write-Host "ERROR: No log file generated!" -ForegroundColor Red
    pause
    exit 1
}

$content = Get-Content $logFile -Raw

# Key evidence patterns
$hardwareIndicators = @(
    "hardware.*timestamp",
    "Intel.*hardware",
    "PTP.*hardware"
)

$softwareIndicators = @(
    "software.*timestamp", 
    "Enhanced.*software",
    "Falling back.*software",
    "NDIS.*timestamp"
)

$accessIndicators = @(
    "Access.*denied",
    "Administrator.*privilege",
    "OID.*not.*available",
    "Error 5"
)

Write-Host "Hardware Evidence:" -ForegroundColor Green
$hardwareCount = 0
foreach ($pattern in $hardwareIndicators) {
    $found = [regex]::Matches($content, $pattern, [System.Text.RegularExpressions.RegexOptions]::IgnoreCase)
    if ($found.Count -gt 0) {
        Write-Host "  $pattern`: $($found.Count) matches" -ForegroundColor Green
        $hardwareCount += $found.Count
    }
}

Write-Host "Software Fallback Evidence:" -ForegroundColor Red  
$softwareCount = 0
foreach ($pattern in $softwareIndicators) {
    $found = [regex]::Matches($content, $pattern, [System.Text.RegularExpressions.RegexOptions]::IgnoreCase)
    if ($found.Count -gt 0) {
        Write-Host "  $pattern`: $($found.Count) matches" -ForegroundColor Red
        $softwareCount += $found.Count
    }
}

Write-Host "Access Issues:" -ForegroundColor Yellow
$accessCount = 0
foreach ($pattern in $accessIndicators) {
    $found = [regex]::Matches($content, $pattern, [System.Text.RegularExpressions.RegexOptions]::IgnoreCase)
    if ($found.Count -gt 0) {
        Write-Host "  $pattern`: $($found.Count) matches" -ForegroundColor Yellow
        $accessCount += $found.Count
    }
}

Write-Host ""
Write-Host "ADMINISTRATOR VERDICT:" -ForegroundColor Magenta

if ($hardwareCount -gt 0 -and $softwareCount -eq 0 -and $accessCount -eq 0) {
    Write-Host "SUCCESS: Hardware timestamping confirmed!" -ForegroundColor Green
    Write-Host "Administrator privileges enabled hardware access" -ForegroundColor Green
} elseif ($accessCount -gt 0) {
    Write-Host "PARTIAL: Still access issues despite admin privileges" -ForegroundColor Yellow
} elseif ($softwareCount -gt 0) {
    Write-Host "FAILURE: Still using software fallback" -ForegroundColor Red
} else {
    Write-Host "INCONCLUSIVE: Check log manually" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "Key log entries:" -ForegroundColor White
Get-Content $logFile | Where-Object { 
    $_ -match "timestamp|hardware|software|Intel|OID|Error|access" -and
    $_ -notmatch "DEBUG.*Enhanced packet"
} | Select-Object -First 10

Write-Host ""
Write-Host "Full log: $logFile" -ForegroundColor Gray
Write-Host "Press any key to continue..."
pause > $null
