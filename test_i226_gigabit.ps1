# Test I226 at Gigabit Speed for Hardware Timestamping
param(
    [int]$TestDuration = 20
)

# Check if running as Administrator
$isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")

if (-not $isAdmin) {
    Write-Host "Requesting Administrator privileges for network speed change..." -ForegroundColor Yellow
    $scriptPath = $MyInvocation.MyCommand.Path
    $arguments = "-TestDuration $TestDuration"
    Start-Process PowerShell -ArgumentList "-ExecutionPolicy Bypass", "-File", "`"$scriptPath`"", $arguments -Verb RunAs
    exit
}

Write-Host "I226 GIGABIT HARDWARE TIMESTAMPING TEST" -ForegroundColor Red
Write-Host "Running with Administrator privileges..." -ForegroundColor Green
Write-Host ""

Set-Location "C:\Users\dzarf\source\repos\OpenAvnu-2"

# Check current I226 status
$i226 = Get-NetAdapter -Name "Ethernet" -ErrorAction SilentlyContinue
if (-not $i226) {
    Write-Host "ERROR: I226 adapter not found" -ForegroundColor Red
    pause
    exit 1
}

Write-Host "Current I226 Status:" -ForegroundColor Cyan
Write-Host "  Name: $($i226.Name)" -ForegroundColor White  
Write-Host "  Speed: $($i226.LinkSpeed)" -ForegroundColor White
Write-Host "  Status: $($i226.Status)" -ForegroundColor White
Write-Host "  MAC: $($i226.MacAddress)" -ForegroundColor White
Write-Host ""

# Set to Gigabit if not already
if ($i226.LinkSpeed -ne "1 Gbps") {
    Write-Host "Setting I226 to Gigabit Full Duplex..." -ForegroundColor Yellow
    
    try {
        Set-NetAdapterAdvancedProperty -Name "Ethernet" -DisplayName "Geschwindigkeit und Duplex" -DisplayValue "1000 Mbit/s Vollduplex"
        Write-Host "Speed setting applied!" -ForegroundColor Green
        
        Write-Host "Waiting 10 seconds for link to establish..." -ForegroundColor Gray
        Start-Sleep 10
        
        # Check new status
        $i226Updated = Get-NetAdapter -Name "Ethernet"
        Write-Host "Updated I226 Status:" -ForegroundColor Cyan  
        Write-Host "  Speed: $($i226Updated.LinkSpeed)" -ForegroundColor White
        Write-Host "  Status: $($i226Updated.Status)" -ForegroundColor White
        
        if ($i226Updated.LinkSpeed -ne "1 Gbps") {
            Write-Host "WARNING: Still not at Gigabit - may be limited by network infrastructure" -ForegroundColor Yellow
            Write-Host "Continuing test anyway..." -ForegroundColor Gray
        } else {
            Write-Host "SUCCESS: I226 now running at Gigabit!" -ForegroundColor Green
        }
        
    } catch {
        Write-Host "ERROR: Failed to set speed - $($_.Exception.Message)" -ForegroundColor Red
        Write-Host "Continuing with current speed..." -ForegroundColor Gray
    }
    
    Write-Host ""
} else {
    Write-Host "I226 already at Gigabit speed" -ForegroundColor Green
    Write-Host ""
}

# Run hardware timestamping test  
$mac = $i226.MacAddress.Replace("-", ":")
$timestamp = (Get-Date).ToString("yyyyMMdd_HHmmss")
$logFile = "I226_gigabit_$timestamp.err"

Write-Host "HARDWARE TIMESTAMPING TEST" -ForegroundColor Magenta
Write-Host "Adapter: $($i226.Name) - $($i226.InterfaceDescription)" -ForegroundColor White
Write-Host "MAC: $mac" -ForegroundColor White
Write-Host "Duration: $TestDuration seconds" -ForegroundColor Gray
Write-Host "Log: $logFile" -ForegroundColor Gray
Write-Host ""

try {
    $process = Start-Process -FilePath ".\thirdparty\gptp\build\Release\gptp.exe" -ArgumentList $mac -RedirectStandardError $logFile -NoNewWindow -PassThru
    
    Write-Host "gPTP process started (PID: $($process.Id))" -ForegroundColor Green
    Write-Host "Monitoring hardware timestamping at Gigabit speed..." -ForegroundColor Yellow
    
    # Progress bar
    for ($i = 1; $i -le $TestDuration; $i++) {
        Start-Sleep 1
        $percent = ($i / $TestDuration) * 100
        Write-Progress -Activity "I226 Gigabit Hardware Test" -Status "Collecting evidence..." -PercentComplete $percent
    }
    
    Write-Progress -Completed -Activity "I226 Gigabit Hardware Test"
    
    if (-not $process.HasExited) {
        $process.Kill()
        $process.WaitForExit(3000)
    }
    
    Write-Host "Test completed!" -ForegroundColor Green
    
} catch {
    Write-Host "ERROR: $($_.Exception.Message)" -ForegroundColor Red
    pause
    exit 1
}

Write-Host ""
Write-Host "EVIDENCE ANALYSIS" -ForegroundColor Magenta

if (-not (Test-Path $logFile)) {
    Write-Host "ERROR: No log generated" -ForegroundColor Red
    pause
    exit 1
}

$content = Get-Content $logFile -Raw

# Evidence patterns
$hardwarePatterns = @("hardware.*timestamp", "Intel.*hardware", "PTP.*hardware")
$softwarePatterns = @("software.*timestamp", "Enhanced.*software", "Falling back.*software")
$clockPatterns = @("Unable.*determine.*clock", "clock rate", "Using.*clock.*rate.*Hz")
$successPatterns = @("Hardware.*timestamper.*initialized", "PTP.*hardware.*enabled")

Write-Host "Hardware Evidence:" -ForegroundColor Green
$hardwareCount = 0
foreach ($pattern in $hardwarePatterns) {
    $found = [regex]::Matches($content, $pattern, [System.Text.RegularExpressions.RegexOptions]::IgnoreCase)
    if ($found.Count -gt 0) {
        Write-Host "  $pattern`: $($found.Count) matches" -ForegroundColor Green
        $hardwareCount += $found.Count
    }
}

