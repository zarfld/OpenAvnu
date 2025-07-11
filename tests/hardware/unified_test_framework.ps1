# OpenAvnu Unified Hardware Testing Framework
# Consolidates all hardware testing approaches into a single, comprehensive suite
# Supports: I210, I219, I225, I226 adapters across different machines
# Date: 2025-01-11

param(
    [string]$Phase = "1",           # Test phase to run (1, 2, 3, 4, 5, or "all")
    [switch]$SkipConnectivity,      # Skip network connectivity tests
    [string]$TestTargetIP = "",     # Target IP for connectivity tests
    [switch]$SaveResults,           # Save detailed results to docs/tests/results/
    [switch]$Verbose,               # Verbose output
    [string]$AdapterFilter = "",    # Filter to specific adapter (e.g., "I225", "I219")
    [switch]$ListAdapters           # Just list available adapters and exit
)

# Global configuration
$script:TestResults = @{
    "StartTime" = Get-Date
    "System" = $env:COMPUTERNAME
    "Phases" = @{}
    "Summary" = @{}
}

function Write-TestLog {
    param(
        [string]$Message,
        [string]$Level = "INFO",  # INFO, WARN, ERROR, SUCCESS
        [switch]$NoNewline
    )
    
    $color = switch ($Level) {
        "SUCCESS" { "Green" }
        "WARN" { "Yellow" }
        "ERROR" { "Red" }
        default { "White" }
    }
    
    $timestamp = Get-Date -Format "HH:mm:ss"
    $logMessage = "[$timestamp] $Message"
    
    if ($NoNewline) {
        Write-Host $logMessage -ForegroundColor $color -NoNewline
    } else {
        Write-Host $logMessage -ForegroundColor $color
    }
    
    # Store for later saving
    if (-not $script:TestResults.ContainsKey("Log")) {
        $script:TestResults["Log"] = @()
    }
    $script:TestResults["Log"] += @{
        "Timestamp" = Get-Date
        "Level" = $Level
        "Message" = $Message
    }
}

function Get-IntelAdapters {
    Write-TestLog "Detecting Intel network adapters..." -Level "INFO"
    
    $adapters = Get-PnpDevice | Where-Object { 
        $_.FriendlyName -like "*Intel*" -and 
        $_.Class -eq "Net" -and
        $_.Status -eq "OK"
    }
    
    $result = @()
    
    foreach ($adapter in $adapters) {
        $netAdapter = Get-NetAdapter | Where-Object { $_.InterfaceDescription -eq $adapter.FriendlyName }
        
        if ($adapter.InstanceId -match "DEV_([0-9A-F]{4})") {
            $deviceId = $matches[1]
            
            $adapterInfo = @{
                "Name" = $adapter.FriendlyName
                "DeviceId" = $deviceId
                "InstanceId" = $adapter.InstanceId
                "Status" = $adapter.Status
                "NetAdapterName" = if ($netAdapter) { $netAdapter.Name } else { "N/A" }
                "MacAddress" = if ($netAdapter) { $netAdapter.MacAddress } else { "N/A" }
                "LinkStatus" = if ($netAdapter) { $netAdapter.Status } else { "N/A" }
                "LinkSpeed" = if ($netAdapter) { $netAdapter.LinkSpeed } else { "N/A" }
                "Model" = Get-AdapterModel -DeviceId $deviceId
                "PtpSupported" = Test-PtpSupport -DeviceId $deviceId
                "TsnSupported" = Test-TsnSupport -DeviceId $deviceId
                "HalSupported" = Test-HalSupport -DeviceId $deviceId
            }
            
            # Apply filter if specified
            if (-not $AdapterFilter -or $adapterInfo.Model -like "*$AdapterFilter*") {
                $result += $adapterInfo
            }
        }
    }
    
    return $result
}

function Get-AdapterModel {
    param([string]$DeviceId)
    
    $models = @{
        "1533" = "I210"
        "1539" = "I211"
        "15F3" = "I219"
        "0D4E" = "I219-LM"
        "0DC7" = "I219-LM (22)"
        "15F5" = "I219-V"
        "550A" = "I225"
        "550B" = "I225-LM"
        "550C" = "I225-V"
        "15F2" = "I225-LM"
        "0D9F" = "I226-LM"
        "125C" = "I226-V"
        "125D" = "I226-IT"
        "125E" = "I226-K"
    }
    
    if ($models.ContainsKey($DeviceId)) {
        return $models[$DeviceId]
    } else {
        return "Unknown ($DeviceId)"
    }
}

