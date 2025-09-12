#!/usr/bin/env pwsh

<#
.SYNOPSIS
    Quick gPTP packet reception test
.DESCRIPTION
    Tests exactly what happens when gPTP tries to receive network packets
#>

Write-Host "🔍 Quick gPTP Packet Reception Test" -ForegroundColor Magenta
Write-Host "====================================" -ForegroundColor Magenta

# Get Intel adapter
$adapters = Get-NetAdapter | Where-Object { $_.Status -eq "Up" -and $_.Virtual -eq $false }
$intelAdapter = $adapters | Where-Object { $_.InterfaceDescription -like "*Intel*" }

if ($intelAdapter) {
    Write-Host "✅ Intel adapter: $($intelAdapter.InterfaceDescription)" -ForegroundColor Green
    Write-Host "📍 MAC: $($intelAdapter.MacAddress)" -ForegroundColor White
    
    $macFormatted = $intelAdapter.MacAddress.Replace("-", ":")
    
    # Change to gPTP directory
    Push-Location "thirdparty\gptp\build_gptp\Release"
    
    if (Test-Path "gptp2.exe") {
        Write-Host "✅ Found gPTP executable" -ForegroundColor Green
        
        # Test 5-second packet reception
        Write-Host "`n🚀 Testing packet reception (5 seconds)..." -ForegroundColor Cyan
        
        $process = Start-Process -FilePath ".\gptp2.exe" -ArgumentList @($macFormatted, "-f", "test_milan_config.ini") -NoNewWindow -PassThru -RedirectStandardOutput "packet_test.log" -RedirectStandardError "packet_test_err.log"
        
        Start-Sleep -Seconds 5
        
        if (-not $process.HasExited) {
            $process.Kill()
            Write-Host "⏹️  Stopped gPTP after timeout" -ForegroundColor Yellow
        }
        
        # Analyze results
        if (Test-Path "packet_test.log") {
            $output = Get-Content "packet_test.log" -Raw
            
            Write-Host "`n📊 Results:" -ForegroundColor Yellow
            
            if ($output -match "Successfully opened interface") {
                Write-Host "   ✅ Interface opened successfully" -ForegroundColor Green
            } else {
                Write-Host "   ❌ Failed to open interface" -ForegroundColor Red
            }
            
            if ($output -match "PTP PACKET.*DETECTED") {
                Write-Host "   ✅ PTP packets received!" -ForegroundColor Green
            } else {
                Write-Host "   ❌ No PTP packets received" -ForegroundColor Red
            }
            
            # Show timeout count
            $timeoutMatches = [regex]::Matches($output, "consecutive timeouts")
            Write-Host "   📈 Timeout events: $($timeoutMatches.Count)" -ForegroundColor Gray
            
            # Show last few lines of output
            $lines = $output -split "`n" | Where-Object { $_.Trim() } | Select-Object -Last 3
            Write-Host "`n📄 Last output lines:" -ForegroundColor Gray
            foreach ($line in $lines) {
                Write-Host "   $line" -ForegroundColor Gray
            }
        } else {
            Write-Host "❌ No output generated!" -ForegroundColor Red
        }
        
        if (Test-Path "packet_test_err.log") {
            $errors = Get-Content "packet_test_err.log" -Raw
            if ($errors.Trim()) {
                Write-Host "`n⚠️  Errors:" -ForegroundColor Red
                Write-Host $errors -ForegroundColor Red
            }
        }
    } else {
        Write-Host "❌ gPTP executable not found!" -ForegroundColor Red
    }
    
    Pop-Location
} else {
    Write-Host "❌ No Intel adapter found!" -ForegroundColor Red
}

Write-Host "`n🔍 Test complete." -ForegroundColor Magenta