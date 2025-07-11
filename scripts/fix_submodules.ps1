#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Fix and validate OpenAvnu submodule configuration
    
.DESCRIPTION
    This script ensures all submodules are properly configured and can be initialized.
    It fixes common submodule issues and validates that all dependencies are accessible.
    
.PARAMETER FixUrls
    Update submodule URLs to use the correct repositories
    
.PARAMETER Validate
    Only validate submodules without making changes
    
.EXAMPLE
    .\fix_submodules.ps1 -FixUrls
    .\fix_submodules.ps1 -Validate
#>

param(
    [switch]$FixUrls,
    [switch]$Validate
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

# Main execution
if ($FixUrls) {
    Fix-SubmoduleUrls
}

$isValid = Validate-Submodules

if ($Validate) {
    Show-SubmoduleStatus
    if ($isValid) {
        Write-Host "`n✅ All submodules are accessible" -ForegroundColor Green
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
