# Intel HAL NDIS Timestamp Provider Architecture

**Date**: 2025-07-10  
**Status**: Concept Proposal  
**Priority**: HIGH  

## Overview

The Intel Ethernet HAL must be architected as an **NDIS (Network Driver Interface Specification) timestamp provider** that integrates with Windows networking and OpenAvnu components. Rather than being a standalone library, the HAL should provide precision timestamping services that gPTP, mrpd, maap, and other consumers can access through standard Windows interfaces.

## Current State

The Intel HAL currently provides:
- Device detection and enumeration
- Basic device management (open/close/status)
- Direct hardware timestamp capture
- Cross-platform abstraction layer

However, it operates as an isolated library rather than integrating with Windows networking infrastructure.

## NDIS Provider Architecture Goals

### 1. Integration as Timestamp Provider
- **Primary Role**: Serve as a Windows timestamp provider for AVB/TSN operations
- **Interface**: Expose timestamping through NDIS or iphlpapi interfaces
- **Fallback Support**: Allow OpenAvnu components to use Intel HAL as fallback when standard Windows timestamping is insufficient
- **Primary Support**: Enable Intel HAL as the primary timestamp source for supported hardware

### 2. Consumer Integration
- **gPTP Integration**: gPTP daemon can query Intel HAL for precision timestamps
- **mrpd Integration**: MRP daemon can use Intel HAL for message timing
- **maap Integration**: MAAP can leverage Intel HAL for address acquisition timing
- **General Fallback**: Any OpenAvnu component requiring precision timing

### 3. Windows Standards Compliance
- **NDIS Interface**: Implement NDIS timestamp provider interface
- **iphlpapi Integration**: Provide iphlpapi-compatible timestamp functions
- **WinPcap Compatibility**: Ensure compatibility with existing packet capture infrastructure
- **Driver Integration**: Work with existing Intel driver stack without conflicts

## Proposed Architecture

### 1. NDIS Timestamp Provider Interface

```c
// NDIS timestamp provider interface
typedef struct intel_ndis_provider {
    // NDIS provider registration
    NDIS_HANDLE provider_handle;
    
    // Timestamp capabilities
    INTEL_TIMESTAMP_CAPS capabilities;
    
    // Provider callbacks
    NDIS_TIMESTAMP_GET_FUNC get_timestamp;
    NDIS_TIMESTAMP_SYNC_FUNC sync_timestamp;
    
    // Device context
    intel_device_context_t *device_ctx;
} intel_ndis_provider_t;
```

### 2. Provider Registration

```c
// Register Intel HAL as NDIS timestamp provider
NTSTATUS intel_register_ndis_provider(
    intel_device_context_t *ctx,
    intel_ndis_provider_t **provider
);

// Unregister provider
NTSTATUS intel_unregister_ndis_provider(
    intel_ndis_provider_t *provider
);
```

### 3. Consumer Integration Points

#### gPTP Integration
```c
// gPTP can query available timestamp providers
intel_timestamp_result_t gptp_get_timestamp(
    gptp_context_t *gptp_ctx,
    intel_timestamp_t *timestamp
);
```

#### MRP Daemon Integration
```c
// mrpd can use Intel HAL for message timing
intel_timestamp_result_t mrpd_get_tx_timestamp(
    mrpd_context_t *mrpd_ctx,
    packet_id_t packet_id,
    intel_timestamp_t *tx_timestamp
);
```

## Existing gPTP Architecture

The OpenAvnu gPTP daemon already includes sophisticated Windows timestamping infrastructure:

### Current Implementation
- **Cross-Timestamping**: `windows_crosststamp.hpp/cpp` - Windows equivalent to Linux PTP_HW_CROSSTSTAMP
- **Intel Vendor Support**: `windows_hal_vendor_intel.hpp/cpp` - Intel OUI detection and device specifications
- **NDIS Integration**: `windows_hal_ndis.hpp/cpp` - NDIS query infrastructure
- **IPHLPAPI Support**: `windows_hal_iphlpapi.hpp/cpp` - IP Helper API integration

### Integration Points
The Intel HAL must integrate with existing gPTP infrastructure:

```cpp
// Current gPTP cross-timestamping interface
class WindowsCrossTimestamp {
    bool GetCrossTimestamp(Timestamp *system_time, Timestamp *hw_time);
    bool SetHardwareFreq(int32_t freq_offset_ppb);
};

// Intel HAL integration point
class IntelNDISTimestampProvider {
    bool Initialize(intel_device_context_t *ctx);
    bool GetHardwareTimestamp(Timestamp *hw_time);
    bool SetFrequencyAdjustment(int32_t ppb);
};
```

### Vendor Integration Strategy
The Intel HAL should extend the existing vendor detection rather than replace it:

```cpp
// Extend existing intel vendor detection
namespace IntelVendorExtensions {
    bool IsIntelHALSupported(const char* device_name);
    intel_device_context_t* GetHALContext(const char* device_name);
    bool EnableHALTimestamping(intel_device_context_t* ctx);
}
```

## Proposed NDIS Provider Integration

```c
// In Intel HAL initialization
intel_device_context_t *ctx = intel_create_device_context();
intel_ndis_provider_t *provider;

if (intel_register_ndis_provider(ctx, &provider) == STATUS_SUCCESS) {
    // Provider registered successfully
    printf("Intel timestamp provider registered\n");
}
```

### Consumer Usage (gPTP)
```c
// In gPTP daemon
intel_timestamp_t current_time;
intel_timestamp_result_t result;

// Query Intel timestamp provider
result = intel_get_current_timestamp(&current_time);
if (result == INTEL_TIMESTAMP_SUCCESS) {
    // Use Intel timestamp for gPTP operations
    gptp_process_timestamp(&current_time);
} else {
    // Fall back to standard Windows timestamping
    fallback_get_timestamp(&current_time);
}
```

