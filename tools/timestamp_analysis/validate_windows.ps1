# Intel I219-LM Windows Validation and Testing Script
# PowerShell script for Windows-specific hardware validation

param(
    [string]$Interface = "",
    [switch]$Verbose = $false,
    [switch]$TestOnly = $false,
    [string]$OutputPath = "i219_windows_validation"
)

Write-Host "Intel I219-LM Windows Validation Script" -ForegroundColor Yellow
Write-Host "========================================" -ForegroundColor Yellow

# Function to write verbose output
function Write-Verbose-Custom {
    param([string]$Message)
    if ($Verbose) {
        Write-Host "[VERBOSE] $Message" -ForegroundColor Gray
    }
}

# Function to detect I219 interfaces
function Get-I219Interfaces {
    Write-Host "`n1. Detecting I219 Network Interfaces..." -ForegroundColor Cyan
    
    try {
        $adapters = Get-NetAdapter | Where-Object {
            $_.InterfaceDescription -like "*I219*" -or 
            $_.InterfaceDescription -like "*Intel*Ethernet*"
        }
        
        if ($adapters) {
            Write-Host "Found Intel Ethernet adapters:" -ForegroundColor Green
            foreach ($adapter in $adapters) {
                Write-Host "  - $($adapter.Name): $($adapter.InterfaceDescription)" -ForegroundColor Green
                Write-Host "    Status: $($adapter.Status), Speed: $($adapter.LinkSpeed)" -ForegroundColor Gray
                
                # Check for I219 specifically
                if ($adapter.InterfaceDescription -like "*I219*") {
                    Write-Host "    *** I219-LM DETECTED ***" -ForegroundColor Yellow
                    return $adapter.Name
                }
            }
            return $adapters[0].Name  # Return first Intel adapter if no I219 specifically found
        } else {
            Write-Host "No Intel Ethernet adapters found" -ForegroundColor Red
            return $null
        }
    } catch {
        Write-Host "Error detecting network adapters: $($_.Exception.Message)" -ForegroundColor Red
        return $null
    }
}

# Function to get detailed adapter information
function Get-AdapterDetails {
    param([string]$AdapterName)
    
    Write-Host "`n2. Getting Adapter Details for '$AdapterName'..." -ForegroundColor Cyan
    
    try {
        $adapter = Get-NetAdapter -Name $AdapterName
        $adapterAdvanced = Get-NetAdapterAdvancedProperty -Name $AdapterName -ErrorAction SilentlyContinue
        $adapterHardware = Get-NetAdapterHardwareInfo -Name $AdapterName -ErrorAction SilentlyContinue
        
        Write-Host "Basic Information:" -ForegroundColor Green
        Write-Host "  Interface: $($adapter.InterfaceDescription)" -ForegroundColor White
        Write-Host "  Driver Version: $($adapter.DriverVersion)" -ForegroundColor White
        Write-Host "  Driver Date: $($adapter.DriverDate)" -ForegroundColor White
        Write-Host "  MAC Address: $($adapter.MacAddress)" -ForegroundColor White
        Write-Host "  Status: $($adapter.Status)" -ForegroundColor White
        Write-Host "  Link Speed: $($adapter.LinkSpeed)" -ForegroundColor White
        
        if ($adapterHardware) {
            Write-Host "`nHardware Information:" -ForegroundColor Green
            Write-Host "  Device ID: $($adapterHardware.DeviceID)" -ForegroundColor White
            Write-Host "  Vendor ID: $($adapterHardware.VendorID)" -ForegroundColor White
            Write-Host "  Subsystem ID: $($adapterHardware.SubsystemID)" -ForegroundColor White
        }
        
        # Check for PTP/Timestamping capabilities
        Write-Host "`nAdvanced Properties (PTP-related):" -ForegroundColor Green
        $ptpProperties = $adapterAdvanced | Where-Object {
            $_.DisplayName -like "*Time*" -or 
            $_.DisplayName -like "*PTP*" -or
            $_.DisplayName -like "*Precision*" -or
            $_.DisplayName -like "*Clock*"
        }
        
        if ($ptpProperties) {
            foreach ($prop in $ptpProperties) {
                Write-Host "  $($prop.DisplayName): $($prop.DisplayValue)" -ForegroundColor White
            }
        } else {
            Write-Host "  No PTP-specific properties found" -ForegroundColor Yellow
        }
        
        return $true
    } catch {
        Write-Host "Error getting adapter details: $($_.Exception.Message)" -ForegroundColor Red
        return $false
    }
}

