# Build Error: Intel HAL Header Path Resolution

**Date**: July 11, 2025  
**Context**: CI/CD pipeline build failure with Intel Ethernet HAL  

## Observation
Build fails with error:
```
D:\a\OpenAvnu\OpenAvnu\thirdparty\gptp\windows\daemon_cl\intel_hal_vendor_extensions.hpp(56,14): error C1083: Cannot open include file: '../../../../intel-ethernet-hal/include/intel_ethernet_hal.h': No such file or directory
```

## Root Cause
The relative path `../../../../intel-ethernet-hal/include/intel_ethernet_hal.h` in `intel_hal_vendor_extensions.hpp` doesn't resolve correctly in the CI build environment due to different directory structures between local and CI builds.

## Resolution
Update the include path in `intel_hal_vendor_extensions.hpp` to use the correct relative path for the CI environment:

```cpp
// Change from:
#include "../../../../intel-ethernet-hal/include/intel_ethernet_hal.h"

// To:
#include "../../../intel-ethernet-hal/include/intel_ethernet_hal.h"
```

## Takeaways
- CI/CD environments may have different directory structures than local development
- Always test include paths in both local and CI environments
- Consider using CMake's target-based include directories instead of relative paths
- Document any environment-specific build requirements

## Impact
This fix is critical for:
- CI/CD pipeline success
- Intel HAL integration in gPTP daemon
- I210/I225 hardware timestamping functionality
