# Quick test for daemon functionality
param([string]$DaemonName = "mrpd")

$buildPath = "build\daemons\$DaemonName\Release\$DaemonName"
if ($DaemonName -eq "maap") {
    $buildPath += "_daemon"
}
$buildPath += ".exe"

Write-Host "Testing daemon: $DaemonName" -ForegroundColor Green
Write-Host "Expected path: $buildPath" -ForegroundColor Yellow

if (Test-Path $buildPath) {
    $fileInfo = Get-ItemProperty $buildPath
    Write-Host "✅ Executable exists: $($fileInfo.Length) bytes" -ForegroundColor Green
    
    if ($DaemonName -eq "mrpd") {
        Write-Host "⚠️  Skipping runtime tests for MRPD (requires network interface)" -ForegroundColor Yellow
    } else {
        Write-Host "Testing help output..." -ForegroundColor Cyan
        try {
            $job = Start-Job -ScriptBlock { 
                param($exePath)
                & $exePath --help 2>&1 | Out-String
            } -ArgumentList (Resolve-Path $buildPath)
            
            $output = Wait-Job $job -Timeout 3 | Receive-Job
            Remove-Job $job -Force
            
            if ($output -and $output.Length -gt 10) {
                Write-Host "✅ Help output available ($($output.Length) chars)" -ForegroundColor Green
            } else {
                Write-Host "⚠️  No help output (may need parameters)" -ForegroundColor Yellow
            }
        } catch {
            Write-Host "❌ Help test failed: $($_.Exception.Message)" -ForegroundColor Red
        }
    }
} else {
    Write-Host "❌ Executable not found: $buildPath" -ForegroundColor Red
}
