# ============================================================================
# Stop OpenAvnu Daemons Script
# ============================================================================

Write-Host ""
Write-Host "🛑 Stopping OpenAvnu Daemons" -ForegroundColor Red
Write-Host "==============================" -ForegroundColor Red
Write-Host ""

Write-Host "[CLEANUP] Stopping OpenAvnu Daemons..." -ForegroundColor Yellow

$daemons = Get-Process -Name "mrpd","maap_daemon" -ErrorAction SilentlyContinue

if ($daemons) {
    $daemons | ForEach-Object { 
        Write-Host "🔴 Stopping $($_.ProcessName) (PID: $($_.Id))" -ForegroundColor Red
        Stop-Process -Id $_.Id -Force 
    }
    
    Start-Sleep -Seconds 2
    
    # Verify they're stopped
    $remainingDaemons = Get-Process -Name "mrpd","maap_daemon" -ErrorAction SilentlyContinue
    if ($remainingDaemons) {
        Write-Host "⚠️  Some daemons still running:" -ForegroundColor Yellow
        $remainingDaemons | Select-Object ProcessName, Id | Format-Table -AutoSize
    } else {
        Write-Host "✅ All OpenAvnu daemons stopped successfully" -ForegroundColor Green
    }
} else {
    Write-Host "ℹ️  No OpenAvnu daemons were running" -ForegroundColor Blue
}

Write-Host ""
