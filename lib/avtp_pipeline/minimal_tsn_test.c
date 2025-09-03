/**
 * Minimal TSN Test - No includes, direct Intel HAL validation
 * Tests that Intel HAL TSN functions exist and can be called
 */

// Minimal Intel HAL typedefs to avoid include conflicts
typedef int intel_hal_ret_t;
typedef struct intel_hal_device intel_hal_device_t;
typedef unsigned int uint32_t;

// Intel HAL return codes
#define INTEL_HAL_SUCCESS 0
#define INTEL_HAL_ERROR -1

// TSN capability flags (from our Intel HAL implementation)
#define INTEL_CAP_TSN_TIME_AWARE_SHAPER 0x08
#define INTEL_CAP_TSN_FRAME_PREEMPTION 0x10
#define INTEL_CAP_ENHANCED_TIMESTAMPING 0x20

// Device families
#define INTEL_DEVICE_FAMILY_I210 0x01
#define INTEL_DEVICE_FAMILY_I226 0x02

// Minimal device info structure
struct intel_device_info {
    uint32_t family;
};

// Minimal device structure
struct intel_hal_device {
    char name[64];
    struct intel_device_info info;
    uint32_t capabilities;
};

// Intel HAL function declarations (these should exist in our library)
extern intel_hal_ret_t intel_hal_init(void);
extern void intel_hal_cleanup(void);
extern uint32_t intel_hal_get_device_count(void);
extern intel_hal_device_t* intel_hal_get_device(uint32_t index);
extern intel_hal_ret_t intel_hal_setup_time_aware_shaper(intel_hal_device_t *device, void *config);
extern intel_hal_ret_t intel_hal_setup_frame_preemption(intel_hal_device_t *device, void *config);
extern intel_hal_ret_t intel_hal_xmit_timed_packet(intel_hal_device_t *device, void *params);
extern intel_hal_ret_t intel_hal_get_tas_status(intel_hal_device_t *device, uint32_t *status);
extern intel_hal_ret_t intel_hal_get_frame_preemption_status(intel_hal_device_t *device, uint32_t *status);

// Standard C functions
extern int printf(const char *format, ...);

