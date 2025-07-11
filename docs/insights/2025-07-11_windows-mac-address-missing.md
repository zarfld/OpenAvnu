# 2025-07-11_windows-mac-address-missing.md

**Context:**
Testing and analysis of the Intel Ethernet HAL on Windows, especially the output of the MAC address in the automated test (`intel_hal_full_test`).

**Observation:**
The logfile shows all adapters with MAC address `00:00:00:00:00:00`. The HAL API does not return real MAC addresses on Windows.

**Root Cause:**
The Windows-specific implementation (`src/windows/intel_ndis.c`) does not read out the MAC address. Only the registry is queried for device information, but no Windows API like `GetAdaptersInfo` or `GetAdaptersAddresses` is used to obtain the real MAC address. The field remains at its default value.

**Resolution:**
- Insight documented.
- To obtain real MAC addresses, the HAL implementation would need to be extended to call the Windows IP Helper API.

**Takeaways:**
- The registry does not provide MAC addresses.
- For hardware tests and logging on Windows, the HAL must be extended if MAC addresses are required.
- Always check if all fields in the interface struct are actually populated.
