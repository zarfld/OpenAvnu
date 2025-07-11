# OpenAvnu Repository Test Results Cleanup Script
# Organizes scattered test logs, results, and temporary files
# Date: 2025-01-11

param(
    [switch]$DryRun = $false,      # Preview changes without moving files
    [switch]$Verbose = $false      # Show detailed operations
)

Write-Host "=== OpenAvnu Repository Cleanup ===" -ForegroundColor Green
Write-Host "Mode: $(if ($DryRun) { 'DRY RUN - Preview only' } else { 'LIVE MODE - Files will be moved' })" -ForegroundColor $(if ($DryRun) { "Yellow" } else { "Green" })
Write-Host ""

# Target directories
$resultsDir = "docs/tests/results"
$configDir = "docs/tests/config"  
$tempDir = "docs/tests/temp"

# Create directories if needed
if (-not $DryRun) {
    @($resultsDir, $configDir, $tempDir) | ForEach-Object {
        if (-not (Test-Path $_)) {
            New-Item -ItemType Directory -Path $_ -Force | Out-Null
            Write-Host "Created: $_" -ForegroundColor Green
        }
    }
}

$movedFiles = 0

# 1. Root-level log files
Write-Host "1. Processing root-level log files..." -ForegroundColor Yellow
$rootLogs = Get-ChildItem -Path "." -Filter "*.log" -File | Where-Object { $_.Name -like "*test*" -or $_.Name -like "*register*" }

foreach ($log in $rootLogs) {
    $targetPath = Join-Path $resultsDir "legacy_logs"
    if (-not $DryRun -and -not (Test-Path $targetPath)) {
        New-Item -ItemType Directory -Path $targetPath -Force | Out-Null
    }
    
    $finalPath = Join-Path $targetPath $log.Name
    
    if ($DryRun) {
        Write-Host "  WOULD MOVE: $($log.Name) -> $targetPath" -ForegroundColor Cyan
    } else {
        Move-Item $log.FullName $finalPath
        Write-Host "  MOVED: $($log.Name)" -ForegroundColor Green
    }
    $movedFiles++
}

# 2. Timestamp analysis results
Write-Host "2. Processing timestamp analysis results..." -ForegroundColor Yellow
$timestampDir = "tools/timestamp_analysis"

if (Test-Path $timestampDir) {
    # JSON results (except config files)
    $jsonResults = Get-ChildItem -Path $timestampDir -Filter "*.json" | Where-Object { $_.Name -notlike "config*" }
    $txtResults = Get-ChildItem -Path $timestampDir -Filter "*.txt" | Where-Object { $_.Name -notlike "README*" -and $_.Name -notlike "*SETUP*" }
    
    $targetPath = Join-Path $resultsDir "timestamp_analysis"
    if (-not $DryRun -and -not (Test-Path $targetPath)) {
        New-Item -ItemType Directory -Path $targetPath -Force | Out-Null
    }
    
    foreach ($file in ($jsonResults + $txtResults)) {
        $finalPath = Join-Path $targetPath $file.Name
        
        if ($DryRun) {
            Write-Host "  WOULD MOVE: $($file.Name) -> timestamp_analysis/" -ForegroundColor Cyan
        } else {
            Move-Item $file.FullName $finalPath
            Write-Host "  MOVED: $($file.Name)" -ForegroundColor Green
        }
        $movedFiles++
    }
    
    # Config files to separate location
    $configFiles = Get-ChildItem -Path $timestampDir -Filter "config*.json"
    $configTargetPath = Join-Path $configDir "timestamp_analysis"
    if (-not $DryRun -and -not (Test-Path $configTargetPath)) {
        New-Item -ItemType Directory -Path $configTargetPath -Force | Out-Null
    }
    
    foreach ($configFile in $configFiles) {
        $finalPath = Join-Path $configTargetPath $configFile.Name
        
        if ($DryRun) {
            Write-Host "  WOULD MOVE CONFIG: $($configFile.Name) -> config/timestamp_analysis/" -ForegroundColor Cyan
        } else {
            Copy-Item $configFile.FullName $finalPath  # Copy instead of move to keep original
            Write-Host "  COPIED CONFIG: $($configFile.Name)" -ForegroundColor Green
        }
    }
}

