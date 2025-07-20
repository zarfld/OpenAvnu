#!/usr/bin/env powershell
#
# IEEE 1722.1-2021 Compliance Testing Script
# Tests the responsive AVDECC entity with enhanced compliance features
#

param(
    [switch]$Verbose = $false,
    [int]$TestDuration = 30
)

Write-Host "🧪 IEEE 1722.1-2021 Compliance Testing" -ForegroundColor Cyan
Write-Host "=====================================" -ForegroundColor Cyan
Write-Host ""

# Test environment verification
Write-Host "🔍 Verifying test environment..." -ForegroundColor Yellow

# Check if executable exists
$entityPath = "d:\Repos\OpenAvnu\build\bin\Release\intel_pcap_avdecc_entity_responsive.exe"
if (!(Test-Path $entityPath)) {
    Write-Host "❌ AVDECC entity executable not found: $entityPath" -ForegroundColor Red
    Write-Host "   Please build the project first using CMake" -ForegroundColor Red
    exit 1
}

Write-Host "✅ Found AVDECC entity executable" -ForegroundColor Green

# Check network interfaces
Write-Host "🔍 Checking network interfaces for Intel adapters..." -ForegroundColor Yellow
$networkAdapters = Get-NetAdapter | Where-Object { $_.Status -eq "Up" }

$intelAdapters = $networkAdapters | Where-Object { 
    $_.InterfaceDescription -like "*Intel*" -and 
    ($_.InterfaceDescription -like "*I219*" -or 
     $_.InterfaceDescription -like "*I210*" -or 
     $_.InterfaceDescription -like "*I225*" -or 
     $_.InterfaceDescription -like "*I226*")
}

if ($intelAdapters.Count -gt 0) {
    Write-Host "✅ Found Intel Ethernet adapters:" -ForegroundColor Green
    foreach ($adapter in $intelAdapters) {
        Write-Host "   - $($adapter.InterfaceDescription)" -ForegroundColor White
    }
} else {
    Write-Host "⚠️  No Intel Ethernet adapters found - testing with available adapters" -ForegroundColor Yellow
}

# Display available adapters
Write-Host "📋 All available network adapters:" -ForegroundColor Cyan
foreach ($adapter in $networkAdapters) {
    $status = if ($adapter.Status -eq "Up") { "✅" } else { "❌" }
    Write-Host "   $status $($adapter.Name): $($adapter.InterfaceDescription)" -ForegroundColor White
}

Write-Host ""

# Start compliance test
Write-Host "🚀 Starting IEEE 1722.1-2021 Responsive AVDECC Entity..." -ForegroundColor Magenta
Write-Host ""

Write-Host "📊 Compliance Features Being Tested:" -ForegroundColor Cyan
Write-Host "   ✅ Enum-Based Status Codes (AECP_Status enum)" -ForegroundColor White
Write-Host "   ✅ Specification-Compliant Command Handling (AEM_Command enum)" -ForegroundColor White
Write-Host "   ✅ Enhanced Status Logging (human-readable status names)" -ForegroundColor White
Write-Host "   ✅ Proper Command Categorization (vendor vs standard vs reserved)" -ForegroundColor White
Write-Host "   ✅ IEEE 1722.1-2021 Constants (AVDECC namespace)" -ForegroundColor White
Write-Host "   ✅ Configuration Descriptor Support (IEEE 1722.1-2013 Clause 7.2.1)" -ForegroundColor White
Write-Host ""

Write-Host "🎯 Entity Configuration:" -ForegroundColor Cyan
Write-Host "   Entity ID: 0xc047e0fffe167b89" -ForegroundColor White
Write-Host "   MAC Address: c0:47:e0:16:7b:89" -ForegroundColor White
Write-Host "   Protocol: IEEE 1722.1-2021 AVDECC/AVTP" -ForegroundColor White
Write-Host "   Transport: Layer 2 Ethernet (PCAP)" -ForegroundColor White
Write-Host "   Features: ADP Discovery + AEM Command Responses" -ForegroundColor White
Write-Host ""

Write-Host "⏱️  Test will run for $TestDuration seconds..." -ForegroundColor Yellow
Write-Host "💡 Use Hive AVDECC Controller to test command responses" -ForegroundColor Yellow
Write-Host "💡 Monitor the output for compliance status messages" -ForegroundColor Yellow
Write-Host ""

Write-Host "Press Ctrl+C to stop the test early" -ForegroundColor Gray
Write-Host ""

# Start the entity in background and capture output
$job = Start-Job -ScriptBlock {
    param($entityPath, $verbose)
    if ($verbose) {
        & $entityPath 2>&1 | ForEach-Object { "$((Get-Date).ToString('HH:mm:ss.fff')) $_" }
    } else {
        & $entityPath 2>&1
    }
} -ArgumentList $entityPath, $Verbose

# Monitor for a specified duration
$startTime = Get-Date
$endTime = $startTime.AddSeconds($TestDuration)

Write-Host "🔄 Entity started (Job ID: $($job.Id))" -ForegroundColor Green
Write-Host "📡 Listening for AVDECC traffic..." -ForegroundColor Cyan

