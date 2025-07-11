#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Comprehensive cleanup script to organize all scattered test files, logs, configs, and legacy scripts
    
.DESCRIPTION
    This script consolidates all test-related files scattered throughout the OpenAvnu repository
    into the organized directory structure under docs/tests/
    
.PARAMETER DryRun
    Show what would be moved without actually moving files
    
.PARAMETER KeepOriginals
    Keep original files in place (copy instead of move)
    
.EXAMPLE
    .\comprehensive_cleanup.ps1 -DryRun
    .\comprehensive_cleanup.ps1 -KeepOriginals
#>

param(
    [switch]$DryRun,
    [switch]$KeepOriginals
)

$ErrorActionPreference = "Continue"

# Define target directories
$baseTestDir = "docs/tests"
$resultsDir = "$baseTestDir/results"
$configDir = "$baseTestDir/config"
$tempDir = "$baseTestDir/temp"
$legacyDir = "$baseTestDir/legacy"

# Ensure target directories exist
$targetDirs = @($resultsDir, $configDir, $tempDir, "$legacyDir/scripts", "$legacyDir/outputs")
foreach ($dir in $targetDirs) {
    if (-not $DryRun) {
        New-Item -ItemType Directory -Path $dir -Force | Out-Null
    }
    Write-Host "Target directory: $dir" -ForegroundColor Green
}

Write-Host "`n=== COMPREHENSIVE REPOSITORY CLEANUP ===" -ForegroundColor Cyan
Write-Host "Mode: $(if ($DryRun) { 'DRY RUN' } else { 'LIVE CLEANUP' })" -ForegroundColor Yellow
Write-Host "Keep Originals: $KeepOriginals" -ForegroundColor Yellow

$moveCount = 0
$copyCount = 0
$skipCount = 0

function Move-FileToTarget {
    param(
        [string]$SourcePath,
        [string]$TargetDir,
        [string]$Category
    )
    
    $fileName = Split-Path $SourcePath -Leaf
    $targetPath = Join-Path $TargetDir $fileName
    
    # Handle name conflicts
    $counter = 1
    $baseName = [System.IO.Path]::GetFileNameWithoutExtension($fileName)
    $extension = [System.IO.Path]::GetExtension($fileName)
    
    while (Test-Path $targetPath) {
        $newName = "${baseName}_${counter}${extension}"
        $targetPath = Join-Path $TargetDir $newName
        $counter++
    }
    
    $finalFileName = Split-Path $targetPath -Leaf
    
    Write-Host "  [$Category] $fileName -> $TargetDir/$finalFileName" -ForegroundColor White
    
    if (-not $DryRun) {
        try {
            if ($KeepOriginals) {
                Copy-Item $SourcePath $targetPath -Force
                $script:copyCount++
                Write-Host "    Copied successfully" -ForegroundColor Green
            } else {
                Move-Item $SourcePath $targetPath -Force
                $script:moveCount++
                Write-Host "    Moved successfully" -ForegroundColor Green
            }
        } catch {
            Write-Host "    ERROR: $($_.Exception.Message)" -ForegroundColor Red
            $script:skipCount++
        }
    }
}

# 1. Clean up build directory logs
Write-Host "`n1. BUILD DIRECTORY LOGS" -ForegroundColor Cyan
$buildLogs = Get-ChildItem -Path "build" -Recurse -File -ErrorAction SilentlyContinue | Where-Object { 
    $_.Extension -in @('.log', '.txt') -and $_.Name -match "(gptp|test|output)" 
}

foreach ($log in $buildLogs) {
    Move-FileToTarget -SourcePath $log.FullName -TargetDir "$resultsDir/build_logs" -Category "BUILD"
}

if (-not $buildLogs) {
    Write-Host "  No build logs found" -ForegroundColor Gray
}

# 2. Root directory output files
Write-Host "`n2. ROOT DIRECTORY OUTPUT FILES" -ForegroundColor Cyan
$rootOutputs = Get-ChildItem -Path "." -File | Where-Object { 
    ($_.Name -match "(output|result|log|test_)" -or $_.Name -match "PROFILE_FRAMEWORK") -and 
    $_.Extension -in @('.log', '.txt', '.json', '.csv', '.xml', '.out') 
}

foreach ($output in $rootOutputs) {
    Move-FileToTarget -SourcePath $output.FullName -TargetDir "$resultsDir/scattered" -Category "OUTPUT"
}

if (-not $rootOutputs) {
    Write-Host "  No root output files found" -ForegroundColor Gray
}

