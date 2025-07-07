# PowerShell script to run gPTP daemon on Windows

param(
    [Parameter(Mandatory=$true)]
    [string]$NetworkInterface,
    
    [Parameter(Mandatory=$false)]
    [string[]]$AdditionalArgs
)

if (-not $NetworkInterface) {
    Write-Host "Please enter network interface name as parameter. For example:"
    Write-Host ".\run_gptp.ps1 -NetworkInterface 'Ethernet'"
    Write-Host ""
    Write-Host "To list available network interfaces:"
    Write-Host "Get-NetAdapter | Select-Object Name, InterfaceDescription"
    exit 1
}

# Check if gPTP daemon exists
$gptpPaths = @(
    "thirdparty\gptp\windows\daemon_cl\x64\Release\daemon_cl.exe",
    "thirdparty\gptp\windows\daemon_cl\Release\daemon_cl.exe", 
    "build\thirdparty\gptp\windows\daemon_cl\Release\daemon_cl.exe",
    "build\thirdparty\gptp\windows\daemon_cl\x64\Release\daemon_cl.exe"
)

$gptpExe = $null
foreach ($path in $gptpPaths) {
    if (Test-Path $path) {
        $gptpExe = $path
        break
    }
}

if (-not $gptpExe) {
    Write-Host "Error: gPTP daemon not found. Please build the project first:"
    Write-Host "  mkdir build"
    Write-Host "  cd build"
    Write-Host "  cmake .."
    Write-Host "  cmake --build . --config Release"
    Write-Host ""
    Write-Host "Or build manually in Visual Studio from thirdparty/gptp/windows/"
    exit 1
}

Write-Host "Starting gPTP daemon from: $gptpExe"
Write-Host "Network Interface: $NetworkInterface"

# Combine all arguments
$allArgs = @($NetworkInterface) + $AdditionalArgs

# Start the gPTP daemon
& $gptpExe @allArgs
