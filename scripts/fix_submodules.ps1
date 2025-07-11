#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Fix and validate OpenAvnu submodule configuration
    
.DESCRIPTION
    This script ensures all submodules are properly configured and can be initialized.
    It fixes common submodule issues and validates that all dependencies are accessible.
    
.PARAMETER FixUrls
    Update submodule URLs to use the correct repositories
    
.PARAMETER UpdateToLatest
    Update all submodules to their latest remote commits
    
.EXAMPLE
    .\fix_submodules.ps1 -FixUrls
    .\fix_submodules.ps1 -Validate
    .\fix_submodules.ps1 -UpdateToLatest
#>

param(
    [switch]$FixUrls,
    [switch]$Validate,
    [switch]$UpdateToLatest
)

$ErrorActionPreference = "Continue"

Write-Host "=== OpenAvnu Submodule Fix and Validation ===" -ForegroundColor Cyan

# Define correct submodule URLs
$submoduleUrls = @{
    "avdecc-lib" = "https://github.com/AVnu/avdecc-lib.git"
    "lib/atl_avb" = "https://github.com/zarfld/atl_avb.git"
    "lib/igb_avb" = "https://github.com/AVnu/igb_avb.git"
    "lib/la_avdecc" = "https://github.com/L-Acoustics/avdecc.git"
    "thirdparty/cpputest" = "https://github.com/cpputest/cpputest.git"
    "thirdparty/gptp" = "https://github.com/zarfld/gptp.git"
    "thirdparty/intel-ethernet-hal" = "https://github.com/zarfld/intel-ethernet-hal.git"
    "thirdparty/open1722" = "https://github.com/COVESA/Open1722.git"
}

function Test-SubmoduleUrl {
    param([string]$url)
    
    try {
        $result = git ls-remote $url HEAD 2>$null
        return $result -ne $null
    } catch {
        return $false
    }
}

function Fix-SubmoduleUrls {
    Write-Host "`nFixing submodule URLs..." -ForegroundColor Yellow
    
    foreach ($path in $submoduleUrls.Keys) {
        $url = $submoduleUrls[$path]
        Write-Host "  Setting $path -> $url" -ForegroundColor White
        
        try {
            git config --file .gitmodules "submodule.$path.url" $url
        } catch {
            Write-Host "    WARNING: Failed to set URL for $path" -ForegroundColor Red
        }
    }
}

function Validate-Submodules {
    Write-Host "`nValidating submodule accessibility..." -ForegroundColor Yellow
    
    $allValid = $true
    
    foreach ($path in $submoduleUrls.Keys) {
        $url = $submoduleUrls[$path]
        Write-Host "  Checking $path ($url)..." -ForegroundColor White
        
        if (Test-SubmoduleUrl $url) {
            Write-Host "    ✅ Accessible" -ForegroundColor Green
        } else {
            Write-Host "    ❌ Not accessible" -ForegroundColor Red
            $allValid = $false
        }
    }
    
    return $allValid
}

function Initialize-Submodules {
    Write-Host "`nInitializing submodules..." -ForegroundColor Yellow
    
    try {
        Write-Host "  Running git submodule sync..." -ForegroundColor White
        git submodule sync
        
        Write-Host "  Running git submodule update --init --recursive..." -ForegroundColor White
        git submodule update --init --recursive
        
        Write-Host "  ✅ Submodules initialized successfully" -ForegroundColor Green
        return $true
    } catch {
        Write-Host "  ❌ Failed to initialize submodules: $($_.Exception.Message)" -ForegroundColor Red
        return $false
    }
}

function Show-SubmoduleStatus {
    Write-Host "`nSubmodule status:" -ForegroundColor Yellow
    git submodule status
}

function Update-SubmodulesToLatest {
    Write-Host "`nUpdating submodules to latest remote commits..." -ForegroundColor Yellow
    
    try {
        # Sync submodules to ensure URLs are current
        git submodule sync
        
        # Update each submodule to latest remote commit
        git submodule foreach --recursive 'git fetch origin && git checkout origin/$(git symbolic-ref --short HEAD) && git branch -D $(git symbolic-ref --short HEAD) 2>/dev/null; git checkout -b $(git symbolic-ref --short HEAD) origin/$(git symbolic-ref --short HEAD)'
        
        Write-Host "  ✅ Submodules updated to latest commits" -ForegroundColor Green
        return $true
    } catch {
        Write-Host "  ❌ Failed to update submodules: $($_.Exception.Message)" -ForegroundColor Red
        return $false
    }
}

function Check-SubmoduleSync {
    Write-Host "`nChecking if submodules are in sync with remotes..." -ForegroundColor Yellow
    
    $outOfSync = @()
    
    # Check each submodule
    git submodule foreach --quiet 'echo "Checking $name..."; git fetch origin 2>/dev/null; LOCAL=$(git rev-parse HEAD); REMOTE=$(git rev-parse origin/$(git symbolic-ref --short HEAD) 2>/dev/null || echo "unknown"); if [ "$LOCAL" != "$REMOTE" ]; then echo "OUT_OF_SYNC: $name ($LOCAL != $REMOTE)"; fi' | Where-Object { $_ -match "OUT_OF_SYNC" }
    
    return $outOfSync.Count -eq 0
}

# Main execution
if ($FixUrls) {
    Fix-SubmoduleUrls
}

if ($UpdateToLatest) {
    Write-Host "`nUpdating submodules to latest commits..." -ForegroundColor Green
    if (Update-SubmodulesToLatest) {
        Show-SubmoduleStatus
        Write-Host "`n🎉 Submodules updated to latest commits!" -ForegroundColor Green
    } else {
        Write-Host "`n❌ Failed to update submodules to latest commits" -ForegroundColor Red
        exit 1
    }
    return
}

$isValid = Validate-Submodules

if ($Validate) {
    $inSync = Check-SubmoduleSync
    Show-SubmoduleStatus
    
    if ($isValid -and $inSync) {
        Write-Host "`n✅ All submodules are accessible and in sync" -ForegroundColor Green
        exit 0
    } elseif ($isValid) {
        Write-Host "`n⚠️ Submodules are accessible but some may be out of sync with remotes" -ForegroundColor Yellow
        Write-Host "Use -UpdateToLatest to sync with latest remote commits" -ForegroundColor Cyan
        exit 0
    } else {
        Write-Host "`n❌ Some submodules are not accessible" -ForegroundColor Red
        exit 1
    }
}

if ($FixUrls -and $isValid) {
    Write-Host "`nURLs fixed and validated. Initializing submodules..." -ForegroundColor Green
    
    if (Initialize-Submodules) {
        Show-SubmoduleStatus
        Write-Host "`n🎉 Submodules fixed and initialized successfully!" -ForegroundColor Green
    } else {
        Write-Host "`n❌ Failed to initialize submodules after URL fix" -ForegroundColor Red
        exit 1
    }
} elseif ($FixUrls) {
    Write-Host "`n⚠️ URLs fixed but some repositories are not accessible" -ForegroundColor Yellow
    Write-Host "Please check network connectivity and repository permissions" -ForegroundColor Yellow
    exit 1
}

Write-Host "`nUse -FixUrls to fix submodule URLs and initialize" -ForegroundColor Cyan
Write-Host "Use -Validate to only check accessibility" -ForegroundColor Cyan
Write-Host "Use -UpdateToLatest to update all submodules to latest remote commits" -ForegroundColor Cyan