# Function to check driver capabilities
function Test-DriverCapabilities {
    param([string]$AdapterName)
    
    Write-Host "`n3. Testing Driver Capabilities..." -ForegroundColor Cyan
    
    try {
        # Check if adapter supports advanced features
        $stats = Get-NetAdapterStatistics -Name $AdapterName -ErrorAction SilentlyContinue
        $offload = Get-NetAdapterChecksumOffload -Name $AdapterName -ErrorAction SilentlyContinue
        $rss = Get-NetAdapterRss -Name $AdapterName -ErrorAction SilentlyContinue
        
        Write-Host "Driver Capabilities:" -ForegroundColor Green
        
        if ($stats) {
            Write-Host "  Statistics: Available" -ForegroundColor Green
        } else {
            Write-Host "  Statistics: Not Available" -ForegroundColor Red
        }
        
        if ($offload) {
            Write-Host "  Checksum Offload: $($offload.Enabled)" -ForegroundColor White
        }
        
        if ($rss) {
            Write-Host "  RSS Support: $($rss.Enabled)" -ForegroundColor White
        }
        
        # Test basic connectivity
        Write-Host "`nTesting Basic Connectivity:" -ForegroundColor Green
        $pingResult = Test-NetConnection -ComputerName "8.8.8.8" -Port 53 -InformationLevel Quiet
        if ($pingResult) {
            Write-Host "  Internet connectivity: OK" -ForegroundColor Green
        } else {
            Write-Host "  Internet connectivity: Failed" -ForegroundColor Yellow
        }
        
        return $true
    } catch {
        Write-Host "Error testing driver capabilities: $($_.Exception.Message)" -ForegroundColor Red
        return $false
    }
}

# Function to check for gPTP build
function Test-GptpBuild {
    Write-Host "`n4. Checking for gPTP Build..." -ForegroundColor Cyan
    
    $possiblePaths = @(
        "..\..\build-intel-hal\thirdparty\gptp\RelWithDebInfo\gptp.exe",
        "..\..\build\thirdparty\gptp\Release\gptp.exe",
        "..\build-intel-hal\thirdparty\gptp\RelWithDebInfo\gptp.exe",
        "..\build\thirdparty\gptp\Release\gptp.exe"
    )
    
    foreach ($path in $possiblePaths) {
        $fullPath = Resolve-Path $path -ErrorAction SilentlyContinue
        if ($fullPath -and (Test-Path $fullPath)) {
            Write-Host "  gPTP executable found: $fullPath" -ForegroundColor Green
            
            # Try to get version info
            try {
                $versionOutput = & $fullPath --help 2>&1
                if ($LASTEXITCODE -eq 0 -or $versionOutput -like "*gPTP*" -or $versionOutput -like "*Intel*") {
                    Write-Host "  gPTP executable appears functional" -ForegroundColor Green
                    return $fullPath
                }
            } catch {
                Write-Host "  gPTP executable found but may not be functional" -ForegroundColor Yellow
                return $fullPath
            }
        }
    }
    
    Write-Host "  gPTP executable not found" -ForegroundColor Red
    Write-Host "  Expected locations:" -ForegroundColor Gray
    foreach ($path in $possiblePaths) {
        Write-Host "    - $path" -ForegroundColor Gray
    }
    
    return $null
}

