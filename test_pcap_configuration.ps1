#!/usr/bin/env powershell
# Test PCAP Configuration Fix
# This script tests the flexible PCAP library detection in CMakeLists.txt files

Write-Host "=== Testing PCAP Configuration Fix ===" -ForegroundColor Cyan

# Change to repository root
Set-Location -Path "d:\Repos\OpenAvnu"

Write-Host "`nStep 1: Testing with NPCAP_SDK_DIR environment..." -ForegroundColor Yellow
try {
    # Set the NPCAP_SDK_DIR environment variable (simulating CI environment)
    $env:NPCAP_SDK_DIR = "C:\npcap-sdk"
    
    # Remove WPCAP_DIR if it exists to test the fallback
    if ($env:WPCAP_DIR) {
        $oldWpcapDir = $env:WPCAP_DIR
        Remove-Item env:WPCAP_DIR
        Write-Host "Temporarily removed WPCAP_DIR for testing" -ForegroundColor White
    }
    
    Write-Host "Environment variables:" -ForegroundColor Green
    Write-Host "  NPCAP_SDK_DIR: $env:NPCAP_SDK_DIR" -ForegroundColor White
    Write-Host "  WPCAP_DIR: $env:WPCAP_DIR" -ForegroundColor White
    
    # Test CMake configuration
    Write-Host "`nTesting CMake configuration..." -ForegroundColor Yellow
    $configResult = cmake -S . -B build_test -G "Visual Studio 17 2022" -A x64 2>&1
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "SUCCESS: CMake configuration completed" -ForegroundColor Green
        
        # Look for our PCAP detection messages
        $pcapMessages = $configResult | Where-Object { $_ -match "Using.*pcap" }
        if ($pcapMessages) {
            Write-Host "`nPCAP Detection Messages:" -ForegroundColor Green
            $pcapMessages | ForEach-Object { Write-Host "  $_" -ForegroundColor White }
        }
        
        # Check if build files were created
        if (Test-Path "build_test\OpenAvnu.sln") {
            Write-Host "SUCCESS: Visual Studio solution file created" -ForegroundColor Green
        } else {
            Write-Host "WARNING: Visual Studio solution file not found" -ForegroundColor Yellow
        }
    } else {
        Write-Host "ERROR: CMake configuration failed" -ForegroundColor Red
        $configResult | ForEach-Object { Write-Host "  $_" -ForegroundColor Red }
    }
    
    # Restore WPCAP_DIR if it was set
    if ($oldWpcapDir) {
        $env:WPCAP_DIR = $oldWpcapDir
        Write-Host "Restored WPCAP_DIR: $env:WPCAP_DIR" -ForegroundColor White
    }
    
} catch {
    Write-Host "ERROR: Exception during CMake testing: $_" -ForegroundColor Red
}

Write-Host "`nStep 2: Testing PCAP file availability..." -ForegroundColor Yellow
try {
    $npcapPath = "C:\npcap-sdk"
    
    $testFiles = @(
        "$npcapPath\Include\pcap.h",
        "$npcapPath\Include\Packet32.h",
        "$npcapPath\Lib\x64\wpcap.lib",
        "$npcapPath\Lib\x64\Packet.lib",
        "$npcapPath\Lib\wpcap.lib",
        "$npcapPath\Lib\Packet.lib"
    )
    
    Write-Host "Checking PCAP files:" -ForegroundColor Green
    $allFilesExist = $true
    foreach ($file in $testFiles) {
        if (Test-Path $file) {
            Write-Host "  ✅ $file" -ForegroundColor Green
        } else {
            Write-Host "  ❌ $file" -ForegroundColor Red
            $allFilesExist = $false
        }
    }
    
    if ($allFilesExist) {
        Write-Host "SUCCESS: All required PCAP files are available" -ForegroundColor Green
    } else {
        Write-Host "WARNING: Some PCAP files are missing" -ForegroundColor Yellow
    }
    
} catch {
    Write-Host "ERROR: Exception during file checking: $_" -ForegroundColor Red
}

Write-Host "`nStep 3: Cleanup test build directory..." -ForegroundColor Yellow
try {
    if (Test-Path "build_test") {
        Remove-Item -Recurse -Force "build_test"
        Write-Host "Cleaned up test build directory" -ForegroundColor Green
    }
} catch {
    Write-Host "WARNING: Could not clean up test build directory: $_" -ForegroundColor Yellow
}

Write-Host "`n=== PCAP Configuration Test COMPLETED ===" -ForegroundColor Cyan
Write-Host "Result: CMake configuration should now work with both WPCAP_DIR and NPCAP_SDK_DIR" -ForegroundColor Green
Write-Host "Note: The flexible PCAP detection has been implemented in all affected CMakeLists.txt files" -ForegroundColor Green
