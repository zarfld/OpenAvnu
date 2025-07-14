# ============================================================================
# Manual ASIO Listener Compile Script (Fallback)
# ============================================================================

Write-Host ""
Write-Host "🔨 Manual ASIO Listener Compilation" -ForegroundColor Yellow
Write-Host "====================================" -ForegroundColor Yellow
Write-Host ""

$buildSuccess = $false

# Check if we have source files
if (Test-Path "simple_asio_listener.c") {
    Write-Host "📁 Found simple_asio_listener.c" -ForegroundColor White
    
    # Try gcc
    Write-Host "🔍 Trying gcc compilation..." -ForegroundColor Gray
    try {
        $gccOutput = & gcc -o simple_asio_listener.exe simple_asio_listener.c -lws2_32 2>&1
        if (Test-Path "simple_asio_listener.exe") {
            Write-Host "✅ gcc compilation successful" -ForegroundColor Green
            Copy-Item "simple_asio_listener.exe" "asio_listener.exe" -Force
            Write-Host "✅ Created asio_listener.exe" -ForegroundColor Green
            $buildSuccess = $true
        }
    } catch {
        Write-Host "⚠️  gcc not available or failed" -ForegroundColor Yellow
    }
    
    # Try Visual Studio compiler if gcc failed
    if (-not $buildSuccess) {
        Write-Host "🔍 Trying Visual Studio cl.exe..." -ForegroundColor Gray
        try {
            $clOutput = & cl /Fe:simple_asio_listener.exe simple_asio_listener.c ws2_32.lib 2>&1
            if (Test-Path "simple_asio_listener.exe") {
                Write-Host "✅ Visual Studio compilation successful" -ForegroundColor Green
                Copy-Item "simple_asio_listener.exe" "asio_listener.exe" -Force
                Write-Host "✅ Created asio_listener.exe" -ForegroundColor Green
                $buildSuccess = $true
            }
        } catch {
            Write-Host "⚠️  Visual Studio compiler not available or failed" -ForegroundColor Yellow
        }
    }
}

if (-not $buildSuccess -and (Test-Path "asio_listener.c")) {
    Write-Host "📁 Found asio_listener.c" -ForegroundColor White
    
    # Try gcc
    Write-Host "🔍 Trying gcc compilation..." -ForegroundColor Gray
    try {
        $gccOutput = & gcc -o asio_listener.exe asio_listener.c -lws2_32 2>&1
        if (Test-Path "asio_listener.exe") {
            Write-Host "✅ gcc compilation successful" -ForegroundColor Green
            $buildSuccess = $true
        }
    } catch {
        Write-Host "⚠️  gcc not available or failed" -ForegroundColor Yellow
    }
    
    # Try Visual Studio compiler if gcc failed
    if (-not $buildSuccess) {
        Write-Host "🔍 Trying Visual Studio cl.exe..." -ForegroundColor Gray
        try {
            $clOutput = & cl /Fe:asio_listener.exe asio_listener.c ws2_32.lib 2>&1
            if (Test-Path "asio_listener.exe") {
                Write-Host "✅ Visual Studio compilation successful" -ForegroundColor Green
                $buildSuccess = $true
            }
        } catch {
            Write-Host "⚠️  Visual Studio compiler not available or failed" -ForegroundColor Yellow
        }
    }
}

if ($buildSuccess) {
    Write-Host ""
    Write-Host "🎉 Manual compilation successful!" -ForegroundColor Green
    if (Test-Path "asio_listener.exe") {
        $fileInfo = Get-Item "asio_listener.exe"
        Write-Host "   Executable: asio_listener.exe" -ForegroundColor Gray
        Write-Host "   Size: $([math]::Round($fileInfo.Length/1KB,2)) KB" -ForegroundColor Gray
        Write-Host "   Modified: $($fileInfo.LastWriteTime)" -ForegroundColor Gray
    }
} else {
    Write-Host ""
    Write-Host "❌ Manual compilation failed" -ForegroundColor Red
    Write-Host "💡 Install MinGW-w64 or Visual Studio Build Tools" -ForegroundColor Yellow
    Write-Host "   MinGW-w64: https://www.mingw-w64.org/downloads/" -ForegroundColor Blue
    Write-Host "   VS Build Tools: https://visualstudio.microsoft.com/downloads/" -ForegroundColor Blue
}

Write-Host ""
