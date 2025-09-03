# Intel HAL Admin vs User Comparison
param([switch]$Admin)

$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$isCurrentlyAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltinRole] "Administrator")

if ($Admin) {
    Write-Host "🔒 ADMIN TEST: Running as Administrator" -ForegroundColor Yellow
    $logFile = "admin_test_$timestamp.log"
    
    if (-not $isCurrentlyAdmin) {
        Write-Host "❌ ERROR: Must run PowerShell as Administrator for -Admin flag" -ForegroundColor Red
        exit 1
    }
} else {
    Write-Host "👤 USER TEST: Running as regular user" -ForegroundColor Green
    $logFile = "user_test_$timestamp.log"
    
    if ($isCurrentlyAdmin) {
        Write-Host "⚠️ WARNING: You are running as Admin but testing user mode" -ForegroundColor Yellow
    }
}

Write-Host "Administrator Status: $isCurrentlyAdmin" -ForegroundColor Cyan
Write-Host "Log File: $logFile" -ForegroundColor Cyan
Write-Host ""

# Run the test and capture output
.\build\tests\intel_hal_validation_test_consolidated.exe 2>&1 | Tee-Object -FilePath $logFile

Write-Host ""
Write-Host "✅ Test completed. Results saved to: $logFile" -ForegroundColor Green