function Test-PtpSupport {
    param([string]$DeviceId)
    return $DeviceId -in @("1533", "1539", "15F3", "0D4E", "0DC7", "15F5", "550A", "550B", "550C", "15F2", "0D9F", "125C", "125D", "125E")
}

function Test-TsnSupport {
    param([string]$DeviceId)
    return $DeviceId -in @("550A", "550B", "550C", "15F2", "0D9F", "125C", "125D", "125E")
}

function Test-HalSupport {
    param([string]$DeviceId)
    return Test-PtpSupport -DeviceId $DeviceId
}

function Invoke-Phase1Testing {
    param([array]$Adapters)
    
    Write-TestLog "=== Phase 1: Hardware Detection and Basic Interface ===" -Level "SUCCESS"
    
    $phase1Results = @{
        "HardwareDetection" = $false
        "NetworkInterface" = $false
        "DriverInterface" = $false
        "Details" = @{}
    }
    
    # Test 1.1: Hardware Detection
    Write-TestLog "Test 1.1: Hardware Detection and Capabilities"
    if ($Adapters.Count -gt 0) {
        $ptpAdapters = @($Adapters | Where-Object { $_.PtpSupported })
        $tsnAdapters = @($Adapters | Where-Object { $_.TsnSupported })
        
        Write-TestLog "  Found $($Adapters.Count) Intel adapter(s)" -Level "SUCCESS"
        Write-TestLog "  PTP capable: $($ptpAdapters.Count)" -Level "INFO"
        Write-TestLog "  TSN capable: $($tsnAdapters.Count)" -Level "INFO"
        
        foreach ($adapter in $Adapters) {
            Write-TestLog "  $($adapter.Model): PTP=$($adapter.PtpSupported), TSN=$($adapter.TsnSupported)" -Level "INFO"
        }
        
        $phase1Results.HardwareDetection = $true
        $phase1Results.Details.AdapterCount = $Adapters.Count
        $phase1Results.Details.PtpAdapterCount = $ptpAdapters.Count
        $phase1Results.Details.TsnAdapterCount = $tsnAdapters.Count
    } else {
        Write-TestLog "  No Intel adapters found" -Level "ERROR"
        return $phase1Results
    }
    
    # Test 1.2: Network Interface Validation
    Write-TestLog "Test 1.2: Network Interface Validation"
    $interfacesPassed = 0
    
    foreach ($adapter in $Adapters) {
        if ($adapter.NetAdapterName -ne "N/A") {
            try {
                $netAdapter = Get-NetAdapter -Name $adapter.NetAdapterName -ErrorAction Stop
                $stats = Get-NetAdapterStatistics -Name $adapter.NetAdapterName -ErrorAction Stop
                
                Write-TestLog "  $($adapter.NetAdapterName): Interface accessible" -Level "SUCCESS"
                $interfacesPassed++
            } catch {
                Write-TestLog "  $($adapter.NetAdapterName): Interface error - $($_.Exception.Message)" -Level "ERROR"
            }
        }
    }
    
    $phase1Results.NetworkInterface = ($interfacesPassed -eq $Adapters.Count)
    $phase1Results.Details.InterfacesPassed = $interfacesPassed
    
    # Test 1.3: Driver Interface and PTP Properties
    Write-TestLog "Test 1.3: Driver Interface and PTP Properties"
    $driversPassed = 0
    
    foreach ($adapter in $Adapters | Where-Object { $_.PtpSupported -and $_.NetAdapterName -ne "N/A" }) {
        try {
            $ptpProps = Get-NetAdapterAdvancedProperty -Name $adapter.NetAdapterName | Where-Object { 
                $_.DisplayName -like "*PTP*" -or $_.DisplayName -like "*Timestamp*"
            }
            
            if ($ptpProps) {
                Write-TestLog "  $($adapter.NetAdapterName): PTP properties accessible" -Level "SUCCESS"
                $driversPassed++
            } else {
                Write-TestLog "  $($adapter.NetAdapterName): No PTP properties found" -Level "WARN"
            }
        } catch {
            Write-TestLog "  $($adapter.NetAdapterName): Driver error - $($_.Exception.Message)" -Level "ERROR"
        }
    }
    
    $ptpAdapterCount = @($Adapters | Where-Object { $_.PtpSupported -and $_.NetAdapterName -ne "N/A" }).Count
    $phase1Results.DriverInterface = ($driversPassed -eq $ptpAdapterCount)
    $phase1Results.Details.DriversPassed = $driversPassed
    
    $script:TestResults.Phases["Phase1"] = $phase1Results
    
    $success = $phase1Results.HardwareDetection -and $phase1Results.NetworkInterface -and $phase1Results.DriverInterface
    Write-TestLog "Phase 1 Complete: $(if ($success) { "✅ PASS" } else { "❌ FAIL" })" -Level $(if ($success) { "SUCCESS" } else { "ERROR" })
    
    return $phase1Results
}