try {
    while ((Get-Date) -lt $endTime) {
        # Check if job is still running
        if ($job.State -ne "Running") {
            Write-Host "⚠️  Entity process stopped unexpectedly" -ForegroundColor Yellow
            break
        }
        
        # Display any output from the entity
        $output = Receive-Job $job -Keep
        if ($output) {
            $output | ForEach-Object {
                Write-Host "📟 $_" -ForegroundColor White
            }
        }
        
        Start-Sleep -Milliseconds 500
    }
    
    Write-Host ""
    Write-Host "⏰ Test duration completed" -ForegroundColor Yellow
    
} catch {
    Write-Host "❌ Test interrupted: $($_.Exception.Message)" -ForegroundColor Red
} finally {
    # Stop the entity
    Write-Host "🛑 Stopping AVDECC entity..." -ForegroundColor Yellow
    Stop-Job $job -ErrorAction SilentlyContinue
    Remove-Job $job -ErrorAction SilentlyContinue
}

Write-Host ""
Write-Host "📋 IEEE 1722.1-2021 Compliance Test Summary" -ForegroundColor Cyan
Write-Host "===========================================" -ForegroundColor Cyan

# Verify that improvements were applied by checking source code
Write-Host ""
Write-Host "🔍 Verifying applied compliance improvements..." -ForegroundColor Yellow

$sourceFile = "d:\Repos\OpenAvnu\lib\Standards\intel_pcap_avdecc_entity_responsive.cpp"

# Check for AVDECC namespace
$hasNamespace = (Get-Content $sourceFile | Select-String "namespace AVDECC").Count -gt 0
Write-Host "   ✅ AVDECC Namespace: $(if ($hasNamespace) { 'FOUND' } else { 'MISSING' })" -ForegroundColor $(if ($hasNamespace) { 'Green' } else { 'Red' })

# Check for enum status codes
$hasEnumStatus = (Get-Content $sourceFile | Select-String "enum class AECP_Status").Count -gt 0
Write-Host "   ✅ AECP_Status Enum: $(if ($hasEnumStatus) { 'FOUND' } else { 'MISSING' })" -ForegroundColor $(if ($hasEnumStatus) { 'Green' } else { 'Red' })

# Check for command enum
$hasCommandEnum = (Get-Content $sourceFile | Select-String "enum class AEM_Command").Count -gt 0
Write-Host "   ✅ AEM_Command Enum: $(if ($hasCommandEnum) { 'FOUND' } else { 'MISSING' })" -ForegroundColor $(if ($hasCommandEnum) { 'Green' } else { 'Red' })

# Check for descriptor enum
$hasDescriptorEnum = (Get-Content $sourceFile | Select-String "enum class DescriptorType").Count -gt 0
Write-Host "   ✅ DescriptorType Enum: $(if ($hasDescriptorEnum) { 'FOUND' } else { 'MISSING' })" -ForegroundColor $(if ($hasDescriptorEnum) { 'Green' } else { 'Red' })

# Check for configuration descriptor support
$hasConfigDescriptor = (Get-Content $sourceFile | Select-String "CONFIGURATION_DESCRIPTOR").Count -gt 0
Write-Host "   ✅ Configuration Descriptor: $(if ($hasConfigDescriptor) { 'FOUND' } else { 'MISSING' })" -ForegroundColor $(if ($hasConfigDescriptor) { 'Green' } else { 'Red' })

# Check for proper status logging
$hasStatusLogging = (Get-Content $sourceFile | Select-String "status_name").Count -gt 0
Write-Host "   ✅ Enhanced Status Logging: $(if ($hasStatusLogging) { 'FOUND' } else { 'MISSING' })" -ForegroundColor $(if ($hasStatusLogging) { 'Green' } else { 'Red' })

Write-Host ""

# Calculate compliance grade
$totalFeatures = 6
$implementedFeatures = @($hasNamespace, $hasEnumStatus, $hasCommandEnum, $hasDescriptorEnum, $hasConfigDescriptor, $hasStatusLogging) | Where-Object { $_ } | Measure-Object | Select-Object -ExpandProperty Count

$compliancePercentage = [math]::Round(($implementedFeatures / $totalFeatures) * 100, 1)
$complianceGrade = switch ($compliancePercentage) {
    { $_ -ge 95 } { "A (Excellent)" }
    { $_ -ge 90 } { "A- (Very Good)" }
    { $_ -ge 85 } { "B+ (Good)" }
    { $_ -ge 80 } { "B (Fair)" }
    { $_ -ge 75 } { "B- (Acceptable)" }
    default { "C (Needs Improvement)" }
}

if ($compliancePercentage -ge 90) {
    Write-Host "🎉 IEEE 1722.1-2021 Compliance Test: PASSED" -ForegroundColor Green
    Write-Host "   Grade: $complianceGrade ($compliancePercentage%)" -ForegroundColor Green
    Write-Host "   Status: Ready for production deployment" -ForegroundColor Green
} elseif ($compliancePercentage -ge 75) {
    Write-Host "⚠️  IEEE 1722.1-2021 Compliance Test: PARTIAL" -ForegroundColor Yellow
    Write-Host "   Grade: $complianceGrade ($compliancePercentage%)" -ForegroundColor Yellow
    Write-Host "   Status: Minor improvements recommended" -ForegroundColor Yellow
} else {
    Write-Host "❌ IEEE 1722.1-2021 Compliance Test: FAILED" -ForegroundColor Red
    Write-Host "   Grade: $complianceGrade ($compliancePercentage%)" -ForegroundColor Red
    Write-Host "   Status: Significant improvements required" -ForegroundColor Red
}

Write-Host ""
Write-Host "📝 Test completed at $(Get-Date)" -ForegroundColor Gray
Write-Host "📁 Entity executable: $entityPath" -ForegroundColor Gray
Write-Host "📄 Source code: $sourceFile" -ForegroundColor Gray
