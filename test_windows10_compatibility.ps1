# ============================================================================
# Windows 10 Compatibility Test for OpenAvnu Daemon Startup
# ============================================================================

Write-Host ""
Write-Host "Windows 10 OpenAvnu Compatibility Test" -ForegroundColor Magenta
Write-Host "=======================================" -ForegroundColor Magenta
Write-Host ""

# Test 1: Path Detection
Write-Host "[TEST 1] Testing dynamic path detection..." -ForegroundColor Cyan

$scriptLocation = Split-Path -Parent $MyInvocation.MyCommand.Path
$workspaceRoot = Split-Path -Parent (Split-Path -Parent $scriptLocation)

Write-Host "Script location: $scriptLocation" -ForegroundColor Gray
Write-Host "Workspace root: $workspaceRoot" -ForegroundColor Gray

# Try different possible build directory locations
$possibleBuildDirs = @(
    "$workspaceRoot\build",
    "$workspaceRoot\..\build", 
    "C:\Users\$env:USERNAME\Source\OpenAvnu\build",
    ".\..\..\build",
    "build"
)

$buildDir = $null
$mrpdPath = $null
$maapPath = $null

Write-Host "Searching for OpenAvnu executables..." -ForegroundColor White
foreach ($dir in $possibleBuildDirs) {
    $testMrpd = "$dir\daemons\mrpd\Release\mrpd.exe"
    $testMaap = "$dir\daemons\maap\Release\maap_daemon.exe"
    
    Write-Host "  Checking: $dir" -ForegroundColor Gray
    if ((Test-Path $testMrpd) -and (Test-Path $testMaap)) {
        $buildDir = $dir
        $mrpdPath = $testMrpd
        $maapPath = $testMaap
        Write-Host "  FOUND: $dir" -ForegroundColor Green
        break
    } else {
        Write-Host "    MRPD: $(if (Test-Path $testMrpd) { 'OK' } else { 'MISSING' })" -ForegroundColor Gray
        Write-Host "    MAAP: $(if (Test-Path $testMaap) { 'OK' } else { 'MISSING' })" -ForegroundColor Gray
    }
}

if (-not $buildDir) {
    Write-Host "ERROR: OpenAvnu executables not found!" -ForegroundColor Red
    Write-Host "Please build OpenAvnu first or check paths" -ForegroundColor Yellow
    exit 1
}

Write-Host "SUCCESS: Found executables" -ForegroundColor Green
Write-Host "  Build dir: $buildDir" -ForegroundColor White
Write-Host "  MRPD: $mrpdPath" -ForegroundColor White  
Write-Host "  MAAP: $maapPath" -ForegroundColor White
Write-Host ""

# Test 2: Working Directory Validation
Write-Host "[TEST 2] Testing working directory validation..." -ForegroundColor Cyan

$workingDir = Split-Path -Parent $mrpdPath
Write-Host "Daemon directory: $workingDir" -ForegroundColor Gray

if (Test-Path $workingDir) {
    Write-Host "SUCCESS: Working directory exists" -ForegroundColor Green
} else {
    Write-Host "WARNING: Daemon directory not found, using build root" -ForegroundColor Yellow
    $workingDir = $buildDir
    if (Test-Path $workingDir) {
        Write-Host "SUCCESS: Build directory exists" -ForegroundColor Green
    } else {
        Write-Host "ERROR: No valid working directory found" -ForegroundColor Red
        exit 1
    }
}

Write-Host "Using working directory: $workingDir" -ForegroundColor White
Write-Host ""

# Test 3: Interface Detection
Write-Host "[TEST 3] Testing Intel interface detection..." -ForegroundColor Cyan

$intelAdapters = Get-NetAdapter | Where-Object { 
    $_.InterfaceDescription -like "*Intel*" -and 
    $_.Status -eq "Up"
}

if ($intelAdapters.Count -eq 0) {
    Write-Host "ERROR: No Intel adapters found!" -ForegroundColor Red
    exit 1
}

Write-Host "Found $($intelAdapters.Count) Intel adapter(s):" -ForegroundColor Green
foreach ($adapter in $intelAdapters) {
    Write-Host "  $($adapter.Name): $($adapter.InterfaceDescription)" -ForegroundColor White
}

$testAdapter = $intelAdapters | Select-Object -First 1
$windowsInterfaceName = $testAdapter.Name
$pcapDeviceName = "\Device\NPF_$($testAdapter.InterfaceGuid)"

Write-Host ""
Write-Host "Test configuration:" -ForegroundColor White
Write-Host "  Windows Interface: $windowsInterfaceName" -ForegroundColor Gray
Write-Host "  PCap Device: $pcapDeviceName" -ForegroundColor Gray
Write-Host ""

# Test 4: Process Start Simulation (without actually starting)
Write-Host "[TEST 4] Testing process start parameters..." -ForegroundColor Cyan

Write-Host "MRPD start command would be:" -ForegroundColor White
Write-Host "  FilePath: $mrpdPath" -ForegroundColor Gray
Write-Host "  Arguments: -i `"$windowsInterfaceName`"" -ForegroundColor Gray
Write-Host "  WorkingDirectory: $workingDir" -ForegroundColor Gray

Write-Host ""
Write-Host "MAAP start command would be:" -ForegroundColor White
Write-Host "  FilePath: $maapPath" -ForegroundColor Gray
Write-Host "  Arguments: -i `"$pcapDeviceName`"" -ForegroundColor Gray
Write-Host "  WorkingDirectory: $workingDir" -ForegroundColor Gray

Write-Host ""
Write-Host "SUCCESS: All compatibility tests passed!" -ForegroundColor Green
Write-Host "The fixed scripts should work on Windows 10" -ForegroundColor Green
Write-Host ""

# Test 5: Suggest next steps
Write-Host "[NEXT STEPS] To test the actual daemon startup:" -ForegroundColor Yellow
Write-Host "1. Run: start_openavnu_final.ps1 (Multi-adapter)" -ForegroundColor White
Write-Host "2. Run: start_openavnu_corrected.ps1 (I219 specific)" -ForegroundColor White
Write-Host "3. Use VS Code task: 'Start OpenAvnu Daemons (Multi-Adapter)'" -ForegroundColor White
Write-Host ""
