# ASIO AVB Audio Listener

Professional audio listener for AVB/TSN streams using ASIO drivers on Windows.

## Overview

This example demonstrates how to receive AVB (Audio Video Bridging) audio streams and play them through professional ASIO audio interfaces. ASIO provides ultra-low latency audio processing, making it ideal for real-time professional audio applications.

## Features

- **Professional ASIO Audio**: Ultra-low latency audio output
- **AVB Stream Reception**: IEEE 1722 audio stream processing  
- **Multi-channel Support**: Up to 8 audio channels
- **Automatic Driver Detection**: Finds available ASIO drivers
- **Network Interface Selection**: Choose specific network adapter
- **Real-time Processing**: Sample-accurate audio timing

## Requirements

### Software Dependencies
1. **ASIO SDK 2.3+** - Download from Steinberg
   - https://www.steinberg.net/developers/
2. **WinPcap/Npcap SDK** - For network packet capture
   - Npcap SDK: https://npcap.com/dist/npcap-sdk-1.13.zip (recommended)
   - WinPcap SDK: https://www.winpcap.org/install/bin/WpdPack_4_1_2.zip (legacy)
3. **Visual Studio Build Tools** - For compilation

### Hardware Requirements
- **Professional Audio Interface** with ASIO drivers
- **AVB-capable Network Adapter** (Intel I210/I219/I225/I226 recommended)
- **AVB Network Infrastructure** (AVB switches, gPTP synchronization)

## Popular ASIO Drivers

### Professional Audio Interfaces
- **RME**: Fireface, Babyface series
- **Focusrite**: Scarlett, Clarett series  
- **PreSonus**: AudioBox, Studio series
- **Steinberg**: UR series
- **MOTU**: M series

### Universal ASIO Drivers
- **ASIO4ALL**: Universal ASIO driver for any audio device
- **FL Studio ASIO**: Included with FL Studio
- **Reaper**: ReaRoute ASIO

## Building

### 1. Install Prerequisites
```cmd
# Download and extract ASIO SDK to C:\ASIOSDK2.3
# Download and extract Npcap SDK to C:\npcap-sdk

# Set environment variables (optional)
set ASIO_SDK_DIR=C:\ASIOSDK2.3
set PCAP_DIR=C:\npcap-sdk
```

### 2. Build
```cmd
# Using Visual Studio Developer Command Prompt
cd examples\asio-listener
nmake

# Or using the Makefile directly
make
```

### 3. Verify Build
```cmd
asio_listener.exe
# Should show usage information
```

## Usage

### Basic Usage
```cmd
asio_listener.exe "Intel I219" "ASIO4ALL v2"
```

### Find Network Interfaces
```cmd
# Run without parameters to see available interfaces
asio_listener.exe "?"
```

### Find ASIO Drivers
```cmd
# Run with invalid driver to see available drivers
asio_listener.exe "Intel I219" "invalid_driver"
```

### Example with Different Interfaces
```cmd
# Using specific Intel adapter
asio_listener.exe "Intel(R) Ethernet Controller I219-LM"

# Using ASIO4ALL (universal driver)
asio_listener.exe "Realtek PCIe GBE" "ASIO4ALL v2"

# Using professional audio interface
asio_listener.exe "Intel I219" "Focusrite USB ASIO"
```

## Configuration

### Stream ID Configuration
Edit the stream ID in `asio_listener.c`:
```c
static char g_stream_id[17] = "0123456789ABCDEF"; // Change this
```

### Audio Parameters
Modify audio configuration:
```c
#define MAX_CHANNELS 8
#define DEFAULT_SAMPLE_RATE 48000
#define DEFAULT_BUFFER_SIZE 256
```

## AVB Stream Format

The listener expects AVB audio streams conforming to:
- **IEEE 1722** (AVTP) protocol
- **IEC 61883-6** audio format (typically)
- **Sample rates**: 44.1kHz, 48kHz, 88.2kHz, 96kHz
- **Bit depths**: 16, 24, 32 bit
- **Channels**: 1-8 channels

## Integration with OpenAvnu

This listener integrates with the OpenAvnu ecosystem:

1. **gPTP Synchronization**: Requires `gptp` daemon for timing
2. **SRP Stream Reservation**: Use `mrpd` for stream registration
3. **AVB Talker**: Pair with OpenAvnu talker examples
4. **Intel HAL**: Leverages Intel Ethernet HAL for adapter control

### Complete Setup Example
```cmd
# 1. Start gPTP daemon
gptp.exe

# 2. Start MRP daemon  
mrpd.exe

# 3. Configure stream reservation (if using SRP)
# ... SRP configuration ...

# 4. Start ASIO listener
asio_listener.exe "Intel I219" "ASIO4ALL v2"
```

## Troubleshooting

### Common Issues

**"No ASIO drivers found"**
- Install ASIO4ALL or professional audio interface drivers
- Check driver is properly installed and functioning

**"Interface not found"**
- Use exact interface name from Device Manager
- Run as Administrator if needed

**"pcap_open_live failed"**
- Install Npcap or WinPcap
- Check network interface is enabled
- Verify permissions (may need Administrator)

**"No audio output"**
- Verify ASIO driver is working (test with audio software)
- Check audio routing in ASIO control panel
- Ensure AVB stream is being transmitted

**Build Errors**
- Verify ASIO SDK path is correct
- Verify WinPcap/Npcap SDK path is correct
- Use Visual Studio Developer Command Prompt

### Debug Steps

1. **Test ASIO Driver**:
   ```cmd
   # Test with audio application (e.g., Audacity, Reaper)
   ```

2. **Verify Network Capture**:
   ```cmd
   # Use Wireshark to verify AVB packets are received
   # Filter: eth.type == 0x22f0
   ```

3. **Check gPTP Synchronization**:
   ```cmd
   # Verify gPTP is running and synchronized
   gptp.exe -f gptp_config.ini
   ```

## Future Enhancements

Potential improvements for production use:

1. **Advanced Stream Parsing**: Complete IEEE 1722 packet parsing
2. **Multiple Stream Support**: Handle multiple concurrent streams  
3. **Dynamic Configuration**: Runtime parameter adjustment
4. **Synchronization**: Integrate gPTP timing with ASIO callbacks
5. **Stream Quality**: Error detection and correction
6. **GUI Interface**: Windows application with stream monitoring
7. **Plugin Architecture**: VST/AU plugin for DAW integration

## Related Examples

- `examples/jackd-listener/` - JACK audio listener (Linux/macOS)
- `examples/alsa_timed_audio/` - ALSA timed audio (Linux)
- `examples/simple_listener/` - Basic AVB packet listener

## Professional Audio Resources

- **ASIO Documentation**: https://steinberg.help/developer/
- **AVB Overview**: IEEE 802.1BA Audio Video Bridging
- **OpenAvnu Documentation**: https://github.com/AVnu/OpenAvnu