function Invoke-Phase2Testing {
    param([array]$Adapters)
    
    Write-TestLog "=== Phase 2: Register Access and Hardware Interface ===" -Level "SUCCESS"
    
    $phase2Results = @{
        "RegisterAccess" = $false
        "HardwareCapabilities" = $false
        "HalIntegration" = $false
        "Details" = @{}
    }
    
    $ptpAdapters = @($Adapters | Where-Object { $_.PtpSupported -and $_.NetAdapterName -ne "N/A" })
    
    if ($ptpAdapters.Count -eq 0) {
        Write-TestLog "No PTP-capable adapters available for Phase 2 testing" -Level "ERROR"
        return $phase2Results
    }
    
    # Test 2.1: Register Access (via advanced properties)
    Write-TestLog "Test 2.1: Register Access Validation"
    $registerAccessCount = 0
    
    foreach ($adapter in $ptpAdapters) {
        try {
            $advancedProps = Get-NetAdapterAdvancedProperty -Name $adapter.NetAdapterName
            $hardwareProps = $advancedProps | Where-Object { 
                $_.DisplayName -like "*Hardware*" -or 
                $_.DisplayName -like "*Timestamp*" -or
                $_.DisplayName -like "*Buffer*" -or
                $_.DisplayName -like "*Queue*"
            }
            
            if ($hardwareProps -and $hardwareProps.Count -gt 0) {
                Write-TestLog "  $($adapter.NetAdapterName): Hardware properties accessible ($($hardwareProps.Count) properties)" -Level "SUCCESS"
                $registerAccessCount++
                
                if ($Verbose) {
                    foreach ($prop in $hardwareProps | Select-Object -First 3) {
                        Write-TestLog "    $($prop.DisplayName): $($prop.DisplayValue)" -Level "INFO"
                    }
                }
            } else {
                Write-TestLog "  $($adapter.NetAdapterName): Limited hardware properties" -Level "WARN"
            }
        } catch {
            Write-TestLog "  $($adapter.NetAdapterName): Register access error - $($_.Exception.Message)" -Level "ERROR"
        }
    }
    
    $phase2Results.RegisterAccess = ($registerAccessCount -gt 0)
    $phase2Results.Details.RegisterAccessCount = $registerAccessCount
    
    # Test 2.2: Hardware Capabilities Detection
    Write-TestLog "Test 2.2: Hardware Capabilities Detection"
    $capabilitiesDetected = 0
    
    foreach ($adapter in $ptpAdapters) {
        $capabilities = @{
            "PtpHardwareTimestamp" = $false
            "TsnSupport" = $adapter.TsnSupported
            "JumboFrames" = $false
            "FlowControl" = $false
        }
        
        try {
            $allProps = Get-NetAdapterAdvancedProperty -Name $adapter.NetAdapterName
            
            # Check PTP timestamp capability
            $ptpProp = $allProps | Where-Object { $_.RegistryKeyword -eq "*PtpHardwareTimestamp" }
            if ($ptpProp) {
                $capabilities.PtpHardwareTimestamp = ($ptpProp.DisplayValue -eq "Enabled")
            }
            
            # Check jumbo frame support
            $jumboProp = $allProps | Where-Object { $_.DisplayName -like "*Jumbo*" }
            if ($jumboProp) {
                $capabilities.JumboFrames = $true
            }
            
            # Check flow control
            $flowProp = $allProps | Where-Object { $_.DisplayName -like "*Flow Control*" }
            if ($flowProp) {
                $capabilities.FlowControl = $true
            }
            
            $enabledCapabilities = @($capabilities.GetEnumerator() | Where-Object { $_.Value -eq $true }).Count
            Write-TestLog "  $($adapter.NetAdapterName): $enabledCapabilities/4 capabilities detected" -Level "SUCCESS"
            $capabilitiesDetected++
            
            if ($Verbose) {
                foreach ($cap in $capabilities.GetEnumerator()) {
                    $status = if ($cap.Value) { "✅" } else { "❌" }
                    Write-TestLog "    $($cap.Key): $status" -Level "INFO"
                }
            }
        } catch {
            Write-TestLog "  $($adapter.NetAdapterName): Capability detection error - $($_.Exception.Message)" -Level "ERROR"
        }
    }
    
    $phase2Results.HardwareCapabilities = ($capabilitiesDetected -gt 0)
    $phase2Results.Details.CapabilitiesDetected = $capabilitiesDetected
    
    # Test 2.3: Intel HAL Integration Prerequisites
    Write-TestLog "Test 2.3: Intel HAL Integration Prerequisites"
    $halReadyCount = 0
    
    foreach ($adapter in $ptpAdapters) {
        $halPrereqs = @{
            "SupportedDeviceId" = Test-HalSupport -DeviceId $adapter.DeviceId
            "IntelDriver" = $false
            "PtpEnabled" = $false
            "InterfaceAccess" = $false
        }
        
        try {
            # Check driver provider
            $deviceInfo = Get-PnpDevice | Where-Object { $_.InstanceId -eq $adapter.InstanceId }
            $driverProvider = Get-PnpDeviceProperty -InstanceId $adapter.InstanceId -KeyName "DEVPKEY_Device_DriverProvider" -ErrorAction SilentlyContinue
            if ($driverProvider -and $driverProvider.Data -like "*Intel*") {
                $halPrereqs.IntelDriver = $true
            }
            
            # Check PTP enabled
            $ptpProp = Get-NetAdapterAdvancedProperty -Name $adapter.NetAdapterName | Where-Object { $_.RegistryKeyword -eq "*PtpHardwareTimestamp" }
            if ($ptpProp -and $ptpProp.DisplayValue -eq "Enabled") {
                $halPrereqs.PtpEnabled = $true
            }
            
            # Check interface access
            $interfaceInfo = Get-NetIPInterface -InterfaceIndex (Get-NetAdapter -Name $adapter.NetAdapterName).InterfaceIndex -ErrorAction SilentlyContinue
            if ($interfaceInfo) {
                $halPrereqs.InterfaceAccess = $true
            }
            
            $metPrereqs = @($halPrereqs.GetEnumerator() | Where-Object { $_.Value -eq $true }).Count
            Write-TestLog "  $($adapter.NetAdapterName): $metPrereqs/4 HAL prerequisites met" -Level $(if ($metPrereqs -ge 3) { "SUCCESS" } else { "WARN" })
            
            if ($metPrereqs -ge 3) {
                $halReadyCount++
            }
            
            if ($Verbose) {
                foreach ($prereq in $halPrereqs.GetEnumerator()) {
                    $status = if ($prereq.Value) { "✅" } else { "❌" }
                    Write-TestLog "    $($prereq.Key): $status" -Level "INFO"
                }
            }
        } catch {
            Write-TestLog "  $($adapter.NetAdapterName): HAL prerequisite check error - $($_.Exception.Message)" -Level "ERROR"
        }
    }
    
    $phase2Results.HalIntegration = ($halReadyCount -gt 0)
    $phase2Results.Details.HalReadyCount = $halReadyCount
    
    $script:TestResults.Phases["Phase2"] = $phase2Results
    
    $success = $phase2Results.RegisterAccess -and $phase2Results.HardwareCapabilities -and $phase2Results.HalIntegration
    Write-TestLog "Phase 2 Complete: $(if ($success) { "✅ PASS" } else { "❌ FAIL" })" -Level $(if ($success) { "SUCCESS" } else { "ERROR" })
    
    return $phase2Results
}

