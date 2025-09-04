#ifndef INTEL_HAL_COMPATIBILITY_H
#define INTEL_HAL_COMPATIBILITY_H

/**
 * Intel HAL Compatibility Layer for OpenAvnu Test Framework
 * 
 * This header provides stub definitions for Intel HAL types and constants
 * used by the Intel TSN integration tests. These are test framework stubs only.
 * 
 * NOTE: This header completely avoids socket-related includes to prevent
 *       Windows socket header conflicts.
 */

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Intel HAL Result Type
typedef enum {
    INTEL_HAL_SUCCESS = 0,
    INTEL_HAL_ERROR = -1,
    INTEL_HAL_INVALID_PARAM = -2,
    INTEL_HAL_NOT_SUPPORTED = -3,
    INTEL_HAL_DEVICE_NOT_FOUND = -4
} intel_hal_result_t;

// Intel Device Family Constants
#define INTEL_DEVICE_FAMILY_I210    0x10
#define INTEL_DEVICE_FAMILY_I226    0x20
#define INTEL_DEVICE_FAMILY_I225    0x21

// Intel Capability Constants
#define INTEL_CAP_TSN_TIME_AWARE_SHAPER     0x01
#define INTEL_CAP_TSN_FRAME_PREEMPTION      0x02
#define INTEL_CAP_BASIC_IEEE1588            0x04
#define INTEL_CAP_ENHANCED_TIMESTAMPING     0x08

// Intel Device Structure
typedef struct intel_device {
    char name[64];
    uint32_t device_id;
    void* private_data;
} intel_device_t;

// Intel Device Info Structure
typedef struct {
    char name[64];
    uint32_t device_id;
    uint16_t vendor_id;
    uint32_t family;
    uint32_t capabilities;
    char description[256];
} intel_device_info_t;

// Intel Time Aware Shaper Configuration
typedef struct {
    uint64_t base_time;          // Base time (original naming for basic usage)
    uint64_t base_time_ns;       // Base time in nanoseconds (extended naming)
    uint64_t cycle_time;         // Cycle time (original naming)
    uint64_t cycle_time_ns;      // Cycle time in nanoseconds (extended naming)
    uint64_t extension_time_ns;  // Extension time in nanoseconds
    uint32_t gate_control_list_length;
    uint32_t num_entries;        // Number of entries (alternative naming)
    struct {
        uint32_t time_interval;     // Time interval (original naming)
        uint32_t time_interval_ns;  // Time interval in nanoseconds (extended)
        uint8_t gate_states;        // Gate states (original naming)
        uint8_t operation;          // Operation code (extended naming)
        uint8_t gate_mask;          // Gate mask (extended naming)
    } gate_control_list[8];
    struct {
        uint32_t time_interval_ns;  // Time interval in nanoseconds
        uint8_t operation;          // Operation code
        uint8_t gate_mask;          // Gate mask
    } entries[8];               // Alternative structure naming
} intel_tas_config_t;

// Intel Frame Preemption Configuration
typedef struct {
    uint8_t preemptible_queues;      // Original naming
    uint8_t preemptable_queues;      // Alternative naming
    uint8_t express_queues;          // Express queues
    uint16_t additional_fragment_size;
    bool verify_disable;             // Original naming
    bool verify_enabled;             // Alternative naming
    uint32_t verify_time;            // Original naming
    uint32_t hold_advance_ns;        // Hold advance in nanoseconds
    uint32_t release_advance_ns;     // Release advance in nanoseconds
} intel_frame_preemption_config_t;

// Intel Timed Packet Structure
typedef struct {
    const uint8_t* packet_data;     // Original pointer-based interface
    size_t packet_length;           // Original length field
    uint64_t launch_time;           // Original launch time
    uint8_t queue;                  // Original queue field
    
    // Extended interface for comprehensive testing
    uint8_t data[1518];             // Inline data buffer for comprehensive tests
    size_t length;                  // Alternative length field
    uint8_t priority;               // Priority field for comprehensive tests
    uint64_t launch_time_ns;        // Launch time in nanoseconds for comprehensive tests
} intel_timed_packet_t;

// Intel HAL Function Stubs for Test Framework
// These are stub implementations for testing only

/**
 * Setup Time Aware Shaper (Test Stub)
 * @param device Intel device handle or interface name
 * @param config TAS configuration
 * @return 0 on success, negative on error
 */
static inline int intel_hal_setup_time_aware_shaper(const void* device, 
                                                   const intel_tas_config_t* config) {
    // Test stub - always succeeds
    (void)device;
    (void)config;
    return 0;
}

