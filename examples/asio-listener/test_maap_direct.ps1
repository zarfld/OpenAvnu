Write-Host "=== MAAP Daemon Direct Test ===" -ForegroundColor Cyan
Write-Host ""

$buildDir = "d:\Repos\OpenAvnu\build"
$maapPath = "$buildDir\daemons\maap\Release\maap_daemon.exe"
$networkInterface = "Ethernet"

Write-Host "Testing MAAP daemon with interface parameter..." -ForegroundColor Yellow
Write-Host "Command: $maapPath -i `"$networkInterface`"" -ForegroundColor Gray
Write-Host ""

try {
    # Start MAAP with interface parameter and capture output
    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName = $maapPath
    $psi.Arguments = "-i `"$networkInterface`""
    $psi.WorkingDirectory = $buildDir
    $psi.UseShellExecute = $false
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    $psi.CreateNoWindow = $true
    
    $process = [System.Diagnostics.Process]::Start($psi)
    Write-Host "MAAP daemon started (PID: $($process.Id))" -ForegroundColor Green
    
    # Wait a bit and check status
    Start-Sleep -Seconds 3
    
    if ($process.HasExited) {
        $exitCode = $process.ExitCode
        $stdout = $process.StandardOutput.ReadToEnd()
        $stderr = $process.StandardError.ReadToEnd()
        
        Write-Host "MAAP daemon exited with code: $exitCode" -ForegroundColor Red
        
        if ($stdout) {
            Write-Host ""
            Write-Host "=== STDOUT ===" -ForegroundColor Yellow
            Write-Host $stdout -ForegroundColor White
        }
        
        if ($stderr) {
            Write-Host ""
            Write-Host "=== STDERR ===" -ForegroundColor Red
            Write-Host $stderr -ForegroundColor White
        }
    } else {
        Write-Host "MAAP daemon is running successfully!" -ForegroundColor Green
        
        # Let it run for a moment to see if it stays stable
        Start-Sleep -Seconds 5
        
        if ($process.HasExited) {
            Write-Host "MAAP daemon exited after 5 seconds" -ForegroundColor Yellow
        } else {
            Write-Host "MAAP daemon is stable and running!" -ForegroundColor Green
            Write-Host "Terminating for test completion..." -ForegroundColor Gray
            $process.Kill()
            $process.WaitForExit(3000)
        }
    }
    
} catch {
    Write-Host "ERROR: Failed to start MAAP daemon" -ForegroundColor Red
    Write-Host "Exception: $($_.Exception.Message)" -ForegroundColor Red
}

Write-Host ""
Write-Host "=== Test Complete ===" -ForegroundColor Cyan
