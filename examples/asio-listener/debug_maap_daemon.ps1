Write-Host "=== MAAP Daemon Debug Analysis ===" -ForegroundColor Cyan
Write-Host ""

$buildDir = "d:\Repos\OpenAvnu\build"
$maapPath = "$buildDir\daemons\maap\Release\maap_daemon.exe"

# Step 1: Check if MAAP executable exists
Write-Host "Step 1: Checking MAAP daemon executable..." -ForegroundColor Yellow
if (Test-Path $maapPath) {
    Write-Host "SUCCESS: MAAP executable found at: $maapPath" -ForegroundColor Green
    
    # Get file info
    $fileInfo = Get-Item $maapPath
    Write-Host "File Size: $($fileInfo.Length) bytes" -ForegroundColor White
    Write-Host "Last Modified: $($fileInfo.LastWriteTime)" -ForegroundColor White
    Write-Host ""
} else {
    Write-Host "ERROR: MAAP executable not found at: $maapPath" -ForegroundColor Red
    Write-Host "You need to build the MAAP daemon first!" -ForegroundColor Red
    exit 1
}

# Step 2: Check dependencies
Write-Host "Step 2: Checking MAAP daemon dependencies..." -ForegroundColor Yellow
try {
    # Try to get dependency information
    $dependencies = & "dumpbin" /dependents $maapPath 2>$null
    if ($LASTEXITCODE -eq 0) {
        Write-Host "Dependencies check passed" -ForegroundColor Green
    } else {
        Write-Host "Could not check dependencies (dumpbin not available)" -ForegroundColor Yellow
    }
} catch {
    Write-Host "Dependency check skipped (tools not available)" -ForegroundColor Yellow
}
Write-Host ""

# Step 3: Try to run MAAP with verbose output
Write-Host "Step 3: Testing MAAP daemon startup with debug output..." -ForegroundColor Yellow
Write-Host "Starting MAAP daemon and capturing output..." -ForegroundColor Gray

try {
    # Create a temporary log file
    $logFile = Join-Path $env:TEMP "maap_debug.log"
    Write-Host "Log file: $logFile" -ForegroundColor Gray
    
    # Start MAAP process with output redirection
    $processInfo = New-Object System.Diagnostics.ProcessStartInfo
    $processInfo.FileName = $maapPath
    $processInfo.WorkingDirectory = $buildDir
    $processInfo.UseShellExecute = $false
    $processInfo.RedirectStandardOutput = $true
    $processInfo.RedirectStandardError = $true
    $processInfo.CreateNoWindow = $true
    
    $process = New-Object System.Diagnostics.Process
    $process.StartInfo = $processInfo
    
    # Event handlers for output
    $outputBuilder = New-Object System.Text.StringBuilder
    $errorBuilder = New-Object System.Text.StringBuilder
    
    $outputHandler = {
        if ($Event.SourceEventArgs.Data -ne $null) {
            $outputBuilder.AppendLine($Event.SourceEventArgs.Data)
        }
    }
    
    $errorHandler = {
        if ($Event.SourceEventArgs.Data -ne $null) {
            $errorBuilder.AppendLine($Event.SourceEventArgs.Data)
        }
    }
    
    Register-ObjectEvent -InputObject $process -EventName OutputDataReceived -Action $outputHandler | Out-Null
    Register-ObjectEvent -InputObject $process -EventName ErrorDataReceived -Action $errorHandler | Out-Null
    
    # Start the process
    $process.Start() | Out-Null
    $process.BeginOutputReadLine()
    $process.BeginErrorReadLine()
    
    Write-Host "MAAP daemon started (PID: $($process.Id))" -ForegroundColor Green
    Write-Host "Waiting 5 seconds to capture startup messages..." -ForegroundColor Gray
    
    # Wait for 5 seconds
    Start-Sleep -Seconds 5
    
    # Check if process is still running
    if ($process.HasExited) {
        Write-Host "MAAP daemon exited with code: $($process.ExitCode)" -ForegroundColor Red
        
        # Clean up event handlers
        Get-EventSubscriber | Where-Object { $_.SourceObject -eq $process } | Unregister-Event
        
        # Show captured output
        $stdout = $outputBuilder.ToString()
        $stderr = $errorBuilder.ToString()
        
        if ($stdout) {
            Write-Host "" 
            Write-Host "=== STDOUT Output ===" -ForegroundColor Yellow
            Write-Host $stdout -ForegroundColor White
        }
        
        if ($stderr) {
            Write-Host ""
            Write-Host "=== STDERR Output ===" -ForegroundColor Red
            Write-Host $stderr -ForegroundColor White
        }
        
        if (-not $stdout -and -not $stderr) {
            Write-Host "No output captured - daemon may have crashed immediately" -ForegroundColor Red
        }
        
        # Save to log file
        $logContent = "MAAP Daemon Debug Log - $(Get-Date)`n"
        $logContent += "Exit Code: $($process.ExitCode)`n"
        $logContent += "STDOUT:`n$stdout`n"
        $logContent += "STDERR:`n$stderr`n"
        $logContent | Out-File -FilePath $logFile -Encoding UTF8
        
        Write-Host ""
        Write-Host "Debug log saved to: $logFile" -ForegroundColor Cyan
        
    } else {
        Write-Host "MAAP daemon is running successfully!" -ForegroundColor Green
        Write-Host "Stopping daemon for analysis..." -ForegroundColor Gray
        $process.Kill()
        $process.WaitForExit(3000)
        
        # Clean up event handlers
        Get-EventSubscriber | Where-Object { $_.SourceObject -eq $process } | Unregister-Event
    }
    
} catch {
    Write-Host "ERROR: Failed to start MAAP daemon: $($_.Exception.Message)" -ForegroundColor Red
    Write-Host "Exception Type: $($_.Exception.GetType().Name)" -ForegroundColor Red
}

Write-Host ""
Write-Host "Step 4: Checking common MAAP issues..." -ForegroundColor Yellow

# Check if MRPD is running (MAAP might depend on it)
$mrpdProcess = Get-Process -Name "mrpd" -ErrorAction SilentlyContinue
if ($mrpdProcess) {
    Write-Host "MRPD daemon is running (PID: $($mrpdProcess.Id))" -ForegroundColor Green
} else {
    Write-Host "MRPD daemon is NOT running - MAAP might require it!" -ForegroundColor Yellow
    Write-Host "Try starting MRPD first, then MAAP" -ForegroundColor Yellow
}

# Check network interface status
Write-Host "Checking network interfaces..." -ForegroundColor Gray
$activeInterfaces = Get-NetAdapter | Where-Object { $_.Status -eq "Up" } | Select-Object Name, InterfaceDescription
foreach ($interface in $activeInterfaces) {
    Write-Host "  - $($interface.Name): $($interface.InterfaceDescription)" -ForegroundColor White
}

Write-Host ""
Write-Host "=== MAAP Debug Analysis Complete ===" -ForegroundColor Cyan
