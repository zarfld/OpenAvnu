Write-Host "=== Simple Interface Format Test ===" -ForegroundColor Cyan
Write-Host ""

$buildDir = "d:\Repos\OpenAvnu\build"
$maapPath = "$buildDir\daemons\maap\Release\maap_daemon.exe"

# Get the Intel I219 adapter
$i219Adapter = Get-NetAdapter | Where-Object { 
    $_.InterfaceDescription -like "*Intel*" -and 
    $_.InterfaceDescription -like "*I219*" -and
    $_.Status -eq "Up"
} | Select-Object -First 1

if ($i219Adapter) {
    Write-Host "Found Intel I219 adapter:" -ForegroundColor Green
    Write-Host "  Name: '$($i219Adapter.Name)'" -ForegroundColor White
    Write-Host "  ifIndex: $($i219Adapter.ifIndex)" -ForegroundColor White
    Write-Host "  InterfaceGuid: $($i219Adapter.InterfaceGuid)" -ForegroundColor White
    Write-Host ""
    
    # Test different formats
    $testFormats = @(
        $i219Adapter.Name,
        $i219Adapter.ifIndex.ToString(),
        "\\Device\\NPF_$($i219Adapter.InterfaceGuid)"
    )
    
    foreach ($format in $testFormats) {
        Write-Host "Testing format: $format" -ForegroundColor Yellow
        
        $psi = New-Object System.Diagnostics.ProcessStartInfo
        $psi.FileName = $maapPath
        $psi.Arguments = "-i `"$format`""
        $psi.WorkingDirectory = $buildDir
        $psi.UseShellExecute = $false
        $psi.RedirectStandardOutput = $true
        $psi.RedirectStandardError = $true
        $psi.CreateNoWindow = $true
        
        $process = [System.Diagnostics.Process]::Start($psi)
        Start-Sleep -Seconds 2
        
        if ($process.HasExited) {
            $exitCode = $process.ExitCode
            $stderr = $process.StandardError.ReadToEnd()
            
            if ($stderr -like "*Interface not found*") {
                Write-Host "  ✗ Interface not found" -ForegroundColor Red
            } else {
                Write-Host "  ? Exit code: $exitCode" -ForegroundColor Yellow
                Write-Host "  Error: $stderr" -ForegroundColor Gray
            }
        } else {
            Write-Host "  ✓ Process started successfully!" -ForegroundColor Green
            $process.Kill()
            $process.WaitForExit(1000)
        }
        Write-Host ""
    }
} else {
    Write-Host "No Intel I219 adapter found!" -ForegroundColor Red
}

Write-Host "=== Test Complete ===" -ForegroundColor Cyan