# Function to run a basic gPTP test
function Test-GptpBasic {
    param([string]$GptpPath, [string]$AdapterName)
    
    if (-not $GptpPath) {
        Write-Host "`n5. Skipping gPTP test (executable not found)" -ForegroundColor Yellow
        return $false
    }
    
    Write-Host "`n5. Running Basic gPTP Test..." -ForegroundColor Cyan
    
    try {
        Write-Host "  Starting gPTP daemon for 10 seconds..." -ForegroundColor White
        
        # Start gPTP process
        $processArgs = @("-i", $AdapterName)
        $process = Start-Process -FilePath $GptpPath -ArgumentList $processArgs -PassThru -NoNewWindow -RedirectStandardOutput "gptp_test_output.txt" -RedirectStandardError "gptp_test_error.txt"
        
        # Wait 10 seconds
        Start-Sleep -Seconds 10
        
        # Stop process
        if (-not $process.HasExited) {
            $process.Kill()
            $process.WaitForExit(5000)
            Write-Host "  gPTP process stopped successfully" -ForegroundColor Green
        } else {
            Write-Host "  gPTP process exited early" -ForegroundColor Yellow
        }
        
        # Check output
        if (Test-Path "gptp_test_output.txt") {
            $output = Get-Content "gptp_test_output.txt" -Raw
            if ($output -and ($output.Length -gt 0)) {
                Write-Host "  gPTP produced output (test successful)" -ForegroundColor Green
                Write-Verbose-Custom "Output sample: $($output.Substring(0, [Math]::Min(200, $output.Length)))"
                return $true
            }
        }
        
        if (Test-Path "gptp_test_error.txt") {
            $errorOutput = Get-Content "gptp_test_error.txt" -Raw
            if ($errorOutput -and ($errorOutput.Length -gt 0)) {
                Write-Host "  gPTP produced error output:" -ForegroundColor Red
                Write-Host "    $($errorOutput.Substring(0, [Math]::Min(200, $errorOutput.Length)))" -ForegroundColor Red
                return $false
            }
        }
        
        Write-Host "  gPTP test completed (no output captured)" -ForegroundColor Yellow
        return $true
        
    } catch {
        Write-Host "  Error running gPTP test: $($_.Exception.Message)" -ForegroundColor Red
        return $false
    } finally {
        # Cleanup temp files
        Remove-Item "gptp_test_output.txt" -ErrorAction SilentlyContinue
        Remove-Item "gptp_test_error.txt" -ErrorAction SilentlyContinue
    }
}

# Function to test Python tools
function Test-PythonTools {
    Write-Host "`n6. Testing Python Analysis Tools..." -ForegroundColor Cyan
    
    try {
        # Check if Python is available
        $pythonCmd = Get-Command python -ErrorAction SilentlyContinue
        if (-not $pythonCmd) {
            $pythonCmd = Get-Command python3 -ErrorAction SilentlyContinue
        }
        
        if (-not $pythonCmd) {
            Write-Host "  Python not found in PATH" -ForegroundColor Red
            return $false
        }
        
        Write-Host "  Python found: $($pythonCmd.Source)" -ForegroundColor Green
        
        # Test validation script
        if (Test-Path "validate_setup.py") {
            Write-Host "  Running setup validation..." -ForegroundColor White
            $result = & $pythonCmd.Source "validate_setup.py"
            if ($LASTEXITCODE -eq 0) {
                Write-Host "  Python tools validation: PASSED" -ForegroundColor Green
                return $true
            } else {
                Write-Host "  Python tools validation: FAILED" -ForegroundColor Red
                return $false
            }
        } else {
            Write-Host "  validate_setup.py not found" -ForegroundColor Red
            return $false
        }
        
    } catch {
        Write-Host "  Error testing Python tools: $($_.Exception.Message)" -ForegroundColor Red
        return $false
    }
}