# 3. Config files that should be centralized
Write-Host "`n3. CONFIGURATION FILES" -ForegroundColor Cyan
$configFiles = @(
    "tools/timestamp_analysis/config_i210.json",
    "tools/timestamp_analysis/config_i225.json",
    "tools/timestamp_analysis/config.json"
)

foreach ($configPath in $configFiles) {
    if (Test-Path $configPath) {
        $targetConfigDir = "$configDir/timestamp_analysis"
        if (-not $DryRun) {
            New-Item -ItemType Directory -Path $targetConfigDir -Force | Out-Null
        }
        
        # Always copy config files (don't move them, tools need them)
        $originalKeepOriginals = $KeepOriginals
        $KeepOriginals = $true
        Move-FileToTarget -SourcePath $configPath -TargetDir $targetConfigDir -Category "CONFIG"
        $KeepOriginals = $originalKeepOriginals
    }
}

# 4. Legacy test scripts
Write-Host "`n4. LEGACY TEST SCRIPTS" -ForegroundColor Cyan
$legacyScripts = Get-ChildItem -Path "*.ps1" | Where-Object { 
    $_.Name -match "(test_|build_and_test|phase\d|i2\d\d|configure_|simple_)" -and
    $_.Name -notin @("unified_test_framework.ps1")  # Don't move the new unified framework
}

foreach ($script in $legacyScripts) {
    Move-FileToTarget -SourcePath $script.FullName -TargetDir "$legacyDir/scripts" -Category "SCRIPT"
}

if (-not $legacyScripts) {
    Write-Host "  No legacy scripts found" -ForegroundColor Gray
}

# 5. Verification scripts that are no longer needed
Write-Host "`n5. VERIFICATION SCRIPTS" -ForegroundColor Cyan
$verificationScripts = Get-ChildItem -Path "*.ps1" | Where-Object { 
    $_.Name -match "(verify_|fix_|all_phases_status)" 
}

foreach ($script in $verificationScripts) {
    Move-FileToTarget -SourcePath $script.FullName -TargetDir "$legacyDir/scripts" -Category "VERIFY"
}

if (-not $verificationScripts) {
    Write-Host "  No verification scripts found" -ForegroundColor Gray
}

# 6. Check for any other scattered test files
Write-Host "`n6. OTHER SCATTERED TEST FILES" -ForegroundColor Cyan
$otherFiles = Get-ChildItem -Path "." -Recurse -File | Where-Object { 
    ($_.Name -match "(test|result|output)" -or $_.Name -match "^\d{4}-\d{2}-\d{2}") -and 
    $_.Extension -in @('.log', '.txt', '.json', '.csv', '.xml', '.out', '.data') -and 
    $_.FullName -notmatch "(docs\\tests|thirdparty|externals|\.git|src|include|node_modules)" -and
    $_.Directory.Name -ne "build"  # Already handled
}

foreach ($file in $otherFiles) {
    Move-FileToTarget -SourcePath $file.FullName -TargetDir "$resultsDir/scattered" -Category "OTHER"
}

if (-not $otherFiles) {
    Write-Host "  No other scattered files found" -ForegroundColor Gray
}

# 7. Summary
Write-Host "`n=== CLEANUP SUMMARY ===" -ForegroundColor Cyan
Write-Host "Files moved: $moveCount" -ForegroundColor Green
Write-Host "Files copied: $copyCount" -ForegroundColor Blue
Write-Host "Files skipped (errors): $skipCount" -ForegroundColor Red

if ($DryRun) {
    Write-Host "`nThis was a DRY RUN. No files were actually moved." -ForegroundColor Yellow
    Write-Host "Run without -DryRun to perform the actual cleanup." -ForegroundColor Yellow
} else {
    Write-Host "`nCleanup completed!" -ForegroundColor Green
    
    # Create a cleanup report
    $reportPath = "$tempDir/cleanup_report_$(Get-Date -Format 'yyyyMMdd_HHmmss').txt"
    $reportContent = @"
OpenAvnu Repository Cleanup Report
Generated: $(Get-Date)
Mode: $(if ($KeepOriginals) { 'Copy' } else { 'Move' })

Summary:
- Files moved: $moveCount
- Files copied: $copyCount  
- Files skipped: $skipCount

Target directories created:
$(($targetDirs | ForEach-Object { "- $_" }) -join "`n")

This cleanup organized scattered test files, logs, configs, and legacy scripts
into the standardized directory structure under docs/tests/.
"@
    
    Set-Content -Path $reportPath -Value $reportContent
    Write-Host "Cleanup report saved to: $reportPath" -ForegroundColor Green
}

Write-Host "`nRepository is now organized according to the new testing framework structure!" -ForegroundColor Green
