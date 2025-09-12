# gPTP Architecture Chain Documentation

## Complete Architecture Overview with Correct Interfaces

This document provides a comprehensive overview of the OpenAvnu gPTP architecture chain, detailing the exact interfaces and responsibilities of each component layer.

## Architecture Chain with Specific APIs

```
gPTP Service Layer (IEEE 802.1AS Protocol Implementation)
    ↓ (intel-ethernet-hal.h API)
intel-ethernet-hal (Hardware Abstraction Layer) 
    ↓ (intel.h API)
intel_avb (Driver Interface Library)
    ↓ (avb_ioctl.h IOCTL-interface)
NDISIntelFilterDriver (Windows Kernel Driver)
    ↓ (register access)
Intel NIC Hardware (I210/I219/I225/I226)
```

## Component Responsibilities

### 1. gPTP Service Layer
**Location**: `thirdparty/gptp/`  
**API Provided**: IEEE 802.1AS protocol services  
**API Consumed**: `intel-ethernet-hal.h`  
**Primary Responsibility**: IEEE 802.1AS-2020 time synchronization protocol implementation

#### Key Functions:
- **Best Master Clock Algorithm (BMCA)**: Grandmaster selection and hierarchy establishment
- **Path Delay Measurement**: Peer-to-peer delay calculation using hardware timestamps
- **Time Synchronization**: Nanosecond-precision clock adjustment and offset calculation
- **Profile Implementation**: Milan (±80ns), Automotive, and Base profile support
- **State Machine Management**: Port states (Master, Slave, Passive, Disabled)

#### Hardware Integration:
```cpp
// ✅ CORRECT: gPTP uses intel-ethernet-hal.h API
#include "intel_ethernet_hal.h"

// Initialize hardware abstraction layer
intel_hal_result_t result = intel_hal_init();

// Get TX timestamp after sending Sync message
intel_hal_timestamp_t tx_timestamp;
result = intel_hal_get_tx_timestamp(device, &tx_timestamp);
if (result == INTEL_HAL_SUCCESS) {
    printf("Sync TX timestamp: %" PRIu64 " ns\n", tx_timestamp.nanoseconds);
    calculateSyncOffset(tx_timestamp);
}
```

### 2. Intel Ethernet HAL (Hardware Abstraction Layer)
**Location**: `thirdparty/intel-ethernet-hal/`  
**API Provided**: `intel-ethernet-hal.h`  
**API Consumed**: `intel.h`  
**Primary Responsibility**: Cross-platform Intel hardware abstraction

#### Key Functions:
- **Device Enumeration**: Discovery and identification of Intel I210/I219/I225/I226 adapters
- **Capability Detection**: Runtime query of TSN features (TAS, CBS, EST, Enhanced Timestamping)
- **Cross-platform Abstraction**: Unified API over Windows NDIS and Linux PTP/PHC
- **Timestamping Operations**: TX/RX hardware timestamp capture and retrieval
- **Thread Safety**: Multi-threaded access support for Service Layer components

#### Interface Translation:
```cpp
// ✅ CORRECT: intel-ethernet-hal uses intel.h API
#include "intel.h"

intel_hal_result_t intel_hal_read_register(intel_hal_device_t *device, 
                                          uint32_t offset, 
                                          uint32_t *value) {
    // Translate to intel.h API
    int result = intel_read_reg(device->intel_device, offset, value);
    
    // Convert error codes
    return (result == 0) ? INTEL_HAL_SUCCESS : INTEL_HAL_ERROR_HARDWARE;
}
```

### 3. Intel AVB Library (Driver Interface Layer)
**Location**: `lib/intel_avb/`  
**API Provided**: `intel.h`  
**API Consumed**: `avb_ioctl.h`  
**Primary Responsibility**: IOCTL interface to Windows NDIS kernel driver

#### Key Functions:
- **IOCTL Translation**: Convert intel.h API calls to avb_ioctl.h IOCTL operations
- **Register Access**: Safe, validated access to Intel controller registers
- **Error Handling**: Convert Windows kernel errors to intel.h API error codes
- **Resource Management**: IOCTL handle and buffer management
- **Security Compliance**: Maintain Windows driver security model

