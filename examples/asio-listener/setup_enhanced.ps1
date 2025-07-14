# Enhanced ASIO Audio Listener Setup Script
# Automatically configures the development environment for professional audio

param(
    [string]$AsioSdkPath = "",
    [string]$PcapSdkPath = "",
    [switch]$Force,
    [switch]$Verify
)

Write-Host "========================================" -ForegroundColor Green
Write-Host "Enhanced ASIO Audio Listener Setup" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host ""

function Test-AdminRights {
    $currentUser = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = New-Object Security.Principal.WindowsPrincipal($currentUser)
    return $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

function Set-EnvironmentVariableAdvanced {
    param(
        [string]$Name,
        [string]$Value,
        [string]$Scope = "User"
    )
    
    try {
        [Environment]::SetEnvironmentVariable($Name, $Value, $Scope)
        $env:$Name = $Value  # Set for current session
        Write-Host "✓ Set $Name = $Value" -ForegroundColor Green
        return $true
    }
    catch {
        Write-Host "✗ Failed to set $Name" -ForegroundColor Red
        Write-Host "  Error: $($_.Exception.Message)" -ForegroundColor Yellow
        return $false
    }
}

function Find-AsioSdk {
    $searchPaths = @(
        "C:\ASIOSDK2.3",
        "C:\ASIO_SDK",
        "C:\Program Files\ASIO_SDK",
        "C:\Program Files (x86)\ASIO_SDK",
        "$env:USERPROFILE\Documents\ASIOSDK2.3",
        "$env:USERPROFILE\Downloads\ASIOSDK2.3"
    )
    
    foreach ($path in $searchPaths) {
        if (Test-Path "$path\common\asio.h") {
            return $path
        }
    }
    return $null
}

function Find-PcapSdk {
    $searchPaths = @(
        "C:\npcap-sdk",
        "C:\WpdPack",
        "C:\Program Files\Npcap\SDK",
        "$env:USERPROFILE\npcap-sdk",
        "$env:USERPROFILE\Downloads\npcap-sdk"
    )
    
    foreach ($path in $searchPaths) {
        if (Test-Path "$path\Include\pcap.h") {
            return $path
        }
    }
    return $null
}

function Test-VisualStudio {
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) {
        $vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2>$null
        return $vsPath -ne $null
    }
    return $false
}

function Download-AsioSdk {
    Write-Host "ASIO SDK Download Instructions:" -ForegroundColor Yellow
    Write-Host "1. Visit: https://www.steinberg.net/developers/" -ForegroundColor White
    Write-Host "2. Register for a free developer account" -ForegroundColor White
    Write-Host "3. Download ASIO SDK 2.3 or later" -ForegroundColor White
    Write-Host "4. Extract to C:\ASIOSDK2.3" -ForegroundColor White
    Write-Host "5. Rerun this setup script" -ForegroundColor White
    Write-Host ""
}

function Download-PcapSdk {
    Write-Host "Packet Capture SDK Options:" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "Option 1 - Npcap SDK (Recommended for Windows 10/11):" -ForegroundColor White
    Write-Host "1. Download: https://npcap.com/dist/npcap-sdk-1.13.zip" -ForegroundColor Cyan
    Write-Host "2. Extract to C:\npcap-sdk" -ForegroundColor White
    Write-Host ""
    Write-Host "Option 2 - WinPcap SDK (Legacy):" -ForegroundColor White
    Write-Host "1. Download: https://www.winpcap.org/install/bin/WpdPack_4_1_2.zip" -ForegroundColor Cyan
    Write-Host "2. Extract to C:\WpdPack" -ForegroundColor White
    Write-Host ""
}

# Main setup logic
Write-Host "Checking development environment..." -ForegroundColor Cyan
Write-Host ""

# Check if running as administrator
$isAdmin = Test-AdminRights
if ($isAdmin) {
    Write-Host "✓ Running with administrator privileges" -ForegroundColor Green
    $envScope = "Machine"
} else {
    Write-Host "⚠ Running as regular user (some features may be limited)" -ForegroundColor Yellow
    $envScope = "User"
}

# Check Visual Studio
Write-Host "Checking Visual Studio..." -ForegroundColor Cyan
if (Test-VisualStudio) {
    Write-Host "✓ Visual Studio with C++ tools found" -ForegroundColor Green
} else {
    Write-Host "✗ Visual Studio with C++ tools not found" -ForegroundColor Red
    Write-Host "  Download from: https://visualstudio.microsoft.com/downloads/" -ForegroundColor Yellow
    Write-Host "  Required: Visual Studio 2019/2022 with C++ build tools" -ForegroundColor Yellow
}
Write-Host ""