/**
 * Get Time Aware Shaper Status (Test Stub)
 * @param device Intel device handle
 * @param enabled Output parameter for enabled status (optional)
 * @param current_time Output parameter for current time (optional)
 * @return 1 if active, 0 if inactive, negative on error
 */
static inline int intel_hal_get_tas_status(const void* device, bool* enabled, uint64_t* current_time) {
    // Test stub - always reports active
    (void)device;
    if (enabled) *enabled = true;
    if (current_time) *current_time = 0;
    return 1;
}

/**
 * Setup Frame Preemption (Test Stub)
 * @param device Intel device handle or interface name
 * @param config Frame preemption configuration
 * @return 0 on success, negative on error
 */
static inline int intel_hal_setup_frame_preemption(const void* device,
                                                  const intel_frame_preemption_config_t* config) {
    // Test stub - always succeeds
    (void)device;
    (void)config;
    return 0;
}

/**
 * Get Frame Preemption Status (Test Stub)
 * @param device Intel device handle  
 * @param fp_status Output parameter for frame preemption status
 * @return INTEL_HAL_SUCCESS on success
 */
static inline intel_hal_result_t intel_hal_get_frame_preemption_status(const void* device, uint8_t* fp_status) {
    // Test stub - always reports active
    (void)device;
    if (fp_status) *fp_status = 0xFF; // All features active
    return INTEL_HAL_SUCCESS;
}

/**
 * Get Frame Preemption Status (Alternative signature for some tests)
 * @param device Intel device handle  
 * @param status_struct Output parameter for status structure
 * @return 1 if active, 0 if inactive, negative on error
 */
static inline int intel_hal_get_frame_preemption_status_alt(const void* device, void* status_struct) {
    // Test stub - always reports active
    (void)device;
    (void)status_struct;
    return 1;
}

/**
 * Transmit Timed Packet (Test Stub)
 * @param device Intel device handle or interface name
 * @param packet Timed packet to transmit
 * @return 0 on success, negative on error
 */
static inline int intel_hal_xmit_timed_packet(const void* device,
                                             const intel_timed_packet_t* packet) {
    // Test stub - always succeeds
    (void)device;
    (void)packet;
    return 0;
}

/**
 * Initialize Intel HAL (Test Stub)
 * @return INTEL_HAL_SUCCESS on success
 */
static inline intel_hal_result_t intel_hal_initialize(void) {
    // Test stub - always succeeds
    return INTEL_HAL_SUCCESS;
}

/**
 * Enumerate Intel Devices (Test Stub)
 * @param devices_out Output array of device info
 * @param count_out Output count of devices
 * @return INTEL_HAL_SUCCESS on success
 */
static inline intel_hal_result_t intel_hal_enumerate_devices(intel_device_info_t** devices_out, uint32_t* count_out) {
    // Test stub - return one simulated I225 device
    static intel_device_info_t test_device = {
        .name = "Intel(R) Ethernet Controller I225-V",
        .device_id = 0x15F3,
        .vendor_id = 0x8086,
        .family = 1,
        .capabilities = INTEL_CAP_TSN_TIME_AWARE_SHAPER | INTEL_CAP_TSN_FRAME_PREEMPTION | INTEL_CAP_BASIC_IEEE1588,
        .description = "Test Intel I225 Device with TSN Support"
    };
    
    static intel_device_info_t* device_array = &test_device;
    
    if (devices_out) *devices_out = device_array;
    if (count_out) *count_out = 1;
    
    return INTEL_HAL_SUCCESS;
}

/**
 * Open Intel Device (Test Stub)
 * @param device_name Device name to open
 * @param device_out Output device handle
 * @return INTEL_HAL_SUCCESS on success
 */
static inline intel_hal_result_t intel_hal_open_device(const char* device_name, intel_device_t** device_out) {
    // Test stub - return a dummy device handle
    static intel_device_t dummy_device;
    (void)device_name;
    
    if (device_out) *device_out = &dummy_device;
    return INTEL_HAL_SUCCESS;
}

/**
 * Close Intel Device (Test Stub)
 * @param device Device handle to close
 * @return INTEL_HAL_SUCCESS on success
 */
static inline intel_hal_result_t intel_hal_close_device(intel_device_t* device) {
    // Test stub - always succeeds
    (void)device;
    return INTEL_HAL_SUCCESS;
}

/**
 * Cleanup Intel HAL (Test Stub)
 * @return INTEL_HAL_SUCCESS on success
 */
static inline intel_hal_result_t intel_hal_cleanup(void) {
    // Test stub - always succeeds
    return INTEL_HAL_SUCCESS;
}

#ifdef __cplusplus
}
#endif

#endif // INTEL_HAL_COMPATIBILITY_H