#### IOCTL Implementation:
```cpp
// ✅ CORRECT: intel_avb uses avb_ioctl.h interface
#include "avb_ioctl.h"

int intel_read_reg(device_t *dev, uint32_t offset, uint32_t *value) {
    avb_ioctl_request_t request = {
        .register_offset = offset,
        .access_type = AVB_IOCTL_READ_REG,
        .device_id = dev->device_id
    };
    
    DWORD bytes_returned;
    BOOL result = DeviceIoControl(
        dev->driver_handle,
        IOCTL_AVB_READ_REGISTER,
        &request, sizeof(request),
        &request, sizeof(request),
        &bytes_returned, NULL
    );
    
    if (!result) {
        DWORD error = GetLastError();
        printf("Register read IOCTL failed: offset 0x%" PRIX32 ", error %" PRIu32 "\n",
               offset, error);
        return -1;
    }
    
    *value = request.value;
    return 0;
}
```

### 4. NDIS Intel Filter Driver (Kernel Driver)
**Location**: Windows kernel space  
**API Provided**: `avb_ioctl.h` IOCTL interface  
**API Consumed**: Direct hardware register access  
**Primary Responsibility**: Kernel-mode hardware access and security enforcement

#### Key Functions:
- **Hardware Register Access**: Direct read/write to Intel controller registers
- **Interrupt Handling**: Hardware interrupt processing for timestamping events
- **Security Enforcement**: Windows kernel security model compliance
- **Device Management**: PCI device enumeration and resource allocation
- **Performance Optimization**: Minimal latency hardware access

### 5. Intel NIC Hardware
**Supported Models**: I210, I219, I225, I226  
**Interface**: PCI Express register interface  
**Primary Responsibility**: Physical time-sensitive networking implementation

#### Hardware Features:
- **Precision Timestamping**: Nanosecond-resolution TX/RX timestamp capture
- **IEEE 802.1AS Support**: Hardware-accelerated PTP message processing
- **TSN Features**: Time-Aware Shaper (TAS), Credit-Based Shaper (CBS)
- **Enhanced Timestamping**: I225/I226 advanced precision features
- **Multiple Queues**: Hardware queue management for QoS

## Data Flow Example: PTP Sync Message

### 1. gPTP Service Layer
```cpp
// Send Sync message and request TX timestamp
sendSyncMessage(port, sync_packet);
intel_hal_get_tx_timestamp(device, &tx_timestamp);
```

### 2. Intel Ethernet HAL
```cpp
// Translate to intel.h API
intel_hal_result_t intel_hal_get_tx_timestamp(intel_hal_device_t *device, 
                                             intel_hal_timestamp_t *timestamp) {
    uint64_t hw_timestamp;
    int result = intel_get_systime(device->intel_device, &hw_timestamp);
    timestamp->nanoseconds = hw_timestamp;
    return (result == 0) ? INTEL_HAL_SUCCESS : INTEL_HAL_ERROR_HARDWARE;
}
```

### 3. Intel AVB Library
```cpp
// Convert to IOCTL operation
int intel_get_systime(device_t *dev, uint64_t *systime) {
    avb_ioctl_request_t request = {
        .timestamp_type = AVB_TIMESTAMP_TX,
        .device_id = dev->device_id
    };
    
    BOOL result = DeviceIoControl(dev->driver_handle, IOCTL_AVB_GET_TIMESTAMP,
                                  &request, sizeof(request),
                                  systime, sizeof(*systime),
                                  &bytes_returned, NULL);
    return result ? 0 : -1;
}
```

### 4. NDIS Filter Driver
```cpp
// Kernel-mode register access
NTSTATUS HandleTimestampRequest(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    // Read hardware timestamp register
    UINT64 timestamp = READ_REGISTER_ULONG64(&adapter->timestamp_registers->SYSTIML);
    
    // Return to user mode
    *((UINT64*)Irp->AssociatedIrp.SystemBuffer) = timestamp;
    return STATUS_SUCCESS;
}
```

### 5. Intel Hardware
- Hardware captures precise transmission timestamp
- Stores in dedicated timestamp registers
- Generates interrupt for timestamp availability

## Cross-Component Printf Format Standards

### Portable Format Macros (CRITICAL)
All components must use portable format macros to prevent Windows MSVC assertion dialogs:

