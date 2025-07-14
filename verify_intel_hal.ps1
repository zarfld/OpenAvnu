# Intel HAL Integration Verification Script
# This script checks if OpenAvnu daemons were actually built with Intel HAL support

Write-Host "OpenAvnu Intel HAL Integration Verification" -ForegroundColor Green
Write-Host "=" * 50 -ForegroundColor Yellow

# Check CMake configuration
Write-Host "`nChecking CMake Configuration..." -ForegroundColor Cyan
$cacheFile = "build\CMakeCache.txt"
if (Test-Path $cacheFile) {
    $cache = Get-Content $cacheFile
    $halRelated = $cache | Where-Object { $_ -like "*INTEL*" -or $_ -like "*HAL*" }
    
    if ($halRelated) {
        Write-Host "✅ Intel HAL references found in CMake cache:" -ForegroundColor Green
        $halRelated | ForEach-Object { Write-Host "  $_" -ForegroundColor Gray }
    } else {
        Write-Host "❌ No Intel HAL references in CMake cache" -ForegroundColor Red
    }
} else {
    Write-Host "❌ CMakeCache.txt not found" -ForegroundColor Red
}

# Check Intel HAL static library
Write-Host "`nChecking Intel HAL Library..." -ForegroundColor Cyan
$halLib = "build\thirdparty\intel-ethernet-hal\Release\intel-ethernet-hal-static.lib"
if (Test-Path $halLib) {
    $libInfo = Get-ItemProperty $halLib
    Write-Host "✅ Intel HAL static library found: $($libInfo.Length) bytes" -ForegroundColor Green
} else {
    Write-Host "❌ Intel HAL static library not found: $halLib" -ForegroundColor Red
}

# Check daemon sizes (daemons with HAL should be larger)
Write-Host "`nChecking Daemon Executable Sizes..." -ForegroundColor Cyan
$daemons = @(
    @{Name="MAAP"; Path="build\daemons\maap\Release\maap_daemon.exe"},
    @{Name="MRPD"; Path="build\daemons\mrpd\Release\mrpd.exe"}
)

foreach ($daemon in $daemons) {
    if (Test-Path $daemon.Path) {
        $info = Get-ItemProperty $daemon.Path
        $size = $info.Length
        Write-Host "✅ $($daemon.Name): $size bytes" -ForegroundColor Green
        
        # Typical sizes: MAAP without HAL ~30KB, with HAL ~45KB
        #               MRPD without HAL ~50KB, with HAL ~70KB
        if (($daemon.Name -eq "MAAP" -and $size -gt 40000) -or 
            ($daemon.Name -eq "MRPD" -and $size -gt 65000)) {
            Write-Host "  → Size suggests Intel HAL integration" -ForegroundColor Yellow
        } else {
            Write-Host "  → Size may indicate no HAL integration" -ForegroundColor Yellow
        }
    } else {
        Write-Host "❌ $($daemon.Name) not found: $($daemon.Path)" -ForegroundColor Red
    }
}

# Check for Intel HAL source files
Write-Host "`nChecking Intel HAL Source Integration..." -ForegroundColor Cyan
$halSources = Get-ChildItem "thirdparty\intel-ethernet-hal" -Recurse -Filter "*.c" -ErrorAction SilentlyContinue
if ($halSources) {
    Write-Host "✅ Found $($halSources.Count) Intel HAL source files" -ForegroundColor Green
} else {
    Write-Host "❌ No Intel HAL source files found" -ForegroundColor Red
}

Write-Host "`nVerification Complete!" -ForegroundColor Green
