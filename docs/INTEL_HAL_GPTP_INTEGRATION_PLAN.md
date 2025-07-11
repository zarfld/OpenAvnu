# Intel HAL Integration Plan for gPTP Windows Implementation

## Problem Statement

Current Windows gPTP implementation relies on Intel OIDs (OID_INTEL_GET_SYSTIM, OID_INTEL_GET_RXSTAMP, OID_INTEL_GET_TXSTAMP) which fail on modern Windows drivers. The repository already contains a comprehensive Intel HAL solution that should replace the OID-based approach.

## Solution Overview

Replace OID-based timestamping with Intel HAL API calls in the Windows gPTP daemon. The Intel HAL provides a unified interface for Intel I210/I219/I225/I226 adapters and abstracts away platform-specific implementation details.

## Implementation Details

### 1. **Primary Integration Point: HWTimestamper_gettime()**

**Current Code Location**: `thirdparty/gptp/windows/daemon_cl/windows_hal.hpp:1215`

**Current Implementation:**
```cpp
// Check if we should skip the SYSTIM OID due to repeated failures
if (oid_failures.systim.disabled) {
    // Use enhanced software timestamping as fallback
    goto use_enhanced_software_timestamping;
}

if(( result = readOID( OID_INTEL_GET_SYSTIM, buf, sizeof(buf), &returned )) != ERROR_SUCCESS ) {
    // OID failure handling...
    goto use_enhanced_software_timestamping;
}

// Extract timestamp from OID buffer
now_net = (((uint64_t)buf[1]) << 32) | buf[0];
now_net = scaleNativeClockToNanoseconds( now_net );
*device_time = nanoseconds64ToTimestamp( now_net );
```

**Proposed Intel HAL Integration:**
```cpp
#ifdef OPENAVNU_BUILD_INTEL_HAL
    // Try Intel HAL first for hardware timestamping
    if (intel_hal_device && intel_hal_initialized) {
        intel_timestamp_t hal_timestamp;
        intel_hal_result_t hal_result = intel_hal_read_timestamp(intel_hal_device, &hal_timestamp);
        
        if (hal_result == INTEL_HAL_SUCCESS) {
            // Convert Intel HAL timestamp to gPTP timestamp
            uint64_t timestamp_ns = (hal_timestamp.seconds * 1000000000ULL) + hal_timestamp.nanoseconds;
            *device_time = nanoseconds64ToTimestamp(timestamp_ns);
            
            // Get system time for correlation
            LARGE_INTEGER qpc_before, qpc_after;
            QueryPerformanceCounter(&qpc_before);
            
            FILETIME ft;
            if (WindowsTimestampUtils::isSystemTimePreciseAvailable()) {
                GetSystemTimePreciseAsFileTime(&ft);
            } else {
                GetSystemTimeAsFileTime(&ft);
            }
            
            QueryPerformanceCounter(&qpc_after);
            *system_time = fileTimeToTimestamp(ft);
            
            // Calculate correlation quality and clock values
            uint64_t qpc_delta = qpc_after.QuadPart - qpc_before.QuadPart;
            *local_clock = (uint32_t)(qpc_before.QuadPart & 0xFFFFFFFF);
            *nominal_clock_rate = (uint32_t)(tsc_hz.QuadPart & 0xFFFFFFFF);
            
            GPTP_LOG_DEBUG("Intel HAL timestamp: %llu.%09u seconds", 
                           hal_timestamp.seconds, hal_timestamp.nanoseconds);
            return true;
        } else {
            GPTP_LOG_WARNING("Intel HAL timestamp read failed: %d", hal_result);
            // Fall through to OID method for backward compatibility
        }
    }
#endif

    // Legacy OID-based approach (fallback)
    // Check if we should skip the SYSTIM OID due to repeated failures
    if (oid_failures.systim.disabled) {
        goto use_enhanced_software_timestamping;
    }
    
    // ... existing OID code ...
```

### 2. **Intel HAL Device Initialization**

**Location**: `thirdparty/gptp/windows/daemon_cl/windows_hal.cpp:408` (already started)

**Additional Code Needed:**
```cpp
// Add Intel HAL device handle as class member
class WindowsEtherTimestamper : public EtherTimestamper {
private:
    intel_device_t *intel_hal_device;
    bool intel_hal_initialized;
    // ... existing members ...
    
public:
    WindowsEtherTimestamper() : intel_hal_device(nullptr), intel_hal_initialized(false) {}
    ~WindowsEtherTimestamper() {
        if (intel_hal_device) {
            intel_hal_close_device(intel_hal_device);
        }
    }
};

// In HWTimestamper_init():
#ifdef OPENAVNU_BUILD_INTEL_HAL
    // Initialize Intel HAL
    if (intel_hal_init() == INTEL_HAL_SUCCESS) {
        // Try to open Intel device by adapter description
        char device_id_str[16];
        if (extractIntelDeviceID(adapter_desc, device_id_str, sizeof(device_id_str))) {
            if (intel_hal_open_device(device_id_str, &intel_hal_device) == INTEL_HAL_SUCCESS) {
                intel_hal_initialized = true;
                GPTP_LOG_STATUS("Intel HAL device opened successfully: %s", device_id_str);
                
                // Enable timestamping
                if (intel_hal_enable_timestamping(intel_hal_device, true) == INTEL_HAL_SUCCESS) {
                    GPTP_LOG_STATUS("Intel HAL timestamping enabled");
                } else {
                    GPTP_LOG_WARNING("Failed to enable Intel HAL timestamping");
                }
            } else {
                GPTP_LOG_INFO("Intel HAL device open failed, using OID fallback");
            }
        }
    }
#endif
```

