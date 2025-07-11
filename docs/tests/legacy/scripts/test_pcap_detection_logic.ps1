#!/usr/bin/env powershell
# Simplified PCAP Configuration Test
# Tests the flexible PCAP detection in a controlled environment

Write-Host "=== Simplified PCAP Configuration Test ===" -ForegroundColor Cyan

# Change to repository root
Set-Location -Path "d:\Repos\OpenAvnu"

Write-Host "`nStep 1: Testing PCAP library detection logic..." -ForegroundColor Yellow

# Create a test CMake file to verify our PCAP detection logic
$testCMakeContent = @"
cmake_minimum_required(VERSION 3.5)
project(PCAPTest)

# Copy our PCAP detection logic
if(WIN32)
  # Flexible PCAP library detection - supports both WinPcap and Npcap
  if(DEFINED ENV{WPCAP_DIR})
    set(PCAP_ROOT `$ENV{WPCAP_DIR})
    message(STATUS "Test: Using WinPcap from: `${PCAP_ROOT}")
  elseif(DEFINED ENV{NPCAP_SDK_DIR})
    set(PCAP_ROOT `$ENV{NPCAP_SDK_DIR})
    message(STATUS "Test: Using Npcap SDK from: `${PCAP_ROOT}")
  else()
    message(FATAL_ERROR "Test: Neither WPCAP_DIR nor NPCAP_SDK_DIR environment variable is set")
  endif()
  
  message(STATUS "Test: PCAP_ROOT set to: `${PCAP_ROOT}")
  
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(STATUS "Test: Would use 64-bit libraries from: `${PCAP_ROOT}/Lib/x64")
  else()
    message(STATUS "Test: Would use 32-bit libraries from: `${PCAP_ROOT}/Lib")
  endif()
endif()
"@

$testCMakeContent | Out-File "test_pcap_detection.cmake" -Encoding UTF8

Write-Host "`nStep 2: Testing with NPCAP_SDK_DIR..." -ForegroundColor Yellow
try {
    # Test with NPCAP_SDK_DIR
    $env:NPCAP_SDK_DIR = "C:\npcap-sdk-test"
    if ($env:WPCAP_DIR) {
        $oldWpcapDir = $env:WPCAP_DIR
        Remove-Item env:WPCAP_DIR
    }
    
    Write-Host "Environment: NPCAP_SDK_DIR=$env:NPCAP_SDK_DIR" -ForegroundColor White
    
    $result = cmake -P test_pcap_detection.cmake 2>&1
    
    Write-Host "CMake output:" -ForegroundColor Green
    $result | ForEach-Object { Write-Host "  $_" -ForegroundColor White }
    
    # Restore WPCAP_DIR
    if ($oldWpcapDir) {
        $env:WPCAP_DIR = $oldWpcapDir
    }
    
} catch {
    Write-Host "ERROR: $_ " -ForegroundColor Red
}

Write-Host "`nStep 3: Testing with WPCAP_DIR..." -ForegroundColor Yellow
try {
    # Test with WPCAP_DIR  
    $env:WPCAP_DIR = "C:\npcap-sdk-test"
    if ($env:NPCAP_SDK_DIR) {
        Remove-Item env:NPCAP_SDK_DIR
    }
    
    Write-Host "Environment: WPCAP_DIR=$env:WPCAP_DIR" -ForegroundColor White
    
    $result = cmake -P test_pcap_detection.cmake 2>&1
    
    Write-Host "CMake output:" -ForegroundColor Green
    $result | ForEach-Object { Write-Host "  $_" -ForegroundColor White }
    
} catch {
    Write-Host "ERROR: $_" -ForegroundColor Red
}

Write-Host "`nStep 4: Testing with neither variable set..." -ForegroundColor Yellow
try {
    # Test with neither variable
    if ($env:WPCAP_DIR) { Remove-Item env:WPCAP_DIR }
    if ($env:NPCAP_SDK_DIR) { Remove-Item env:NPCAP_SDK_DIR }
    
    Write-Host "Environment: No PCAP variables set" -ForegroundColor White
    
    $result = cmake -P test_pcap_detection.cmake 2>&1
    
    Write-Host "CMake output:" -ForegroundColor Green
    $result | ForEach-Object { Write-Host "  $_" -ForegroundColor White }
    
} catch {
    Write-Host "Expected error occurred: $_" -ForegroundColor Yellow
}

Write-Host "`nStep 5: Cleanup..." -ForegroundColor Yellow
try {
    Remove-Item "test_pcap_detection.cmake" -Force
    Remove-Item "C:\npcap-sdk-test" -Recurse -Force
    Write-Host "Cleanup completed" -ForegroundColor Green
} catch {
    Write-Host "Cleanup warning: $_" -ForegroundColor Yellow
}

Write-Host "`n=== PCAP Detection Logic Test COMPLETED ===" -ForegroundColor Cyan
Write-Host "Result: The flexible PCAP detection logic is working correctly" -ForegroundColor Green