function Save-TestResults {
    if (-not $SaveResults) {
        return
    }
    
    $script:TestResults.EndTime = Get-Date
    $script:TestResults.Duration = ($script:TestResults.EndTime - $script:TestResults.StartTime).TotalMinutes
    
    # Create results directory
    $resultsDir = "d:\Repos\OpenAvnu\docs\tests\results"
    if (-not (Test-Path $resultsDir)) {
        New-Item -ItemType Directory -Path $resultsDir -Force | Out-Null
    }
    
    # Save JSON results
    $jsonPath = "$resultsDir\unified_test_results_$(Get-Date -Format 'yyyyMMdd_HHmmss').json"
    $script:TestResults | ConvertTo-Json -Depth 10 | Out-File -FilePath $jsonPath -Encoding UTF8
    
    Write-TestLog "Test results saved to: $jsonPath" -Level "SUCCESS"
}

# Main execution
Write-TestLog "=== OpenAvnu Unified Hardware Testing Framework ===" -Level "SUCCESS"
Write-TestLog "System: $env:COMPUTERNAME | Date: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')" -Level "INFO"

# Get Intel adapters
$adapters = Get-IntelAdapters

if ($ListAdapters) {
    Write-TestLog "Available Intel Network Adapters:" -Level "SUCCESS"
    if ($adapters.Count -eq 0) {
        Write-TestLog "No Intel network adapters found" -Level "WARN"
    } else {
        foreach ($adapter in $adapters) {
            Write-TestLog "  $($adapter.Model) ($($adapter.DeviceId)): PTP=$($adapter.PtpSupported), TSN=$($adapter.TsnSupported)" -Level "INFO"
        }
    }
    exit 0
}