# Function to generate report
function Generate-Report {
    param(
        [string]$AdapterName,
        [string]$GptpPath,
        [hashtable]$TestResults,
        [string]$OutputPath
    )
    
    Write-Host "`n7. Generating Validation Report..." -ForegroundColor Cyan
    
    $report = @{
        metadata = @{
            generation_time = (Get-Date).ToString("yyyy-MM-ddTHH:mm:ssZ")
            script_version = "1.0"
            computer_name = $env:COMPUTERNAME
            username = $env:USERNAME
        }
        system_info = @{
            os_version = (Get-WmiObject Win32_OperatingSystem).Caption
            os_build = (Get-WmiObject Win32_OperatingSystem).BuildNumber
            cpu = (Get-WmiObject Win32_Processor).Name
            memory_gb = [Math]::Round((Get-WmiObject Win32_ComputerSystem).TotalPhysicalMemory / 1GB, 2)
        }
        network_adapter = @{
            name = $AdapterName
            gptp_executable = $GptpPath
        }
        test_results = $TestResults
        summary = @{
            total_tests = $TestResults.Count
            passed = ($TestResults.Values | Where-Object { $_ -eq $true }).Count
            failed = ($TestResults.Values | Where-Object { $_ -eq $false }).Count
        }
    }
    
    # Save JSON report
    $jsonPath = "$OutputPath.json"
    $report | ConvertTo-Json -Depth 4 | Out-File -FilePath $jsonPath -Encoding UTF8
    
    # Generate text report
    $textPath = "$OutputPath.txt"
    $textReport = @"
Intel I219-LM Windows Validation Report
======================================

Generated: $($report.metadata.generation_time)
Computer: $($report.metadata.computer_name)
User: $($report.metadata.username)

System Information:
  OS: $($report.system_info.os_version) (Build $($report.system_info.os_build))
  CPU: $($report.system_info.cpu)
  Memory: $($report.system_info.memory_gb) GB

Network Adapter:
  Name: $($report.network_adapter.name)
  gPTP Executable: $($report.network_adapter.gptp_executable)

Test Results:
$(foreach ($test in $TestResults.GetEnumerator()) {
    $status = if ($test.Value) { "PASS" } else { "FAIL" }
    "  [$status] $($test.Key)"
})

Summary:
  Total Tests: $($report.summary.total_tests)
  Passed: $($report.summary.passed)
  Failed: $($report.summary.failed)
  Success Rate: $([Math]::Round($report.summary.passed / $report.summary.total_tests * 100, 1))%
"@
    
    $textReport | Out-File -FilePath $textPath -Encoding UTF8
    
    Write-Host "  Reports generated:" -ForegroundColor Green
    Write-Host "    JSON: $jsonPath" -ForegroundColor White
    Write-Host "    Text: $textPath" -ForegroundColor White
}

# Main execution
try {
    $testResults = @{}
    
    # Detect I219 interface
    if ($Interface) {
        $selectedInterface = $Interface
        Write-Host "Using specified interface: $selectedInterface" -ForegroundColor Cyan
    } else {
        $selectedInterface = Get-I219Interfaces
    }
    
    if (-not $selectedInterface) {
        Write-Host "`nValidation FAILED: No suitable network interface found" -ForegroundColor Red
        exit 1
    }
    
    $testResults["Interface Detection"] = $true
    
    # Get adapter details
    $adapterDetailsResult = Get-AdapterDetails $selectedInterface
    $testResults["Adapter Details"] = $adapterDetailsResult
    
    # Test driver capabilities
    $driverCapResult = Test-DriverCapabilities $selectedInterface
    $testResults["Driver Capabilities"] = $driverCapResult
    
    # Check gPTP build
    $gptpPath = Test-GptpBuild
    $testResults["gPTP Build"] = ($gptpPath -ne $null)
    
    # Run gPTP test (if not test-only mode)
    if (-not $TestOnly) {
        $gptpTestResult = Test-GptpBasic $gptpPath $selectedInterface
        $testResults["gPTP Basic Test"] = $gptpTestResult
    } else {
        Write-Host "`n5. Skipping gPTP test (TestOnly mode)" -ForegroundColor Yellow
        $testResults["gPTP Basic Test"] = $null
    }
    
    # Test Python tools
    $pythonTestResult = Test-PythonTools
    $testResults["Python Tools"] = $pythonTestResult
    
    # Generate report
    Generate-Report $selectedInterface $gptpPath $testResults $OutputPath
    
    # Summary
    Write-Host "`n========================================" -ForegroundColor Yellow
    $passedTests = ($testResults.Values | Where-Object { $_ -eq $true }).Count
    $totalTests = ($testResults.Values | Where-Object { $_ -ne $null }).Count
    
    if ($passedTests -eq $totalTests) {
        Write-Host "Validation PASSED: All tests successful" -ForegroundColor Green
        exit 0
    } else {
        Write-Host "Validation PARTIAL: $passedTests/$totalTests tests passed" -ForegroundColor Yellow
        exit 2
    }
    
} catch {
    Write-Host "`nValidation ERROR: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}
