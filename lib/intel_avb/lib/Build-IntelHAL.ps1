# Intel AVB HAL Library Windows Build Script (PowerShell)
#
# This script sets up the Visual Studio environment and builds the Intel HAL library

param(
    [switch]$Clean,
    [switch]$Debug
)

Write-Host "Building Intel AVB HAL Library for Windows..." -ForegroundColor Green

# Check for Visual Studio installation
$vsPath = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
if (-not (Test-Path $vsPath)) {
    Write-Error "Visual Studio 2022 Community not found. Please install Visual Studio with C++ support."
    exit 1
}

# Create output directory
if ($Clean -and (Test-Path "build")) {
    Write-Host "Cleaning build directory..." -ForegroundColor Yellow
    Remove-Item "build" -Recurse -Force
}

if (-not (Test-Path "build")) {
    New-Item -ItemType Directory -Path "build" | Out-Null
}

# Set compile flags
$cflags = "/O2 /W3 /D_CRT_SECURE_NO_WARNINGS /I. /I..\..\..\lib\common"
if ($Debug) {
    $cflags = "/Od /Zi /W3 /D_CRT_SECURE_NO_WARNINGS /I. /I..\..\..\lib\common"
}

# Source files
$sources = @("intel.c", "intel_common.c", "intel_i210.c", "intel_i219.c", "intel_i225.c")
$objects = @()

# Setup Visual Studio environment and compile
Write-Host "Setting up Visual Studio environment..." -ForegroundColor Yellow

try {
    # Start a new process with VS environment
    $buildScript = @"
call "$vsPath"
cd "$PWD"

echo Compiling source files...
"@

    foreach ($source in $sources) {
        $obj = "build\$([System.IO.Path]::GetFileNameWithoutExtension($source)).obj"
        $objects += $obj
        $buildScript += "cl /c $cflags $source /Fo:$obj`n"
    }

    $buildScript += @"

echo Creating static library...
lib /OUT:build\intel_avb.lib $($objects -join ' ')

echo Creating dynamic library...
link /DLL /OUT:build\intel_avb.dll $($objects -join ' ')

echo Compiling test program...
cl $cflags test_intel.c /Fo:build\test_intel.obj
link /OUT:build\test_intel.exe build\test_intel.obj build\intel_avb.lib

echo Build complete!
"@

    # Write build script to temp file and execute
    $tempBat = "build_temp.bat"
    $buildScript | Out-File -FilePath $tempBat -Encoding ASCII
    
    # Execute the batch file
    $result = cmd /c $tempBat
    Write-Host $result

    # Clean up temp file
    Remove-Item $tempBat -ErrorAction SilentlyContinue

    if (Test-Path "build\intel_avb.lib") {
        Write-Host "`nBuild successful!" -ForegroundColor Green
        Write-Host "Output files:" -ForegroundColor Cyan
        Write-Host "  build\intel_avb.lib   - Static library" -ForegroundColor White
        Write-Host "  build\intel_avb.dll   - Dynamic library" -ForegroundColor White
        Write-Host "  build\test_intel.exe  - Test program" -ForegroundColor White
        
        # Show file sizes
        Get-ChildItem "build\*" | ForEach-Object {
            Write-Host "    $($_.Name): $([math]::Round($_.Length/1KB, 1)) KB" -ForegroundColor Gray
        }
    } else {
        Write-Error "Build failed - no output library found"
        exit 1
    }

} catch {
    Write-Error "Build failed: $_"
    exit 1
}
