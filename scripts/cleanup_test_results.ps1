# OpenAvnu Repository Test Results Cleanup Script
# Organizes scattered test logs, results, and temporary files into proper structure
# Date: 2025-01-11

param(
    [switch]$DryRun = $false,      # Preview changes without actually moving files
    [switch]$Verbose = $false,     # Show detailed operations
    [switch]$Archive = $false      # Archive old results instead of organizing by date
)

Write-Host "=== OpenAvnu Repository Cleanup ===" -ForegroundColor Green
Write-Host "$(if ($DryRun) { 'DRY RUN MODE - No files will be moved' } else { 'LIVE MODE - Files will be moved' })" -ForegroundColor $(if ($DryRun) { "Yellow" } else { "Green" })
Write-Host ""

# Define target directories
$resultsDir = "docs/tests/results"
$archiveDir = "docs/tests/archive"
$configDir = "docs/tests/config"
$tempDir = "docs/tests/temp"

# Create target directories if they don't exist
if (-not $DryRun) {
    @($resultsDir, $archiveDir, $configDir, $tempDir) | ForEach-Object {
        if (-not (Test-Path $_)) {
            New-Item -ItemType Directory -Path $_ -Force | Out-Null
            Write-Host "Created directory: $_" -ForegroundColor Green
        }
    }
}

# Define file patterns and their target locations
$cleanupRules = @{
    # Test log files scattered in root
    "*.log" = @{
        "TargetDir" = $resultsDir
        "Pattern" = "Root level log files"
        "SubFolder" = "legacy_logs"
    }
    
    # JSON test results in timestamp_analysis
    "tools/timestamp_analysis/*.json" = @{
        "TargetDir" = $resultsDir  
        "Pattern" = "Timestamp analysis JSON results"
        "SubFolder" = "timestamp_analysis"
        "Exclude" = @("config*.json")  # Keep config files separate
    }
    
    # TXT test results in timestamp_analysis
    "tools/timestamp_analysis/*.txt" = @{
        "TargetDir" = $resultsDir
        "Pattern" = "Timestamp analysis text results" 
        "SubFolder" = "timestamp_analysis"
        "Exclude" = @("README.txt", "PYTHON_SETUP.txt")  # Keep documentation
    }
    
    # Configuration files (keep these organized separately)
    "tools/timestamp_analysis/config*.json" = @{
        "TargetDir" = $configDir
        "Pattern" = "Configuration files"
        "SubFolder" = "timestamp_analysis"
    }
    
    # Temporary/development files
    "*CMakeLists*.txt" = @{
        "TargetDir" = $tempDir
        "Pattern" = "Temporary CMake files"
        "SubFolder" = "cmake_temp"
        "Exclude" = @("CMakeLists.txt")  # Keep main CMake file
    }
    
    # Intel HAL test logs
    "intel_hal_test_log.txt" = @{
        "TargetDir" = $resultsDir
        "Pattern" = "Intel HAL test logs"
        "SubFolder" = "hal_testing"
    }
}

function Move-TestFile {
    param(
        [string]$SourcePath,
        [string]$TargetDir, 
        [string]$SubFolder,
        [string]$Description
    )
    
    if (-not (Test-Path $SourcePath)) {
        return $false
    }
    
    $fileName = Split-Path $SourcePath -Leaf
    $targetSubDir = Join-Path $TargetDir $SubFolder
    $targetPath = Join-Path $targetSubDir $fileName
    
    # Create subdirectory if needed
    if (-not $DryRun -and -not (Test-Path $targetSubDir)) {
        New-Item -ItemType Directory -Path $targetSubDir -Force | Out-Null
    }
    
    # Check if target file already exists
    if (Test-Path $targetPath) {
        $sourceInfo = Get-Item $SourcePath
        $targetInfo = Get-Item $targetPath
        
        if ($sourceInfo.LastWriteTime -le $targetInfo.LastWriteTime) {
            Write-Host "  SKIP: $fileName (newer version exists in target)" -ForegroundColor Yellow
            if (-not $DryRun) {
                Remove-Item $SourcePath -Force
                Write-Host "  REMOVED: Older duplicate from source" -ForegroundColor Gray
            }
            return $true
        } else {
            # Rename existing file with timestamp
            $timestamp = $targetInfo.LastWriteTime.ToString("yyyyMMdd_HHmmss")
            $backupName = "$($targetInfo.BaseName)_backup_$timestamp$($targetInfo.Extension)"
            $backupPath = Join-Path $targetSubDir $backupName
            
            if (-not $DryRun) {
                Move-Item $targetPath $backupPath
                Write-Host "  BACKUP: Existing file renamed to $backupName" -ForegroundColor Cyan
            }
        }
    }
    
    if ($DryRun) {
        Write-Host "  WOULD MOVE: $SourcePath -> $targetPath" -ForegroundColor Cyan
    } else {
        Move-Item $SourcePath $targetPath
        Write-Host "  MOVED: $fileName -> $SubFolder/" -ForegroundColor Green
    }
    
    return $true
}

# Execute cleanup rules
$totalFiles = 0
$processedFiles = 0