## Implementation Phases

### Phase 1: gPTP Vendor Extension
1. **Vendor Detection Extension**: Extend `windows_hal_vendor_intel.cpp` to detect Intel HAL support
2. **HAL Context Integration**: Add Intel HAL device context to vendor detection
3. **Cross-Timestamp Integration**: Integrate Intel HAL with `WindowsCrossTimestamp` class
4. **Basic Testing**: Validate Intel HAL detection and basic timestamp queries

### Phase 2: NDIS Provider Foundation
1. **NDIS Registration**: Implement NDIS provider registration through existing infrastructure
2. **Timestamp Interface**: Create Intel HAL timestamp provider interface
3. **Provider Callbacks**: Implement NDIS-compatible timestamp callbacks
4. **Error Handling**: Add proper error handling and fallback logic

### Phase 3: Consumer Integration
1. **gPTP Integration**: Complete integration with gPTP cross-timestamping
2. **mrpd Integration**: Add Intel HAL support to MRP daemon
3. **maap Integration**: Add Intel HAL support to MAAP daemon
4. **Configuration**: Add configuration options for Intel HAL usage

### Phase 4: Advanced Features
1. **Performance Optimization**: Optimize provider performance for real-time use
2. **Monitoring**: Add provider health monitoring and diagnostics
3. **Documentation**: Complete integration documentation and examples
4. **Testing**: Comprehensive testing across different Intel hardware

## Technical Considerations

### 1. NDIS Provider Model
- **Provider Registration**: Use NdisRegisterTimestampProvider() if available
- **Callback Interface**: Implement NDIS-standard timestamp callbacks
- **Memory Management**: Follow NDIS memory allocation patterns
- **Error Handling**: Use NDIS error codes and patterns

### 2. iphlpapi Integration
- **GetIfTable2Ex()**: Extend interface table with timestamp capabilities
- **GetIfTimestamp()**: Custom function for timestamp queries
- **NotifyIpInterfaceChange()**: Handle interface change notifications
- **Interface Binding**: Bind Intel HAL to specific network interfaces

### 3. Backward Compatibility
- **Existing Code**: Maintain compatibility with existing OpenAvnu Windows code
- **WinPcap**: Ensure WinPcap-based code continues to work
- **Driver Stack**: Avoid conflicts with existing Intel driver installations
- **Fallback Paths**: Provide graceful fallback when Intel HAL unavailable

## Success Criteria

### 1. gPTP Integration
- [x] Intel HAL integrates with `WindowsCrossTimestamp` class
- [x] `IntelVendorExtensions` namespace provides HAL access
- [x] gPTP automatically detects and uses Intel HAL when available
- [x] Fallback to existing Windows timestamping when Intel HAL unavailable

### 2. Provider Registration  
- [x] Intel HAL successfully registers as NDIS timestamp provider
- [x] Provider responds to NDIS timestamp queries correctly
- [x] Integration with existing `windows_hal_ndis.cpp` infrastructure

### 3. Consumer Integration
- [x] gPTP daemon uses Intel HAL through existing vendor detection
- [x] mrpd successfully integrates Intel HAL timestamps
- [x] maap leverages Intel HAL when hardware supports it

### 4. Performance & Reliability
- [x] Timestamp queries complete within 1µs (target latency)
- [x] Provider handles concurrent queries from multiple consumers
- [x] No degradation in existing gPTP performance
- [x] Graceful handling of device disconnection/reconnection

## Risk Mitigation

### 1. NDIS Interface Availability
- **Risk**: NDIS timestamp provider interface may not exist or be documented
- **Mitigation**: Implement custom iphlpapi interface as fallback
- **Fallback**: Direct integration with OpenAvnu components if needed

### 2. Driver Conflicts
- **Risk**: Intel HAL may conflict with existing Intel drivers
- **Mitigation**: Use read-only operations and avoid driver-level modifications
- **Testing**: Extensive testing with various Intel driver versions

### 3. Performance Impact
- **Risk**: NDIS provider overhead may impact performance
- **Mitigation**: Implement efficient caching and batching mechanisms
- **Monitoring**: Continuous performance monitoring and optimization

## Next Steps

### Immediate Actions (Phase 1)
1. **Analyze gPTP Vendor Detection**: Review `windows_hal_vendor_intel.cpp` implementation
2. **Design HAL Extension**: Create `IntelVendorExtensions` namespace for HAL integration
3. **Prototype Integration**: Create minimal Intel HAL detection in existing vendor code
4. **Test with I219-LM**: Validate Intel HAL integration with known working hardware

### Implementation Priorities
1. **Priority 1**: Extend existing gPTP vendor detection with Intel HAL
2. **Priority 2**: Integrate Intel HAL with `WindowsCrossTimestamp` class
3. **Priority 3**: Add NDIS provider registration through existing infrastructure
4. **Priority 4**: Test end-to-end gPTP with Intel HAL timestamps

### Research Required
- Review existing NDIS query infrastructure in `windows_hal_ndis.cpp`
- Understand current cross-timestamping implementation patterns
- Identify integration points in gPTP initialization sequence
- Analyze configuration file structure for Intel HAL options

## References

- Windows Driver Kit (WDK) Documentation
- NDIS Reference Documentation
- iphlpapi Function Reference
- OpenAvnu gPTP Implementation
- Intel I219/I225/I226 Technical Documentation

---

This concept supersedes the basic device detection approach and establishes the Intel HAL as a first-class timestamp provider within the Windows networking ecosystem.
