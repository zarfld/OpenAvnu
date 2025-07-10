#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <windows.h>
#include <iphlpapi.h>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

typedef struct {
    char device_name[256];
    char description[256];
    DWORD vendor_id;
    DWORD device_id;
    char friendly_name[256];
    DWORD if_index;
    GUID interface_guid;
} i225_device_info_t;

int find_i225_devices(i225_device_info_t devices[], int max_devices) {
    IP_ADAPTER_ADDRESSES *adapter_addresses = NULL;
    IP_ADAPTER_ADDRESSES *current_adapter = NULL;
    ULONG adapter_addresses_buffer_size = 0;
    DWORD result;
    int device_count = 0;

    // Get required buffer size
    result = GetAdaptersAddresses(AF_UNSPEC, 
                                GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_INCLUDE_ALL_INTERFACES,
                                NULL, adapter_addresses, &adapter_addresses_buffer_size);
    
    if (result != ERROR_BUFFER_OVERFLOW) {
        printf("❌ GetAdaptersAddresses sizing failed: %lu\n", result);
        return 0;
    }

    // Allocate buffer
    adapter_addresses = (IP_ADAPTER_ADDRESSES*)malloc(adapter_addresses_buffer_size);
    if (!adapter_addresses) {
        printf("❌ Memory allocation failed\n");
        return 0;
    }

    // Get actual adapter information
    result = GetAdaptersAddresses(AF_UNSPEC, 
                                GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_INCLUDE_ALL_INTERFACES,
                                NULL, adapter_addresses, &adapter_addresses_buffer_size);

    if (result != ERROR_SUCCESS) {
        printf("❌ GetAdaptersAddresses failed: %lu\n", result);
        free(adapter_addresses);
        return 0;
    }

    printf("🔍 Scanning for I225 devices...\n\n");

    current_adapter = adapter_addresses;
    while (current_adapter && device_count < max_devices) {
        // Check if this is an I225 device
        if (strstr(current_adapter->Description, "I225") != NULL) {
            printf("✅ Found I225 device:\n");
            printf("   Name: %S\n", current_adapter->FriendlyName);
            printf("   Description: %s\n", current_adapter->Description);
            printf("   Interface Index: %lu\n", current_adapter->IfIndex);
            printf("   Interface Type: %lu\n", current_adapter->IfType);
            printf("   Interface GUID: {%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX}\n",
                   current_adapter->NetworkGuid.Data1, current_adapter->NetworkGuid.Data2,
                   current_adapter->NetworkGuid.Data3, current_adapter->NetworkGuid.Data4[0],
                   current_adapter->NetworkGuid.Data4[1], current_adapter->NetworkGuid.Data4[2],
                   current_adapter->NetworkGuid.Data4[3], current_adapter->NetworkGuid.Data4[4],
                   current_adapter->NetworkGuid.Data4[5], current_adapter->NetworkGuid.Data4[6],
                   current_adapter->NetworkGuid.Data4[7]);

            if (current_adapter->PhysicalAddressLength == 6) {
                printf("   MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
                       current_adapter->PhysicalAddress[0], current_adapter->PhysicalAddress[1],
                       current_adapter->PhysicalAddress[2], current_adapter->PhysicalAddress[3],
                       current_adapter->PhysicalAddress[4], current_adapter->PhysicalAddress[5]);
            }

            printf("   Operational Status: ");
            switch (current_adapter->OperStatus) {
                case IfOperStatusUp:
                    printf("UP\n");
                    break;
                case IfOperStatusDown:
                    printf("DOWN\n");
                    break;
                case IfOperStatusTesting:
                    printf("TESTING\n");
                    break;
                case IfOperStatusUnknown:
                    printf("UNKNOWN\n");
                    break;
                case IfOperStatusDormant:
                    printf("DORMANT\n");
                    break;
                case IfOperStatusNotPresent:
                    printf("NOT PRESENT\n");
                    break;
                case IfOperStatusLowerLayerDown:
                    printf("LOWER LAYER DOWN\n");
                    break;
                default:
                    printf("Other (%lu)\n", current_adapter->OperStatus);
                    break;
            }

            // Fill device info structure
            strncpy_s(devices[device_count].device_name, sizeof(devices[device_count].device_name),
                     current_adapter->AdapterName, _TRUNCATE);
            strncpy_s(devices[device_count].description, sizeof(devices[device_count].description),
                     current_adapter->Description, _TRUNCATE);
            
            // Convert friendly name from wide char to char
            WideCharToMultiByte(CP_UTF8, 0, current_adapter->FriendlyName, -1,
                               devices[device_count].friendly_name, sizeof(devices[device_count].friendly_name),
                               NULL, NULL);
            
            devices[device_count].if_index = current_adapter->IfIndex;
            devices[device_count].interface_guid = current_adapter->NetworkGuid;
            
            // Assume I225-V based on description
            devices[device_count].vendor_id = 0x8086;
            devices[device_count].device_id = 0x15F3;  // I225-V

            printf("\n");
            device_count++;
        }
        current_adapter = current_adapter->Next;
    }

    free(adapter_addresses);
    return device_count;
}

int main() {
    i225_device_info_t devices[4];
    int device_count;

    printf("=================================================\n");
    printf("🔬 I225 Hardware Detection and Analysis Tool\n");
    printf("=================================================\n\n");

    device_count = find_i225_devices(devices, 4);

    if (device_count == 0) {
        printf("❌ No I225 devices found\n");
        return 1;
    }

    printf("📊 Summary:\n");
    printf("   Found %d I225 device(s)\n\n", device_count);

    for (int i = 0; i < device_count; i++) {
        printf("Device %d:\n", i + 1);
        printf("   Name: %s\n", devices[i].device_name);
        printf("   Description: %s\n", devices[i].description);
        printf("   Friendly Name: %s\n", devices[i].friendly_name);
        printf("   Vendor ID: 0x%04lX\n", devices[i].vendor_id);
        printf("   Device ID: 0x%04lX\n", devices[i].device_id);
        printf("   Interface Index: %lu\n", devices[i].if_index);
        printf("\n");
    }

    printf("🎯 I225-V Capabilities (based on datasheet):\n");
    printf("   ✅ 2.5 Gbps Ethernet\n");
    printf("   ✅ IEEE 1588 Hardware Timestamping\n");
    printf("   ✅ TSN (Time-Sensitive Networking)\n");
    printf("   ✅ Enhanced Hardware Timestamps\n");
    printf("   ✅ gPTP Support\n");
    printf("   ✅ Master/Slave Clock Capability\n");
    printf("   ✅ Hardware-based Synchronization\n");

    if (device_count >= 2) {
        printf("\n🚀 gPTP Master-Slave Test Setup Possible:\n");
        printf("   Device 1 (%s) → Master\n", devices[0].friendly_name);
        printf("   Device 2 (%s) → Slave\n", devices[1].friendly_name);
        printf("   Connection: Direct Ethernet cable between both adapters\n");
        printf("   Expected Sync Accuracy: < 1 microsecond\n");
    }

    return 0;
}
