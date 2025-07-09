#include <stdio.h>
#include "intel.h"

int main()
{
    printf("Intel HAL Test - Probe Function\n");
    
    device_t dev;
    dev.pci_vendor_id = INTEL_VENDOR_ID;
    dev.pci_device_id = 0x15f2; // I225-LM
    
    printf("Testing intel_probe function...\n");
    int result = intel_probe(&dev);
    printf("intel_probe result: %d\n", result);
    
    if (result == 0) {
        printf("Device capabilities: 0x%08x\n", dev.capabilities);
        printf("Device type: %d\n", dev.device_type);
    }
    
    printf("Test completed!\n");
    return 0;
}
