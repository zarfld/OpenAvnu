# Enhanced ASIO AVB Audio Listener

Professional audio streaming listener for OpenAvnu with full ASIO SDK integration.

## Features

- **Real ASIO SDK Integration**: Works with actual ASIO drivers when ASIO SDK is available
- **Professional Audio Quality**: Support for multiple audio formats (16-bit, 24-bit, 32-bit, float)
- **Multi-channel Audio**: Up to 8 channels of audio streaming
- **Ring Buffer Architecture**: Efficient audio buffering with overflow/underrun detection
- **AVB/TSN Protocol**: Full IEEE 1722 audio streaming support
- **Automatic Fallback**: Uses stub implementation when ASIO SDK is not available
- **Network Interface Detection**: Automatic detection of AVB-capable network interfaces

## Prerequisites

### Required Dependencies

1. **ASIO SDK** (for professional audio drivers)
   - Download from: https://www.steinberg.net/developers/
   - Set environment variable: `ASIOSDK_DIR=C:\path\to\asiosdk`
   - Supported versions: ASIO SDK 2.3 or later

2. **Network Packet Capture Library**
   - **Npcap SDK** (recommended for Windows 10/11): https://npcap.com/dist/npcap-sdk-1.13.zip
   - **WinPcap Developer Pack** (legacy): https://www.winpcap.org/install/bin/WpdPack_4_1_2.zip

3. **Visual Studio Build Tools**
   - Visual Studio 2019/2022 (Community, Professional, or Enterprise)
   - Or Visual Studio Build Tools 2019/2022

### ASIO Driver Setup