# Setup ASIO SDK
Write-Host "Setting up ASIO SDK..." -ForegroundColor Cyan
$asioPath = $AsioSdkPath
if (-not $asioPath) {
    $asioPath = $env:ASIOSDK_DIR
}
if (-not $asioPath) {
    $asioPath = Find-AsioSdk
}

if ($asioPath -and (Test-Path "$asioPath\common\asio.h")) {
    Write-Host "✓ ASIO SDK found at: $asioPath" -ForegroundColor Green
    if (Set-EnvironmentVariableAdvanced -Name "ASIOSDK_DIR" -Value $asioPath -Scope $envScope) {
        Write-Host "✓ ASIOSDK_DIR environment variable configured" -ForegroundColor Green
    }
} else {
    Write-Host "✗ ASIO SDK not found" -ForegroundColor Red
    Download-AsioSdk
}
Write-Host ""

# Setup PCAP SDK
Write-Host "Setting up Packet Capture SDK..." -ForegroundColor Cyan
$pcapPath = $PcapSdkPath
if (-not $pcapPath) {
    $pcapPath = Find-PcapSdk
}

if ($pcapPath -and (Test-Path "$pcapPath\Include\pcap.h")) {
    Write-Host "✓ PCAP SDK found at: $pcapPath" -ForegroundColor Green
    if (Set-EnvironmentVariableAdvanced -Name "PCAP_DIR" -Value $pcapPath -Scope $envScope) {
        Write-Host "✓ PCAP_DIR environment variable configured" -ForegroundColor Green
    }
} else {
    Write-Host "✗ PCAP SDK not found" -ForegroundColor Red
    Download-PcapSdk
}
Write-Host ""

# Verification
if ($Verify) {
    Write-Host "Verifying setup..." -ForegroundColor Cyan
    Write-Host ""
    
    # Test build
    if (Test-Path "build.bat") {
        Write-Host "Testing build process..." -ForegroundColor White
        $buildResult = & ".\build.bat" 2>&1
        if ($LASTEXITCODE -eq 0) {
            Write-Host "✓ Build test successful" -ForegroundColor Green
        } else {
            Write-Host "✗ Build test failed" -ForegroundColor Red
            Write-Host "Build output:" -ForegroundColor Yellow
            Write-Host $buildResult -ForegroundColor Gray
        }
    }
    Write-Host ""
}

# Summary
Write-Host "========================================" -ForegroundColor Green
Write-Host "Setup Summary" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green

$setupComplete = $true

if ($env:ASIOSDK_DIR -and (Test-Path "$env:ASIOSDK_DIR\common\asio.h")) {
    Write-Host "✓ ASIO SDK: Ready" -ForegroundColor Green
} else {
    Write-Host "⚠ ASIO SDK: Missing (will use stub)" -ForegroundColor Yellow
}

if ($env:PCAP_DIR -and (Test-Path "$env:PCAP_DIR\Include\pcap.h")) {
    Write-Host "✓ PCAP SDK: Ready" -ForegroundColor Green
} else {
    Write-Host "✗ PCAP SDK: Missing (required)" -ForegroundColor Red
    $setupComplete = $false
}

if (Test-VisualStudio) {
    Write-Host "✓ Visual Studio: Ready" -ForegroundColor Green
} else {
    Write-Host "✗ Visual Studio: Missing (required)" -ForegroundColor Red
    $setupComplete = $false
}

Write-Host ""

if ($setupComplete) {
    Write-Host "🎉 Setup complete! You can now build the Enhanced ASIO Audio Listener." -ForegroundColor Green
    Write-Host ""
    Write-Host "Next steps:" -ForegroundColor White
    Write-Host "1. Run: .\build.bat" -ForegroundColor Cyan
    Write-Host "2. Install ASIO driver (e.g., ASIO4ALL)" -ForegroundColor Cyan
    Write-Host "3. Test: .\asio_listener.exe 'Your Network Interface'" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "For detailed usage instructions, see README_ENHANCED.md" -ForegroundColor Yellow
} else {
    Write-Host "⚠ Setup incomplete. Please install missing components and rerun setup." -ForegroundColor Yellow
    Write-Host ""
    Write-Host "After installing missing components:" -ForegroundColor White
    Write-Host "1. Restart PowerShell/VS Code" -ForegroundColor Cyan
    Write-Host "2. Rerun: .\setup_enhanced.ps1 -Verify" -ForegroundColor Cyan
}

Write-Host ""
Write-Host "For support, check the troubleshooting section in README_ENHANCED.md" -ForegroundColor Gray