### 3. **TX and RX Timestamp Integration**

**TX Timestamping** (`HWTimestamper_txtimestamp`):
```cpp
#ifdef OPENAVNU_BUILD_INTEL_HAL
    if (intel_hal_device && intel_hal_initialized) {
        intel_timestamp_t hal_timestamp;
        if (intel_hal_read_timestamp(intel_hal_device, &hal_timestamp) == INTEL_HAL_SUCCESS) {
            uint64_t timestamp_ns = (hal_timestamp.seconds * 1000000000ULL) + hal_timestamp.nanoseconds;
            timestamp = nanoseconds64ToTimestamp(timestamp_ns);
            timestamp._version = version;
            return GPTP_EC_SUCCESS;
        }
    }
#endif
    // Fallback to existing OID method...
```

**RX Timestamping** (`HWTimestamper_rxtimestamp`):
Similar pattern as TX timestamping.

### 4. **Device ID Extraction Helper Function**

```cpp
bool extractIntelDeviceID(const char* adapter_desc, char* device_id_str, size_t str_size) {
    // Extract device ID from adapter description
    // Examples:
    // "Intel I219-LM Gigabit Network Connection" -> "0x15BC"
    // "Intel I210 Gigabit Network Connection" -> "0x1533"
    
    if (strstr(adapter_desc, "I219")) {
        strncpy_s(device_id_str, str_size, "0x15BC", _TRUNCATE); // I219-LM
        return true;
    } else if (strstr(adapter_desc, "I210")) {
        strncpy_s(device_id_str, str_size, "0x1533", _TRUNCATE); // I210
        return true;
    } else if (strstr(adapter_desc, "I225")) {
        strncpy_s(device_id_str, str_size, "0x15F2", _TRUNCATE); // I225-LM
        return true;
    } else if (strstr(adapter_desc, "I226")) {
        strncpy_s(device_id_str, str_size, "0x125B", _TRUNCATE); // I226-LM
        return true;
    }
    
    return false;
}
```

## Implementation Steps

1. **Phase 1**: Add Intel HAL initialization to WindowsEtherTimestamper class
2. **Phase 2**: Integrate HAL calls in HWTimestamper_gettime() method  
3. **Phase 3**: Add HAL support to TX and RX timestamping methods
4. **Phase 4**: Test with supported Intel adapters (I219, I210, I225, I226)
5. **Phase 5**: Add error handling and graceful fallback to OID methods

## Benefits

- **Immediate**: Fix failing hardware timestamping on Windows
- **Reliability**: HAL provides direct hardware register access, bypassing driver limitations
- **Future-proof**: Unified interface that works across different Windows versions
- **Cross-platform**: Same API works on Windows and Linux
- **Performance**: Direct hardware access reduces latency compared to OID calls

## Backward Compatibility

The implementation maintains full backward compatibility by:
- Keeping existing OID-based code as fallback
- Using conditional compilation (`#ifdef OPENAVNU_BUILD_INTEL_HAL`)
- Graceful degradation when HAL is not available
- No changes to external gPTP interfaces

## Testing Strategy

1. **Unit Testing**: Test HAL integration with mock devices
2. **Hardware Testing**: Test on physical I219, I210, I225, I226 adapters
3. **Fallback Testing**: Verify OID fallback works when HAL unavailable
4. **Performance Testing**: Compare HAL vs OID timestamp precision
5. **Integration Testing**: Full gPTP daemon testing with HAL enabled

## Files to Modify

1. `thirdparty/gptp/windows/daemon_cl/windows_hal.hpp` - Add HAL device members
2. `thirdparty/gptp/windows/daemon_cl/windows_hal.cpp` - HAL initialization and integration
3. `CMakeLists.txt` - Ensure Intel HAL linking when `OPENAVNU_BUILD_INTEL_HAL=ON`
4. Documentation updates in `docs/TODO.md`

## Next Actions

1. **Implement HAL device initialization** in WindowsEtherTimestamper constructor
2. **Add HAL timestamping to HWTimestamper_gettime()** method
3. **Test HAL integration** on available Intel hardware
4. **Validate timestamping precision** compared to OID approach
5. **Update TODO.md** with progress and remaining tasks
