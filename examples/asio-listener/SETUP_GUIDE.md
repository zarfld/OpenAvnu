# ASIO Audio Listener - Setup Guide

This guide will help you set up the development environment to build the ASIO Audio Listener for OpenAvnu.

## Current Status

✅ Source code created: `asio_listener.c`  
✅ Build scripts created: `build.bat`, `build_simple.bat`  
✅ Stub implementations: `asio_stub.h`, `pcap_stub.h`  
❌ C compiler not detected  
❌ ASIO SDK not installed  
❌ PCAP SDK not installed  

## Quick Development Setup (Recommended)

### Option 1: Visual Studio Community (Free & Complete)
1. **Download Visual Studio Community 2022**
   - URL: https://visualstudio.microsoft.com/vs/community/
   - Free for individual developers, students, and open source projects

2. **During installation, select:**
   - ✅ Desktop development with C++ workload
   - ✅ Windows 10/11 SDK (latest version)
   - ✅ CMake tools for C++
   - ✅ Git for Windows

3. **After installation:**
   ```cmd
   # Open "Developer Command Prompt for VS 2022"
   cd D:\Repos\OpenAvnu\examples\asio-listener
   .\build.bat
   ```

### Option 2: Visual Studio Build Tools (Minimal)
1. **Download Build Tools for Visual Studio 2022**
   - URL: https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022

2. **Install with C++ build tools**
   - Select "C++ build tools" workload
   - Include Windows SDK

3. **Build from Developer Command Prompt**

### Option 3: Windows SDK Only (Command Line)
1. **Download Windows SDK**
   - URL: https://developer.microsoft.com/en-us/windows/downloads/windows-sdk/

2. **Install with:**
   - ✅ Windows SDK for desktop C++ apps
   - ✅ MSBuild tools

## ASIO SDK Setup

### Download ASIO SDK
1. **Register at Steinberg Developer Portal**
   - URL: https://www.steinberg.net/developers/

2. **Download ASIO SDK 2.3 or later**
   - Extract to: `C:\ASIOSDK2.3`
   - Or set environment variable: `ASIO_SDK_DIR=C:\your\path`

### Alternative: ASIO4ALL (For Testing)
1. **Download ASIO4ALL**
   - URL: http://www.asio4all.org/
   - Universal ASIO driver for any Windows audio device

2. **Install ASIO4ALL**
   - Use for testing even without professional audio interfaces

## Network Packet Capture Setup

### Option 1: Npcap SDK (Recommended for Windows 10/11)
1. **Download Npcap SDK**
   - URL: https://npcap.com/dist/npcap-sdk-1.13.zip

2. **Install:**
   ```cmd
   # Extract to C:\npcap-sdk
   # Or set environment variable: NPCAP_DIR=C:\your\path
   ```

### Option 2: WinPcap SDK (Legacy)
1. **Download WinPcap Developer Pack**
   - URL: https://www.winpcap.org/install/bin/WpdPack_4_1_2.zip

2. **Install:**
   ```cmd
   # Extract to C:\WpdPack
   # Or set environment variable: WPCAP_DIR=C:\your\path
   ```

## Building the ASIO Listener

### With Full SDK Setup
```cmd
cd D:\Repos\OpenAvnu\examples\asio-listener

# Option 1: Automatic build script
.\build.bat

# Option 2: Manual nmake (from VS Developer Command Prompt)
nmake

# Option 3: Simple build script
.\build_simple.bat
```

### Development Build (Stub Implementation)
```cmd
# This builds with stub implementations for development
# Allows compilation without full SDKs
.\build_simple.bat
```

## Testing the ASIO Listener

### With Stub Implementation (Limited)
```cmd
# Test compilation and basic functionality
asio_listener.exe "any_interface" "stub_driver"
```

### With Real ASIO Driver
```cmd
# List available network interfaces
ipconfig /all

# Run with ASIO4ALL (universal driver)
asio_listener.exe "Intel I219" "ASIO4ALL v2"

# Run with professional audio interface
asio_listener.exe "Intel I219" "RME Fireface"
```

### With AVB Network
```cmd
# Requires:
# - AVB-capable network adapter (Intel I210/I219/I225)
# - AVB network infrastructure
# - gPTP synchronization
# - AVB talker sending audio streams

asio_listener.exe "Intel I225" "RME Fireface UFX"
```

## Troubleshooting

### Build Issues
- **"nmake not found"**: Open Developer Command Prompt for Visual Studio
- **Missing headers**: Install ASIO SDK and PCAP SDK
- **Link errors**: Check library paths in Makefile
- **Architecture mismatch**: Use 64-bit tools for 64-bit libraries

### Runtime Issues
- **"ASIO driver not found"**: Install ASIO4ALL or professional drivers
- **"Network interface not found"**: Check interface name with `ipconfig`
- **"No AVB packets"**: Verify AVB network setup and gPTP synchronization
- **Audio problems**: Check ASIO control panel settings

### ASIO Driver Recommendations

#### Professional Audio Interfaces
- **RME**: Fireface series (excellent ASIO drivers, ultra-low latency)
- **Focusrite**: Scarlett, Clarett series (popular, reliable)
- **PreSonus**: AudioBox, Studio series (good value)
- **Steinberg**: UR series (native ASIO SDK developer)

#### Universal Drivers
- **ASIO4ALL**: Works with any Windows audio device
- **FL Studio ASIO**: If you have FL Studio installed
- **Reaper**: ReaRoute ASIO

## Next Steps

1. **Install Visual Studio Community 2022** (recommended)
2. **Download and install ASIO SDK** from Steinberg
3. **Download and install Npcap SDK** 
4. **Install ASIO4ALL** for testing
5. **Build with:** `.\build.bat`
6. **Test basic functionality**
7. **Set up AVB network infrastructure**
8. **Connect professional ASIO audio interface**
9. **Test with real AVB audio streams**

## Professional AVB Setup

For production use, you'll need:

### Hardware
- **AVB Network Switch** (e.g., Extreme Networks, Motu)
- **AVB Audio Interface** (e.g., Motu AVB series, RME)
- **AVB-capable NIC** (Intel I210/I219/I225/I226)

### Software
- **gPTP daemon** (for time synchronization)
- **SRP daemon** (for stream reservation)
- **AVB talker** (audio source)
- **Professional audio software** (DAW with ASIO support)

### OpenAvnu Integration
The ASIO listener integrates with other OpenAvnu components:
- Uses same network infrastructure as other OpenAvnu tools
- Compatible with OpenAvnu AVTP pipeline
- Can receive streams from OpenAvnu talkers
- Supports standard IEEE 1722 audio formats

---

**Ready to build professional AVB audio applications on Windows!**
