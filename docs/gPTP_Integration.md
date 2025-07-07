# gPTP Integration for OpenAvnu

## Overview

OpenAvnu requires a gPTP (generalized Precision Time Protocol, IEEE 802.1AS) daemon for standards-compliant AVTP operation. This integration provides the [zarfld/gptp](https://github.com/zarfld/gptp) implementation as a submodule to ensure proper time synchronization for AVB/TSN applications.

## Why gPTP is Required

- **Time Synchronization**: AVTP streams require precise time synchronization between devices
- **Standards Compliance**: IEEE 802.1AS (gPTP) is mandatory for AVB/TSN networks
- **Stream Coordination**: Talkers and listeners must operate on synchronized time domains
- **Quality of Service**: Proper timing ensures low latency and jitter-free media streaming

## Building gPTP

### Prerequisites

**Linux:**
- Build tools: `gcc`, `make`, `cmake`
- Development packages: `libc6-dev`

**Windows:**
- Visual Studio 2017 or later with C++ build tools
- CMake 3.10 or later

### Build Process

The gPTP daemon is automatically built when you build OpenAvnu with CMake:

```bash
# Linux/macOS
mkdir build && cd build
cmake ..
make

# Windows (PowerShell)
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

To disable gPTP building (if you have an external gPTP daemon):

```bash
cmake -DOPENAVNU_BUILD_GPTP=OFF ..
```

### Manual Build (Advanced)

If you need to build gPTP separately:

**Linux:**
```bash
cd thirdparty/gptp/linux/build
make clean && make
```

**Windows:**
```bash
# Open thirdparty/gptp/windows/daemon_cl.sln in Visual Studio
# Or use MSBuild:
cd thirdparty/gptp/windows
MSBuild daemon_cl.sln /p:Configuration=Release /p:Platform=x64
```

## Running gPTP

### Linux

Use the provided script (requires sudo for network access):

```bash
sudo ./run_gptp.sh eth0
```

Or run directly:

```bash
sudo thirdparty/gptp/linux/build/obj/daemon_cl eth0
```

### Windows

Use the PowerShell script (run as Administrator):

```powershell
# List available network interfaces
Get-NetAdapter | Select-Object Name, InterfaceDescription

# Run gPTP daemon
.\run_gptp.ps1 -NetworkInterface "Ethernet"
```

Or run directly:

```cmd
thirdparty\gptp\windows\daemon_cl\x64\Release\daemon_cl.exe "Ethernet"
```

### Common Options

- `-G <group>`: Specify multicast group (default: 224.0.1.129)
- `-P <priority1>`: Set clock priority1 (default: 248)
- `-A <priority2>`: Set clock priority2 (default: 248)
- `-L <log_level>`: Set logging level (0=silent, 1=critical, 2=error, 3=warning, 4=info, 5=debug)

Example with options:
```bash
sudo ./run_gptp.sh eth0 -L 4 -P 128
```

## Integration with AVTP Pipeline

The OpenAvnu AVTP pipeline automatically detects and uses gPTP time synchronization through shared memory IPC:

### Linux
- Shared memory segment: `/OpenAvbGrandmasterTime`
- Location: `/lib/common/avb_gptp.c`

### Windows  
- Named memory section: `OpenAvbGrandmasterTime`
- Location: `/lib/common/avb_gptp.c`

No additional configuration is needed - the AVTP pipeline will automatically connect to the running gPTP daemon.

## Verification

To verify gPTP is working correctly:

### Check Daemon Status

**Linux:**
```bash
ps aux | grep daemon_cl
# Should show the running gPTP daemon

# Check shared memory
ls -la /dev/shm/ | grep OpenAvb
# Should show OpenAvbGrandmasterTime
```

**Windows:**
```powershell
Get-Process daemon_cl
# Should show the running gPTP daemon

# Check for named section (requires process monitor or similar tools)
```

### Check AVTP Pipeline Logs

When running AVTP applications, look for log messages indicating successful gPTP connection:

```
INFO: gPTP time synchronization active
INFO: Grandmaster clock detected
```

## Troubleshooting

### Common Issues

1. **Permission Denied (Linux)**
   - Run with `sudo` for network interface access
   - Add user to `ptp` group: `sudo usermod -a -G ptp $USER`

2. **Network Interface Not Found**
   - List interfaces: `ip link show` (Linux) or `Get-NetAdapter` (Windows)
   - Use correct interface name (e.g., `eth0`, `enp0s3`, `Ethernet`)

3. **gPTP Daemon Not Found**
   - Ensure project is built: `make` or `cmake --build .`
   - Check build directory for daemon executable

4. **Windows Build Errors (Configuration Files)**
   - **Error**: `Error copying file (if different) from "D:/Repos/OpenAvnu/gptp_cfg.ini"`
   - **Cause**: gPTP submodule CMakeLists.txt uses wrong path for configuration files
   - **Solution**: See `gptp_windows_build_fix.md` for detailed fix instructions
   - **Quick Fix**: Apply patch or edit `thirdparty/gptp/CMakeLists.txt` to change `${CMAKE_SOURCE_DIR}` to `${CMAKE_CURRENT_SOURCE_DIR}`

5. **Time Sync Not Working**
   - Verify network connectivity between devices
   - Check firewall settings (allow multicast traffic)
   - Ensure devices are on same network segment

### Debug Mode

Enable detailed logging:

**Linux:**
```bash
sudo ./run_gptp.sh eth0 -L 5
```

**Windows:**
```powershell
.\run_gptp.ps1 -NetworkInterface "Ethernet" -AdditionalArgs @("-L", "5")
```

### Log Files

gPTP logs are typically written to:
- Linux: syslog (`/var/log/syslog` or `journalctl`)
- Windows: Event log or console output

## Network Configuration

For gPTP to work properly:

1. **Multicast Support**: Network switches must support multicast
2. **IGMP Snooping**: Should be enabled on managed switches  
3. **Hardware Timestamping**: Recommended for best precision (requires compatible NICs)
4. **Priority Tagging**: 802.1Q VLAN tagging may be required in some setups

## Performance Tuning

For optimal performance:

1. **Real-time Priority**: Run gPTP with high priority
   ```bash
   sudo chrt -f 99 ./run_gptp.sh eth0
   ```

2. **Hardware Timestamping**: Use NICs with hardware timestamp support
3. **Dedicated Network**: Use dedicated network interface for AVB traffic
4. **Interrupt Affinity**: Bind network interrupts to specific CPU cores

## Standards Compliance

This gPTP implementation follows:
- IEEE 802.1AS-2011 (gPTP)
- IEEE 802.1AS-2020 (latest revision)
- Compatible with AVB and TSN standards

## References

- [IEEE 802.1AS Standard](https://standards.ieee.org/standard/802_1AS-2020.html)
- [zarfld/gptp Repository](https://github.com/zarfld/gptp)
- [AVnu Alliance](https://avnu.org/)
- [OpenAvnu AVTP Pipeline Documentation](documents/design/)