foreach ($rule in $cleanupRules.GetEnumerator()) {
    $pattern = $rule.Key
    $config = $rule.Value
    
    Write-Host "Processing: $($config.Pattern)" -ForegroundColor Yellow
    
    # Handle different patterns
    if ($pattern.Contains("/")) {
        # Path-specific pattern
        $searchPath = Split-Path $pattern -Parent
        $filePattern = Split-Path $pattern -Leaf
        
        if (Test-Path $searchPath) {
            $files = Get-ChildItem -Path $searchPath -Filter $filePattern -File
        } else {
            $files = @()
        }
    } else {
        # Root-level pattern
        $files = Get-ChildItem -Path "." -Filter $pattern -File
    }
    
    $totalFiles += $files.Count
    
    foreach ($file in $files) {
        # Check exclusions
        $excluded = $false
        if ($config.ContainsKey("Exclude")) {
            foreach ($excludePattern in $config.Exclude) {
                if ($file.Name -like $excludePattern) {
                    $excluded = $true
                    if ($Verbose) {
                        Write-Host "  EXCLUDE: $($file.Name) (matches $excludePattern)" -ForegroundColor Gray
                    }
                    break
                }
            }
        }
        
        if (-not $excluded) {
            if (Move-TestFile -SourcePath $file.FullName -TargetDir $config.TargetDir -SubFolder $config.SubFolder -Description $config.Pattern) {
                $processedFiles++
            }
        }
    }
    
    Write-Host ""
}

# Special handling for any remaining scattered log files in subdirectories
Write-Host "Scanning for additional scattered test files..." -ForegroundColor Yellow

$additionalLogs = Get-ChildItem -Path "." -Recurse -Include "*.log", "*test*.json", "*test*.txt" | Where-Object {
    $_.FullName -notlike "*docs/tests/*" -and
    $_.FullName -notlike "*build/*" -and  
    $_.FullName -notlike "*thirdparty/*" -and
    $_.FullName -notlike "*.git*" -and
    $_.FullName -notlike "*node_modules*" -and
    $_.FullName -notlike "*lib/la_avdecc/tests/data/*" -and  # Exclude test data
    $_.FullName -notlike "*examples/osx/*"  # Exclude macOS examples
}

foreach ($file in $additionalLogs) {
    $relativePath = $file.FullName.Replace((Get-Location).Path, "").TrimStart('\')
    
    if (Move-TestFile -SourcePath $file.FullName -TargetDir $resultsDir -SubFolder "scattered_results" -Description "Additional scattered files") {
        $processedFiles++
    }
}

# Summary
Write-Host "=== Cleanup Summary ===" -ForegroundColor Green
Write-Host "Total files found: $totalFiles" -ForegroundColor White
Write-Host "Files processed: $processedFiles" -ForegroundColor White

if ($processedFiles -gt 0) {
    Write-Host ""
    Write-Host "Organized structure:" -ForegroundColor Cyan
    
    if (Test-Path $resultsDir) {
        Write-Host "📁 $resultsDir/" -ForegroundColor Cyan
        Get-ChildItem $resultsDir -Directory | ForEach-Object {
            $count = (Get-ChildItem $_.FullName -File).Count
            Write-Host "  📂 $($_.Name)/ ($count files)" -ForegroundColor Gray
        }
    }
    
    if (Test-Path $configDir) {
        Write-Host "📁 $configDir/" -ForegroundColor Cyan
        Get-ChildItem $configDir -Directory | ForEach-Object {
            $count = (Get-ChildItem $_.FullName -File).Count  
            Write-Host "  📂 $($_.Name)/ ($count files)" -ForegroundColor Gray
        }
    }
    
    if (Test-Path $tempDir) {
        Write-Host "📁 $tempDir/" -ForegroundColor Cyan
        Get-ChildItem $tempDir -Directory | ForEach-Object {
            $count = (Get-ChildItem $_.FullName -File).Count
            Write-Host "  📂 $($_.Name)/ ($count files)" -ForegroundColor Gray
        }
    }
}

# Create cleanup documentation
$cleanupDoc = @"
# Test Results Cleanup Report

**Date**: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')
**Mode**: $(if ($DryRun) { 'Dry Run (Preview)' } else { 'Live Cleanup' })
**System**: $env:COMPUTERNAME

## Summary
- Total files found: $totalFiles
- Files processed: $processedFiles

## Organization Structure

### docs/tests/results/
Test results and logs from hardware testing, organized by:
- `legacy_logs/` - Log files from root directory  
- `timestamp_analysis/` - Results from timestamp analysis tools
- `hal_testing/` - Intel HAL test logs
- `scattered_results/` - Other test files found throughout repository

### docs/tests/config/ 
Configuration files for testing tools:
- `timestamp_analysis/` - Timestamp analyzer configurations

### docs/tests/temp/
Temporary development files:
- `cmake_temp/` - Temporary CMake files

## Recommendations

1. **Future Test Results**: All new test results should go directly to `docs/tests/results/`
2. **Configuration Management**: Keep config files in `docs/tests/config/`
3. **Cleanup Schedule**: Run this cleanup script monthly to maintain organization
4. **Archive Old Results**: Consider archiving results older than 6 months

## Next Steps

- Update CI/CD workflows to use organized directory structure
- Add .gitignore entries to prevent scattered test files
- Create documentation for test result organization standards
"@

$reportPath = "docs/tests/cleanup_report_$(Get-Date -Format 'yyyyMMdd_HHmmss').md"

if (-not $DryRun) {
    $cleanupDoc | Out-File -FilePath $reportPath -Encoding UTF8
    Write-Host ""
    Write-Host "📄 Cleanup report saved: $reportPath" -ForegroundColor Green
}

if ($DryRun) {
    Write-Host ""
    Write-Host "🔍 This was a dry run. To actually move files, run without -DryRun flag" -ForegroundColor Yellow
} else {
    Write-Host ""
    Write-Host "✅ Repository cleanup complete!" -ForegroundColor Green
}