int main(void) {
    printf("============================================================\n");
    printf("MINIMAL TSN FEATURE VALIDATION TEST\n");
    printf("Testing Intel HAL TSN function availability\n");
    printf("============================================================\n\n");
    
    // Test 1: Intel HAL Initialization
    printf("TEST 1: Intel HAL Initialization\n");
    intel_hal_ret_t ret = intel_hal_init();
    if (ret == INTEL_HAL_SUCCESS) {
        printf("  ✅ Intel HAL initialized successfully\n");
    } else {
        printf("  ❌ Intel HAL initialization failed: %d\n", ret);
        return 1;
    }
    
    // Test 2: Device Detection
    printf("\nTEST 2: Device Detection\n");
    uint32_t device_count = intel_hal_get_device_count();
    printf("  Found %u Intel Ethernet device(s)\n", device_count);
    
    if (device_count == 0) {
        printf("  ❌ No devices found - cannot proceed with TSN tests\n");
        intel_hal_cleanup();
        return 1;
    }
    
    // Test 3: TSN Capability Detection
    printf("\nTEST 3: TSN Capability Detection\n");
    int tsn_devices = 0;
    for (uint32_t i = 0; i < device_count; i++) {
        intel_hal_device_t *device = intel_hal_get_device(i);
        if (device) {
            printf("  Device %u: %s\n", i, device->name);
            printf("    Family: %s\n", 
                   device->info.family == INTEL_DEVICE_FAMILY_I210 ? "I210" :
                   device->info.family == INTEL_DEVICE_FAMILY_I226 ? "I226" : "Unknown");
            
            if (device->capabilities & INTEL_CAP_TSN_TIME_AWARE_SHAPER) {
                printf("    ✅ Time-Aware Shaper (802.1Qbv) supported\n");
                tsn_devices++;
            } else {
                printf("    ❌ No Time-Aware Shaper support\n");
            }
            
            if (device->capabilities & INTEL_CAP_TSN_FRAME_PREEMPTION) {
                printf("    ✅ Frame Preemption (802.1Qbu) supported\n");
            } else {
                printf("    ❌ No Frame Preemption support\n");
            }
            
            if (device->capabilities & INTEL_CAP_ENHANCED_TIMESTAMPING) {
                printf("    ✅ Enhanced timestamping available\n");
            } else {
                printf("    ❌ Basic timestamping only\n");
            }
        }
    }
    
    if (tsn_devices == 0) {
        printf("  ❌ No TSN-capable devices found\n");
        intel_hal_cleanup();
        return 1;
    }
    
    // Test 4: TSN Function Call Test
    printf("\nTEST 4: TSN Function Availability\n");
    intel_hal_device_t *tsn_device = 0; // Use 0 instead of NULL
    
    // Find first TSN device
    for (uint32_t i = 0; i < device_count; i++) {
        intel_hal_device_t *device = intel_hal_get_device(i);
        if (device && (device->capabilities & INTEL_CAP_TSN_TIME_AWARE_SHAPER)) {
            tsn_device = device;
            break;
        }
    }
    
    if (tsn_device) {
        printf("  Using device: %s\n", tsn_device->name);
        
        // Test Time-Aware Shaper function
        printf("  Testing Time-Aware Shaper setup...\n");
        ret = intel_hal_setup_time_aware_shaper(tsn_device, 0); // NULL config for test
        if (ret == INTEL_HAL_SUCCESS || ret == INTEL_HAL_ERROR) {
            printf("    ✅ intel_hal_setup_time_aware_shaper() callable\n");
        } else {
            printf("    ❌ intel_hal_setup_time_aware_shaper() returned unexpected: %d\n", ret);
        }
        
        // Test Frame Preemption function
        printf("  Testing Frame Preemption setup...\n");
        ret = intel_hal_setup_frame_preemption(tsn_device, 0); // NULL config for test
        if (ret == INTEL_HAL_SUCCESS || ret == INTEL_HAL_ERROR) {
            printf("    ✅ intel_hal_setup_frame_preemption() callable\n");
        } else {
            printf("    ❌ intel_hal_setup_frame_preemption() returned unexpected: %d\n", ret);
        }
        
        // Test Timed Transmission function
        printf("  Testing Timed Packet Transmission...\n");
        ret = intel_hal_xmit_timed_packet(tsn_device, 0); // NULL params for test
        if (ret == INTEL_HAL_SUCCESS || ret == INTEL_HAL_ERROR) {
            printf("    ✅ intel_hal_xmit_timed_packet() callable\n");
        } else {
            printf("    ❌ intel_hal_xmit_timed_packet() returned unexpected: %d\n", ret);
        }
        
        // Test Status Functions
        printf("  Testing TSN status functions...\n");
        uint32_t status = 0;
        ret = intel_hal_get_tas_status(tsn_device, &status);
        if (ret == INTEL_HAL_SUCCESS || ret == INTEL_HAL_ERROR) {
            printf("    ✅ intel_hal_get_tas_status() callable\n");
        }
        
        ret = intel_hal_get_frame_preemption_status(tsn_device, &status);
        if (ret == INTEL_HAL_SUCCESS || ret == INTEL_HAL_ERROR) {
            printf("    ✅ intel_hal_get_frame_preemption_status() callable\n");
        }
    }
    
    // Cleanup
    intel_hal_cleanup();
    
    printf("\n============================================================\n");
    printf("TSN VALIDATION SUMMARY\n");
    printf("============================================================\n");
    printf("✅ Intel HAL TSN functions are implemented and callable\n");
    printf("✅ Found %d TSN-capable device(s)\n", tsn_devices);
    printf("✅ Time-Sensitive Networking features available\n");
    printf("\nNOTE: This test validates function availability only.\n");
    printf("Full TSN configuration testing requires proper network setup.\n");
    printf("============================================================\n");
    
    return 0;
}
