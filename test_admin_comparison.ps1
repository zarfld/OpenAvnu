#!/usr/bin/env powershell
# Intel HAL Admin vs Non-Admin Comparison Test
# Run Intel HAL validation test as both admin and non-admin to compare results

param(
    [switch]$AsAdmin,
    [switch]$SaveResults
)

$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$testExe = ".\build\tests\intel_hal_validation_test_consolidated.exe"

if ($AsAdmin) {
    Write-Host "🔒 Running Intel HAL validation test AS ADMINISTRATOR..." -ForegroundColor Yellow
    Write-Host "   This should provide enhanced hardware access and privileges" -ForegroundColor Cyan
    
    $outputFile = "admin_test_results_$timestamp.log"
    
    # Check if running as admin
    $isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltinRole] "Administrator")
    
    if (-not $isAdmin) {
        Write-Host "❌ ERROR: Script must be run as Administrator when using -AsAdmin flag" -ForegroundColor Red
        Write-Host "   Please run PowerShell as Administrator and try again" -ForegroundColor Yellow
        exit 1
    }
    
    Write-Host "✅ Confirmed: Running with Administrator privileges" -ForegroundColor Green
    Write-Host "   Enhanced hardware access should be available" -ForegroundColor Cyan
    Write-Host ""
    
} else {
    Write-Host "👤 Running Intel HAL validation test as REGULAR USER..." -ForegroundColor Green
    Write-Host "   This uses standard user-space hardware access" -ForegroundColor Cyan
    
    $outputFile = "user_test_results_$timestamp.log"
    
    # Check if NOT running as admin
    $isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltinRole] "Administrator")
    
    if ($isAdmin) {
        Write-Host "⚠️  WARNING: You are running as Administrator" -ForegroundColor Yellow
        Write-Host "   Results may not reflect true non-admin behavior" -ForegroundColor Yellow
        Write-Host "   For accurate comparison, run from non-elevated PowerShell" -ForegroundColor Yellow
        Write-Host ""
    }
}

Write-Host "📋 Test Configuration:" -ForegroundColor Magenta
Write-Host "   • Executable: $testExe" -ForegroundColor White
Write-Host "   • Administrator: $($AsAdmin)" -ForegroundColor White
Write-Host "   • Save Results: $($SaveResults)" -ForegroundColor White
Write-Host "   • Output File: $outputFile" -ForegroundColor White
Write-Host ""

# Verify test executable exists
if (-not (Test-Path $testExe)) {
    Write-Host "❌ ERROR: Test executable not found: $testExe" -ForegroundColor Red
    Write-Host "   Please build the project first with cmake --build . --config Release" -ForegroundColor Yellow
    exit 1
}

Write-Host "🚀 Starting Intel HAL Validation Test..." -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green

try {
    if ($SaveResults) {
        # Run test and capture output
        $output = & $testExe 2>&1 | Tee-Object -FilePath $outputFile
        
        Write-Host ""
        Write-Host "📁 Results saved to: $outputFile" -ForegroundColor Cyan
        
        # Extract key metrics for comparison
        $macAddresses = @()
        $speeds = @()
        $timestampPrecision = @()
        
        foreach ($line in $output) {
            if ($line -match "MAC: ([0-9A-F:]{17})") {
                $macAddresses += $matches[1]
            }
            if ($line -match "Speed: (\d+) Mbps") {
                $speeds += [int]$matches[1]
            }
            if ($line -match "Avg resolution: (\d+) ns") {
                $timestampPrecision += [int]$matches[1]
            }
        }
        
        Write-Host ""
        Write-Host "📊 KEY METRICS EXTRACTED:" -ForegroundColor Magenta
        Write-Host "   • MAC Addresses Found: $($macAddresses.Count)" -ForegroundColor White
        foreach ($mac in $macAddresses) {
            Write-Host "     - $mac" -ForegroundColor Cyan
        }
        Write-Host "   • Adapter Speeds: $($speeds -join ', ') Mbps" -ForegroundColor White
        Write-Host "   • Timestamp Precision: $($timestampPrecision -join ', ') ns" -ForegroundColor White
        
        # Check for admin-specific enhancements
        $ndisWarnings = ($output | Where-Object { $_ -match "NDIS.*failed" }).Count
        $hardwareAccess = ($output | Where-Object { $_ -match "hardware register|direct access|MMIO" }).Count
        $adminFeatures = ($output | Where-Object { $_ -match "administrator|elevated|privileged" }).Count
        
        Write-Host ""
        Write-Host "🔍 PRIVILEGE-RELATED ANALYSIS:" -ForegroundColor Magenta
        Write-Host "   • NDIS Query Failures: $ndisWarnings" -ForegroundColor White
        Write-Host "   • Hardware Access References: $hardwareAccess" -ForegroundColor White  
        Write-Host "   • Admin Feature References: $adminFeatures" -ForegroundColor White
        
        if ($AsAdmin -and $ndisWarnings -gt 0) {
            Write-Host "   ⚠️  Even with admin privileges, NDIS queries failed" -ForegroundColor Yellow
            Write-Host "      This may indicate driver or hardware access limitations" -ForegroundColor Yellow
        }
        
    } else {
        # Just run the test normally
        & $testExe
    }
    
} catch {
    Write-Host "❌ ERROR running Intel HAL validation test:" -ForegroundColor Red
    Write-Host "   $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "✅ Intel HAL validation test completed successfully" -ForegroundColor Green

if ($SaveResults) {
    Write-Host "📄 Full results available in: $outputFile" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "💡 COMPARISON TIPS:" -ForegroundColor Yellow
    Write-Host "   1. Run this script twice:" -ForegroundColor White
    Write-Host "      • Once as regular user: .\test_admin_comparison.ps1 -SaveResults" -ForegroundColor Cyan
    Write-Host "      • Once as administrator: .\test_admin_comparison.ps1 -AsAdmin -SaveResults" -ForegroundColor Cyan
    Write-Host "   2. Compare the log files to identify admin-specific enhancements" -ForegroundColor White
    Write-Host "   3. Look for differences in:" -ForegroundColor White
    Write-Host "      • MAC address retrieval success" -ForegroundColor Gray
    Write-Host "      • NDIS query success/failure rates" -ForegroundColor Gray
    Write-Host "      • Hardware register access" -ForegroundColor Gray
    Write-Host "      • Timestamp precision improvements" -ForegroundColor Gray
}