For testing, you can use:
- **ASIO4ALL**: Universal ASIO driver (http://www.asio4all.org/)
- **Professional Audio Interface**: If you have a professional audio interface with ASIO drivers
- **Built-in Audio**: Some motherboards support ASIO through ASIO4ALL

## Environment Setup

### 1. Set ASIO SDK Environment Variable

```powershell
# Set the ASIOSDK_DIR environment variable
$env:ASIOSDK_DIR = "C:\ASIOSDK2.3"

# Make it permanent (requires restart)
[Environment]::SetEnvironmentVariable("ASIOSDK_DIR", "C:\ASIOSDK2.3", "User")
```

### 2. Verify Setup

```powershell
# Check if ASIO SDK is detected
echo $env:ASIOSDK_DIR
dir "$env:ASIOSDK_DIR\common\asio.h"  # Should exist
```

## Building

### Automatic Build (Recommended)

```batch
# Build with full ASIO SDK support
build.bat

# Or build with stub implementation only
build_simple.bat
```

### Manual Build with Visual Studio

```batch
# Set up Visual Studio environment
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

# Compile with ASIO SDK
cl /D_CRT_SECURE_NO_WARNINGS /DWIN32_LEAN_AND_MEAN ^
   /I. /I"%ASIOSDK_DIR%\common" /I"%ASIOSDK_DIR%\host" /I"%ASIOSDK_DIR%\host\pc" ^
   /I"C:\npcap-sdk\Include" ^
   /c asio_listener.c

# Compile ASIO SDK sources
cl /D_CRT_SECURE_NO_WARNINGS /DWIN32_LEAN_AND_MEAN ^
   /I"%ASIOSDK_DIR%\common" /I"%ASIOSDK_DIR%\host" /I"%ASIOSDK_DIR%\host\pc" ^
   /c "%ASIOSDK_DIR%\common\asio.cpp" ^
   "%ASIOSDK_DIR%\host\asiodrivers.cpp" ^
   "%ASIOSDK_DIR%\host\pc\asiolist.cpp"

# Link
link asio_listener.obj asio.obj asiodrivers.obj asiolist.obj ^
     ws2_32.lib winmm.lib ole32.lib oleaut32.lib ^
     "C:\npcap-sdk\Lib\x64\wpcap.lib" "C:\npcap-sdk\Lib\x64\Packet.lib" ^
     /out:asio_listener.exe
```

## Usage

### Basic Usage

```batch
# List available network interfaces
asio_listener.exe

# Use specific network interface
asio_listener.exe "Intel I219"

# Use specific network interface and ASIO driver
asio_listener.exe "Intel I219" "ASIO4ALL v2"
```

### Professional Audio Setup

1. **Install ASIO Driver**: Install ASIO4ALL or your audio interface driver
2. **Configure Audio**: Set up your ASIO driver (buffer size, sample rate)
3. **Run Listener**: Start the listener with your preferred driver

```batch
# Professional audio interface
asio_listener.exe "Intel I225" "RME Fireface UCX"

# Universal ASIO driver
asio_listener.exe "Intel I219" "ASIO4ALL v2"
```

### Advanced Configuration

The listener automatically detects:
- Available ASIO drivers
- Network interfaces with AVB capability
- Audio format and channel configuration
- Optimal buffer sizes

## Audio Formats

### Supported Input Formats (AVB Stream)
- **PCM 16-bit**: Standard audio format
- **PCM 24-bit**: Professional audio standard (typical for AVB)
- **PCM 32-bit**: High-resolution audio
- **Float32**: Floating-point audio

### Supported Output Formats (ASIO)
- **ASIOSTInt16LSB**: 16-bit integer (most common)
- **ASIOSTInt24LSB**: 24-bit integer (professional)
- **ASIOSTFloat32LSB**: 32-bit float (high-end)

## Architecture

### Audio Processing Pipeline

```
AVB Network Stream → Packet Capture → AVB Parser → Audio Ring Buffer → ASIO Output
```

### Key Components

1. **Network Capture**: Uses pcap to capture IEEE 1722 packets
2. **AVB Parser**: Extracts audio data from AVB streams
3. **Ring Buffer**: Manages audio timing and buffering
4. **ASIO Interface**: Professional audio output
5. **Format Conversion**: Handles different audio formats

### Performance Characteristics

- **Latency**: Depends on ASIO buffer size (typically 2-10ms)
- **Channels**: Up to 8 simultaneous audio channels
- **Sample Rates**: 44.1kHz, 48kHz, 96kHz (driver dependent)
- **Bit Depth**: 16-bit, 24-bit, 32-bit support

## Troubleshooting

### Build Issues

**ASIO SDK not found:**
```
ERROR: Invalid ASIO SDK directory
```
- Verify `ASIOSDK_DIR` environment variable
- Check that `asio.h` exists in `%ASIOSDK_DIR%\common\`
- Restart VS Code after setting environment variables

**Compilation errors:**
```
ERROR: Missing ASIO SDK headers
```
- Ensure ASIO SDK is properly extracted
- Check Visual Studio C++ tools are installed
- Verify include paths in build script

### Runtime Issues

**No ASIO drivers found:**
```
ERROR: No ASIO drivers found
```
- Install ASIO4ALL: http://www.asio4all.org/
- Check if your audio interface has ASIO drivers
- Run as administrator if needed

**Network interface not found:**
```
ERROR: Interface 'Intel I219' not found
```
- Run without arguments to list available interfaces
- Use exact interface description as shown
- Check if interface supports packet capture

**Audio dropouts:**
```
Buffer overflow/underrun detected
```
- Increase ASIO buffer size in driver settings
- Check system performance (CPU, disk I/O)
- Reduce other network traffic

### Performance Optimization

1. **ASIO Buffer Size**: Start with 256 samples, adjust based on latency needs
2. **CPU Priority**: Set process priority to "High" for real-time performance
3. **Network Optimization**: Use dedicated network interface for AVB
4. **System Tuning**: Disable power management, enable high-performance mode

## Development

### Code Structure

- `asio_listener.c`: Main application
- `asio_stub.h`: Stub implementation for development
- `pcap_stub.h`: Network stub for testing
- `build.bat`: Automated build script
- `Makefile`: Alternative build system

### Extending the Listener

To add new features:

1. **Audio Effects**: Modify `process_audio_buffers()`
2. **Stream Filtering**: Enhance `process_avb_packet()`
3. **Multiple Streams**: Extend ring buffer architecture
4. **Recording**: Add file output to audio pipeline

### Testing

```batch
# Test with stub implementation
build_simple.bat
asio_listener.exe "any_interface"

# Test with full ASIO
build.bat
asio_listener.exe "Intel I219" "ASIO4ALL v2"
```

## License

Copyright (c) 2025, Intel Corporation. Licensed under MIT License.

## Support

For issues and questions:
1. Check troubleshooting section above
2. Verify environment setup
3. Test with stub implementation first
4. Check OpenAvnu project documentation