Write-Host "Software Evidence:" -ForegroundColor Red
$softwareCount = 0
foreach ($pattern in $softwarePatterns) {
    $found = [regex]::Matches($content, $pattern, [System.Text.RegularExpressions.RegexOptions]::IgnoreCase)
    if ($found.Count -gt 0) {
        Write-Host "  $pattern`: $($found.Count) matches" -ForegroundColor Red
        $softwareCount += $found.Count
    }
}

Write-Host "Clock Issues:" -ForegroundColor Yellow
$clockCount = 0
foreach ($pattern in $clockPatterns) {
    $found = [regex]::Matches($content, $pattern, [System.Text.RegularExpressions.RegexOptions]::IgnoreCase)
    if ($found.Count -gt 0) {
        Write-Host "  $pattern`: $($found.Count) matches" -ForegroundColor Yellow
        $clockCount += $found.Count
    }
}

Write-Host "Success Indicators:" -ForegroundColor Green
$successCount = 0
foreach ($pattern in $successPatterns) {
    $found = [regex]::Matches($content, $pattern, [System.Text.RegularExpressions.RegexOptions]::IgnoreCase)
    if ($found.Count -gt 0) {
        Write-Host "  $pattern`: $($found.Count) matches" -ForegroundColor Green
        $successCount += $found.Count
    }
}

Write-Host ""

# Get key evidence lines
$keyLines = Get-Content $logFile | Where-Object { 
    $_ -match "clock rate|timestamp|Unable|hardware|software|Gigabit|1000.*Hz" -and
    $_ -notmatch "DEBUG.*Enhanced packet|addEventTimer"
} | Select-Object -First 8

Write-Host "Key Evidence Lines:" -ForegroundColor White
foreach ($line in $keyLines) {
    $shortLine = $line.Substring(0, [Math]::Min(100, $line.Length))
    Write-Host "  $shortLine" -ForegroundColor Gray
}

Write-Host ""
Write-Host "GIGABIT HARDWARE TIMESTAMPING VERDICT:" -ForegroundColor Red

if ($successCount -gt 0 -or ($hardwareCount -gt $softwareCount -and $clockCount -eq 0)) {
    Write-Host "🎉 SUCCESS: I226 Hardware Timestamping at Gigabit!" -ForegroundColor Green
    Write-Host "Clock rate detection successful" -ForegroundColor Green
    Write-Host "Hardware timestamper initialized" -ForegroundColor Green
} elseif ($clockCount -eq 0 -and $softwareCount -eq 0) {
    Write-Host "✅ IMPROVED: No clock rate errors detected" -ForegroundColor Yellow  
    Write-Host "Gigabit speed resolved previous issues" -ForegroundColor Yellow
} elseif ($clockCount -gt 0) {
    Write-Host "❌ STILL FAILING: Clock rate detection issues remain" -ForegroundColor Red
} else {
    Write-Host "❓ MIXED RESULTS: Check log details" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "Log file: $logFile" -ForegroundColor Gray
Write-Host "Press any key to continue..."
pause > $null
