# Intel HAL Admin vs User Comparison Test
param([switch]$Admin)

$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltinRole] "Administrator")

if ($Admin) {
    Write-Host "ADMIN TEST: Running with Administrator privileges" -ForegroundColor Yellow
    $logFile = "admin_test_$timestamp.log"
    
    if (-not $isAdmin) {
        Write-Host "ERROR: Must run PowerShell as Administrator for -Admin flag" -ForegroundColor Red
        exit 1
    }
} else {
    Write-Host "USER TEST: Running as regular user" -ForegroundColor Green
    $logFile = "user_test_$timestamp.log"
    
    if ($isAdmin) {
        Write-Host "WARNING: You are running as Admin but testing user mode" -ForegroundColor Yellow
    }
}

Write-Host "Current Administrator Status: $isAdmin" -ForegroundColor Cyan
Write-Host "Output will be saved to: $logFile" -ForegroundColor Cyan
Write-Host ""

# Run the test and save output
$output = .\build\tests\intel_hal_validation_test_consolidated.exe 2>&1 | Tee-Object -FilePath $logFile

Write-Host ""
Write-Host "Test completed successfully. Results saved to: $logFile" -ForegroundColor Green

# Extract key metrics for quick comparison
$macCount = ($output | Select-String "MAC: [0-9A-F]").Count
$ndisWarnings = ($output | Select-String "NDIS.*failed").Count
$avgPrecision = $output | Select-String "Avg resolution: (\d+)" | ForEach-Object { $_.Matches.Groups[1].Value }

Write-Host ""
Write-Host "QUICK SUMMARY:" -ForegroundColor Magenta
Write-Host "- MAC Addresses Found: $macCount" -ForegroundColor White
Write-Host "- NDIS Query Failures: $ndisWarnings" -ForegroundColor White
Write-Host "- Timestamp Precision: $($avgPrecision -join ', ') ns" -ForegroundColor White
