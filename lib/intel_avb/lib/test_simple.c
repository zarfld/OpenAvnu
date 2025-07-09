#include <stdio.h>
#include "intel.h"

int main()
{
    printf("Intel HAL Test - Simple Version\n");
    printf("Testing basic compilation and linking...\n");
    
    // Test if we can at least compile with the header
    device_t dev;
    dev.pci_vendor_id = INTEL_VENDOR_ID;
    dev.pci_device_id = 0x15f2; // I225-LM
    
    printf("Device vendor: 0x%04x\n", dev.pci_vendor_id);
    printf("Device ID: 0x%04x\n", dev.pci_device_id);
    
    printf("Test completed successfully!\n");
    return 0;
}
