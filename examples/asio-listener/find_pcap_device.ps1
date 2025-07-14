Write-Host "=== PCap Device Discovery for MAAP ===" -ForegroundColor Cyan
Write-Host ""

# Check if we have a pcap tool available
$pcapPath = "C:\Program Files\Wireshark\tshark.exe"
if (Test-Path $pcapPath) {
    Write-Host "Using Wireshark's tshark to list interfaces..." -ForegroundColor Yellow
    & $pcapPath -D
} else {
    Write-Host "Wireshark not found, using alternative method..." -ForegroundColor Yellow
}

Write-Host ""
Write-Host "Testing common pcap device name patterns..." -ForegroundColor Yellow

$buildDir = "d:\Repos\OpenAvnu\build"
$maapPath = "$buildDir\daemons\maap\Release\maap_daemon.exe"

# Get the Intel I219 interface GUID
$i219Adapter = Get-NetAdapter | Where-Object { 
    $_.InterfaceDescription -like "*Intel*" -and 
    $_.InterfaceDescription -like "*I219*" -and
    $_.Status -eq "Up"
} | Select-Object -First 1

if ($i219Adapter) {
    $guid = $i219Adapter.InterfaceGuid
    Write-Host "Intel I219 Interface GUID: $guid" -ForegroundColor Green
    
    # Common pcap device name patterns for Windows
    $pcapPatterns = @(
        "\Device\NPF_$guid",
        "\\Device\\NPF_$guid",
        "rpcap://\Device\NPF_$guid",
        "\Device\NPF_$($guid.Replace('{','').Replace('}',''))",
        "\\Device\\NPF_$($guid.Replace('{','').Replace('}',''))"
    )
    
    foreach ($pattern in $pcapPatterns) {
        Write-Host ""
        Write-Host "Testing: $pattern" -ForegroundColor Cyan
        
        $psi = New-Object System.Diagnostics.ProcessStartInfo
        $psi.FileName = $maapPath
        $psi.Arguments = "-i `"$pattern`""
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
                Write-Host "  ✗ Not found" -ForegroundColor Red
            } else {
                Write-Host "  ? Exit code: $exitCode, Error: $stderr" -ForegroundColor Yellow
            }
        } else {
            Write-Host "  ✓ SUCCESS! MAAP started with this pattern!" -ForegroundColor Green
            $process.Kill()
            $process.WaitForExit(1000)
            
            Write-Host ""
            Write-Host "=== SOLUTION FOUND ===" -ForegroundColor Green
            Write-Host "Use this device name for MAAP: $pattern" -ForegroundColor Green
            break
        }
    }
} else {
    Write-Host "Intel I219 adapter not found!" -ForegroundColor Red
}

Write-Host ""
Write-Host "=== PCap Device Discovery Complete ===" -ForegroundColor Cyan
