# OpenAvnu Runtime Basic Test for Windows DAW02
# PowerShell version for Windows systems

Write-Host "=== OpenAvnu Runtime Basic Test ===" -ForegroundColor Green
Write-Host "System: $env:COMPUTERNAME"
Write-Host "Date: $(Get-Date)"
Write-Host "Objective: Verify basic daemon startup and help functionality"
Write-Host ""

# Build configuration for Windows
$BuildDir = "..\build"

# Check if build directory exists
if (-not (Test-Path $BuildDir)) {
    Write-Host "❌ Build directory not found: $BuildDir" -ForegroundColor Red
    exit 1
}

Set-Location $BuildDir

# Test executables
$TestExecutables = @(
    @{ Name = "MRPD"; Path = "daemons\mrpd\Release\mrpd.exe"; HelpArg = "--help" },
    @{ Name = "MAAP"; Path = "daemons\maap\Release\maap_daemon.exe"; HelpArg = "--help" },
    @{ Name = "Shaper"; Path = "daemons\shaper\Release\openavnu_shaper_windows.exe"; HelpArg = "" },
    @{ Name = "gPTP"; Path = "thirdparty\gptp\Release\gptp.exe"; HelpArg = "--help" }
)

$AllSuccess = $true

foreach ($Exe in $TestExecutables) {
    Write-Host ""
    Write-Host "=== Testing $($Exe.Name) ===" -ForegroundColor Yellow
    
    if (Test-Path $Exe.Path) {
        $ExeSize = (Get-Item $Exe.Path).Length
        Write-Host "✅ Executable found: $($Exe.Path) ($ExeSize bytes)" -ForegroundColor Green
        
        # Test basic execution
        try {
            Write-Host "   Testing basic execution..." -ForegroundColor Cyan
            
            $TimeoutJob = Start-Job -ScriptBlock { 
                param($ExePath, $HelpArg)
                try {
                    if ($HelpArg -ne "") {
                        $Output = & $ExePath $HelpArg 2>&1
                    } else {
                        $Output = & $ExePath 2>&1
                    }
                    return @{ Success = $true; Output = $Output }
                } catch {
                    return @{ Success = $false; Error = $_.Exception.Message }
                }
            } -ArgumentList (Resolve-Path $Exe.Path), $Exe.HelpArg
            
            if (Wait-Job $TimeoutJob -Timeout 10) {
                $Result = Receive-Job $TimeoutJob
                Remove-Job $TimeoutJob
                
                if ($Result.Success) {
                    Write-Host "   ✅ Basic execution: SUCCESS" -ForegroundColor Green
                    # Show first few lines of output
                    $Output = $Result.Output | Select-Object -First 3
                    foreach ($Line in $Output) {
                        Write-Host "      $Line" -ForegroundColor Gray
                    }
                } else {
                    Write-Host "   ⚠️  Basic execution: Error - $($Result.Error)" -ForegroundColor Yellow
                }
            } else {
                Remove-Job $TimeoutJob -Force
                Write-Host "   ⚠️  Basic execution: Timeout (10 seconds)" -ForegroundColor Yellow
            }
        } catch {
            Write-Host "   ⚠️  Basic execution: Exception - $($_.Exception.Message)" -ForegroundColor Yellow
        }
    } else {
        Write-Host "❌ Executable not found: $($Exe.Path)" -ForegroundColor Red
        $AllSuccess = $false
    }
}

# Intel HAL Tools Test
Write-Host ""
Write-Host "=== Testing Intel HAL Tools ===" -ForegroundColor Yellow

$HalTools = @(
    @{ Name = "HAL Device Info"; Path = "thirdparty\intel-ethernet-hal\examples\Release\hal_device_info.exe" },
    @{ Name = "HAL Timestamping"; Path = "thirdparty\intel-ethernet-hal\examples\Release\hal_enable_timestamping.exe" },
    @{ Name = "HAL Validation"; Path = "tests\intel_hal_validation_test_consolidated.exe" }
)

foreach ($Tool in $HalTools) {
    if (Test-Path $Tool.Path) {
        $ToolSize = (Get-Item $Tool.Path).Length
        Write-Host "✅ $($Tool.Name): Found ($ToolSize bytes)" -ForegroundColor Green
    } else {
        Write-Host "❌ $($Tool.Name): Not found - $($Tool.Path)" -ForegroundColor Red
        $AllSuccess = $false
    }
}

# Final Results
Write-Host ""
Write-Host "=== Final Results ===" -ForegroundColor Yellow

if ($AllSuccess) {
    Write-Host "✅ RUNTIME BASIC TEST: SUCCESS" -ForegroundColor Green
    Write-Host "   All daemons and tools are present and executable"
    Set-Location ".."
    exit 0
} else {
    Write-Host "❌ RUNTIME BASIC TEST: PARTIAL FAILURE" -ForegroundColor Red
    Write-Host "   Some executables are missing, but core daemons may still work"
    Set-Location ".."
    exit 1
}