```cpp
#include <inttypes.h>

// ✅ CORRECT: Portable format macros
printf("Timestamp: %" PRIu64 " ns\n", timestamp);
printf("Register: 0x%" PRIX32 " = 0x%" PRIX32 "\n", offset, value);
printf("Device ID: 0x%" PRIX32 "\n", device_id);
printf("Handle: 0x%" PRIXPTR "\n", (uintptr_t)handle);

// ❌ WRONG: Platform-specific formats (causes Windows assertions)
printf("Timestamp: %lu ns\n", timestamp);        // Use PRIu64
printf("Register: 0x%x = 0x%x\n", offset, value); // Use PRIX32
printf("Handle: %p\n", handle);                   // Use PRIXPTR
```

## Component Integration Guidelines

### 1. Service Layer (gPTP) Integration
- **API Usage**: Only use intel-ethernet-hal.h API functions
- **Error Handling**: Check intel_hal_result_t return codes
- **Threading**: Intel HAL is thread-safe for concurrent access
- **Initialization**: Call intel_hal_init() before any operations

### 2. Intel HAL Integration  
- **API Implementation**: Implement intel-ethernet-hal.h API functions
- **Backend Calls**: Use intel.h API for hardware access
- **Cross-platform**: Provide identical behavior on Windows and Linux
- **Resource Management**: Handle device open/close lifecycle

### 3. Intel AVB Integration
- **API Implementation**: Implement intel.h API functions
- **IOCTL Operations**: Use avb_ioctl.h for all kernel communication
- **Error Translation**: Convert Windows errors to intel.h error codes
- **Buffer Management**: Efficiently manage IOCTL buffers

### 4. Driver Interface
- **IOCTL Codes**: Use standardized avb_ioctl.h IOCTL definitions
- **Security**: Validate all parameters in kernel mode
- **Performance**: Minimize context switches and latency
- **Error Reporting**: Provide detailed error information

## Architectural Violations to Avoid

### ❌ WRONG: Bypassing Architecture Layers
```cpp
// NEVER: gPTP directly accessing intel_avb
#include "intel_avb_api.h"  // Violates architecture

// NEVER: Intel HAL directly accessing IOCTL
#include "avb_ioctl.h"      // Bypasses intel_avb layer

// NEVER: Direct hardware register access
void* mmio = MapPhysicalMemory(pci_address);  // Security violation
```

### ✅ CORRECT: Proper Layer Separation
```cpp
// gPTP uses only intel-ethernet-hal.h
#include "intel_ethernet_hal.h"

// Intel HAL uses only intel.h
#include "intel.h"

// intel_avb uses only avb_ioctl.h
#include "avb_ioctl.h"
```

## Testing and Validation

### Hardware-in-Loop Testing
All components must be tested with actual Intel hardware:
- **I210**: Basic PTP timestamping validation
- **I219**: Enhanced timestamping accuracy verification
- **I225**: Full TSN feature testing
- **I226**: Latest precision capabilities validation

### Integration Testing
- **Full Chain**: Test complete gPTP → HAL → intel_avb → driver → hardware chain
- **Error Propagation**: Verify error codes propagate correctly through all layers
- **Performance**: Measure latency and throughput at each interface
- **Concurrency**: Test multi-threaded access patterns

### Printf Format Validation
- **Windows MSVC**: Test Debug builds for assertion dialogs
- **Cross-platform**: Verify identical output on Windows and Linux
- **Error Messages**: Ensure all error reporting uses portable formats

## Performance Considerations

### Latency Optimization
- **IOCTL Batching**: Group multiple operations when possible
- **Asynchronous Operations**: Use overlapped I/O for non-blocking access
- **Cache Optimization**: Minimize redundant hardware access
- **Context Switches**: Reduce kernel/user mode transitions

### Memory Management
- **Buffer Reuse**: Efficiently manage IOCTL buffers
- **Handle Caching**: Minimize device open/close operations
- **Memory Pools**: Use pre-allocated buffers for timestamp operations

### Error Recovery
- **Timeout Handling**: Implement robust timeout recovery
- **Device Disconnection**: Handle adapter removal gracefully
- **Driver Restart**: Support driver reload scenarios

This comprehensive architecture documentation ensures proper understanding and implementation of the OpenAvnu gPTP hardware integration chain.