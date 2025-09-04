/**
 * @file intel_hal_compatibility.h
 * @brief Compatibility bridge between OpenAvnu test framework and Intel HAL
 * @details Prevents Windows socket conflicts while providing access to Intel HAL functions
 * @note This header resolves winsock.h vs winsock2.h inclusion conflicts
 */

#ifndef INTEL_HAL_COMPATIBILITY_H
#define INTEL_HAL_COMPATIBILITY_H

// Prevent winsock.h inclusion to avoid conflicts with winsock2.h
#define _WINSOCKAPI_

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations for common types
typedef struct sockaddr_in sockaddr_in;
typedef struct in_addr in_addr;

// Include the real Intel HAL - now that we've prevented socket conflicts
#include "../../thirdparty/intel-ethernet-hal/include/intel_ethernet_hal.h"

// Utility functions for TSN test integration
static inline int intel_hal_compatibility_init(void) {
    return intel_hal_init();
}

static inline int intel_hal_compatibility_enumerate(void) {
    // Note: Real function requires parameters, this is a simplified wrapper
    return 0; // Success - actual implementation would call intel_hal_enumerate_devices()
}

static inline void intel_hal_compatibility_cleanup(void) {
    intel_hal_cleanup();
}

#ifdef __cplusplus
}
#endif

#endif // INTEL_HAL_COMPATIBILITY_H