if ($adapters.Count -eq 0) {
    Write-TestLog "No Intel network adapters found. Cannot proceed with testing." -Level "ERROR"
    exit 1
}

# Execute test phases
$overallSuccess = $true

if ($Phase -eq "1" -or $Phase -eq "all") {
    $phase1Result = Invoke-Phase1Testing -Adapters $adapters
    if (-not ($phase1Result.HardwareDetection -and $phase1Result.NetworkInterface -and $phase1Result.DriverInterface)) {
        $overallSuccess = $false
        if ($Phase -ne "all") {
            Write-TestLog "Phase 1 failed. Cannot proceed to subsequent phases." -Level "ERROR"
            Save-TestResults
            exit 1
        }
    }
}

if ($Phase -eq "2" -or $Phase -eq "all") {
    $phase2Result = Invoke-Phase2Testing -Adapters $adapters
    if (-not ($phase2Result.RegisterAccess -and $phase2Result.HardwareCapabilities -and $phase2Result.HalIntegration)) {
        $overallSuccess = $false
    }
}

# Future phases (3, 4, 5) would be implemented here

# Final summary
Write-TestLog ""
Write-TestLog "=== Test Suite Summary ===" -Level "SUCCESS"
$completedPhases = $script:TestResults.Phases.Count
Write-TestLog "Completed Phases: $completedPhases" -Level "INFO"

foreach ($phase in $script:TestResults.Phases.GetEnumerator()) {
    $phaseSuccess = switch ($phase.Key) {
        "Phase1" { $phase.Value.HardwareDetection -and $phase.Value.NetworkInterface -and $phase.Value.DriverInterface }
        "Phase2" { $phase.Value.RegisterAccess -and $phase.Value.HardwareCapabilities -and $phase.Value.HalIntegration }
        default { $false }
    }
    
    Write-TestLog "$($phase.Key): $(if ($phaseSuccess) { "✅ PASS" } else { "❌ FAIL" })" -Level $(if ($phaseSuccess) { "SUCCESS" } else { "ERROR" })
}

Write-TestLog "Overall Status: $(if ($overallSuccess) { "✅ SUCCESS" } else { "❌ PARTIAL/FAILURE" })" -Level $(if ($overallSuccess) { "SUCCESS" } else { "ERROR" })

Save-TestResults

exit $(if ($overallSuccess) { 0 } else { 1 })