# 3. Temporary development files  
Write-Host "3. Processing temporary development files..." -ForegroundColor Yellow
$tempFiles = Get-ChildItem -Path "." -Filter "*CMakeLists*.txt" | Where-Object { $_.Name -ne "CMakeLists.txt" }
$tempFiles += Get-ChildItem -Path "." -Filter "intel_hal_test_log.txt"

if ($tempFiles.Count -gt 0) {
    $targetPath = Join-Path $tempDir "development"
    if (-not $DryRun -and -not (Test-Path $targetPath)) {
        New-Item -ItemType Directory -Path $targetPath -Force | Out-Null
    }
    
    foreach ($file in $tempFiles) {
        $finalPath = Join-Path $targetPath $file.Name
        
        if ($DryRun) {
            Write-Host "  WOULD MOVE: $($file.Name) -> temp/development/" -ForegroundColor Cyan
        } else {
            Move-Item $file.FullName $finalPath
            Write-Host "  MOVED: $($file.Name)" -ForegroundColor Green
        }
        $movedFiles++
    }
}

# 4. Check for other scattered test files
Write-Host "4. Scanning for additional test files..." -ForegroundColor Yellow
$additionalFiles = Get-ChildItem -Path "." -Recurse -Include "*.log", "*test*.json" | Where-Object {
    $_.FullName -notlike "*docs/tests/*" -and
    $_.FullName -notlike "*build/*" -and  
    $_.FullName -notlike "*thirdparty/*" -and
    $_.FullName -notlike "*.git*" -and
    $_.FullName -notlike "*lib/la_avdecc/tests/data/*" -and
    $_.FullName -notlike "*examples/osx/*" -and
    $_.Name -like "*test*"
}

if ($additionalFiles.Count -gt 0) {
    $targetPath = Join-Path $resultsDir "scattered"
    if (-not $DryRun -and -not (Test-Path $targetPath)) {
        New-Item -ItemType Directory -Path $targetPath -Force | Out-Null
    }
    
    foreach ($file in $additionalFiles) {
        $relativePath = $file.DirectoryName.Replace((Get-Location).Path, "").TrimStart('\')
        $finalPath = Join-Path $targetPath $file.Name
        
        if ($DryRun) {
            Write-Host "  WOULD MOVE: $relativePath\$($file.Name)" -ForegroundColor Cyan
        } else {
            Move-Item $file.FullName $finalPath
            Write-Host "  MOVED: $($file.Name) from $relativePath" -ForegroundColor Green
        }
        $movedFiles++
    }
}

# Summary
Write-Host ""
Write-Host "=== Cleanup Summary ===" -ForegroundColor Green
Write-Host "Files processed: $movedFiles" -ForegroundColor White

if ($movedFiles -gt 0) {
    Write-Host ""
    Write-Host "New organization:" -ForegroundColor Cyan
    
    if (Test-Path $resultsDir) {
        Write-Host "Results: docs/tests/results/" -ForegroundColor Cyan
        Get-ChildItem $resultsDir -Directory -ErrorAction SilentlyContinue | ForEach-Object {
            $count = (Get-ChildItem $_.FullName -File -ErrorAction SilentlyContinue).Count
            Write-Host "  $($_.Name)/ ($count files)" -ForegroundColor Gray
        }
    }
    
    if (Test-Path $configDir) {
        Write-Host "Config: docs/tests/config/" -ForegroundColor Cyan
        Get-ChildItem $configDir -Directory -ErrorAction SilentlyContinue | ForEach-Object {
            $count = (Get-ChildItem $_.FullName -File -ErrorAction SilentlyContinue).Count
            Write-Host "  $($_.Name)/ ($count files)" -ForegroundColor Gray
        }
    }
}

if ($DryRun) {
    Write-Host ""
    Write-Host "This was a preview. Run without -DryRun to actually move files." -ForegroundColor Yellow
} else {
    Write-Host ""
    Write-Host "Repository cleanup complete!" -ForegroundColor Green
    
    # Create .gitignore entries to prevent future scattered files
    $gitignoreAdditions = @"

# Test results should go in docs/tests/results/
*.log
*test*.json
*test*.txt
intel_hal_test_log.txt
register_access_*.log

"@
    
    if (-not (Get-Content ".gitignore" -ErrorAction SilentlyContinue | Select-String "Test results should go")) {
        Add-Content -Path ".gitignore" -Value $gitignoreAdditions
        Write-Host "Updated .gitignore to prevent future scattered test files" -ForegroundColor Green
    }
}
