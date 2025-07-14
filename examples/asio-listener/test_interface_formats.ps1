Write-Host "=== Interface Name Resolution Test ===" -ForegroundColor Cyan
Write-Host ""

$buildDir = "d:\Repos\OpenAvnu\build"
$maapPath = "$buildDir\daemons\maap\Release\maap_daemon.exe"

# Get all network adapters
Write-Host "Available network interfaces:" -ForegroundColor Yellow
$adapters = Get-NetAdapter | Where-Object { $_.Status -eq "Up" }
foreach ($adapter in $adapters) {
    Write-Host "  Name: '$($adapter.Name)'" -ForegroundColor White
    Write-Host "  Description: '$($adapter.InterfaceDescription)'" -ForegroundColor Gray
    Write-Host "  ifIndex: $($adapter.ifIndex)" -ForegroundColor Gray
    Write-Host "  InterfaceGuid: $($adapter.InterfaceGuid)" -ForegroundColor Gray
    Write-Host "  ---" -ForegroundColor Gray
}

Write-Host ""
Write-Host "Testing different interface name formats..." -ForegroundColor Yellow

# Test various formats for the Intel I219 interface
$testFormats = @(
    "Ethernet",
    "ethernet", 
    '"Ethernet"',
    "$($adapters[3].ifIndex)",  # Use ifIndex
    $adapters[3].InterfaceGuid,  # Use GUID
    "Intel(R) Ethernet Connection (22) I219-LM"  # Full description
)

foreach ($format in $testFormats) {
    Write-Host ""
    Write-Host "Testing format: $format" -ForegroundColor Cyan
    
    try {
        $psi = New-Object System.Diagnostics.ProcessStartInfo
        $psi.FileName = $maapPath
        $psi.Arguments = "-i $format"
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
            
            if ($exitCode -eq -1 -and $stderr -like "*Interface not found*") {
                Write-Host "  ✗ Interface not found" -ForegroundColor Red
            } elseif ($exitCode -eq 0) {
                Write-Host "  ✓ SUCCESS! Interface accepted" -ForegroundColor Green
                $process.Kill()
                $process.WaitForExit(1000)
                break
            } else {
                Write-Host "  ? Exit code: $exitCode" -ForegroundColor Yellow
                if ($stderr) {
                    Write-Host "    Error: $stderr" -ForegroundColor Gray
                }
            }
        } else {
            Write-Host "  ✓ Process running (may be success)" -ForegroundColor Green
            $process.Kill()
            $process.WaitForExit(1000)
        }
        
    } catch {
        Write-Host "  ✗ Exception: $($_.Exception.Message)" -ForegroundColor Red
    }
}

Write-Host ""
Write-Host "Checking WinPcap/Npcap device names..." -ForegroundColor Yellow

# Check for WinPcap/Npcap style interface names
try {
    # Try to find interface names that pcap libraries use
    $regPath = "HKLM:\SYSTEM\CurrentControlSet\Services\Tcpip\Parameters\Interfaces"
    if (Test-Path $regPath) {
        Write-Host "Network interface registry entries:" -ForegroundColor Gray
        $interfaces = Get-ChildItem $regPath
        foreach ($interface in $interfaces) {
            $dhcp = (Get-ItemProperty -Path $interface.PSPath -Name "EnableDHCP" -ErrorAction SilentlyContinue).EnableDHCP
            $desc = (Get-ItemProperty -Path $interface.PSPath -Name "Description" -ErrorAction SilentlyContinue).Description
            if ($desc -like "*Intel*I219*") {
                Write-Host "  Found Intel I219 interface: $($interface.PSChildName)" -ForegroundColor Green
                Write-Host "    Description: $desc" -ForegroundColor White
                
                # Test this GUID format
                Write-Host ""
                Write-Host "Testing GUID format: $($interface.PSChildName)" -ForegroundColor Cyan
                
                try {
                    $psi = New-Object System.Diagnostics.ProcessStartInfo
                    $psi.FileName = $maapPath
                    $psi.Arguments = "-i $($interface.PSChildName)"
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
                        
                        if ($exitCode -eq -1 -and $stderr -like "*Interface not found*") {
                            Write-Host "  ✗ GUID format failed" -ForegroundColor Red
                        } else {
                            Write-Host "  ✓ GUID format may work!" -ForegroundColor Green
                        }
                    } else {
                        Write-Host "  ✓ GUID format working!" -ForegroundColor Green
                        $process.Kill()
                        $process.WaitForExit(1000)
                    }
                } catch {
                    Write-Host "  ✗ GUID test failed: $($_.Exception.Message)" -ForegroundColor Red
                }
            }
        }
    }
} catch {
    Write-Host "Could not check registry interfaces" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "=== Interface Resolution Test Complete ===" -ForegroundColor Cyan
