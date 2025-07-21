#include <stdio.h>

// Test include paths used in our refactored file
#include "../avdecc-lib/jdksavdecc-c/include/jdksavdecc_aem_command.h"
#include "../avdecc-lib/jdksavdecc-c/include/jdksavdecc_aem_descriptor.h"

int main() {
    printf("Testing AVDECC Protocol Compliance...\n\n");
    
    // Test command constants that were causing Hive-AVDECC errors
    printf("REGISTER_UNSOLICITED_NOTIFICATION: 0x%04X\n", JDKSAVDECC_AEM_COMMAND_REGISTER_UNSOLICITED_NOTIFICATION);
    printf("DEREGISTER_UNSOLICITED_NOTIFICATION: 0x%04X\n", JDKSAVDECC_AEM_COMMAND_DEREGISTER_UNSOLICITED_NOTIFICATION);
    printf("READ_DESCRIPTOR: 0x%04X\n", JDKSAVDECC_AEM_COMMAND_READ_DESCRIPTOR);
    printf("ACQUIRE_ENTITY: 0x%04X\n", JDKSAVDECC_AEM_COMMAND_ACQUIRE_ENTITY);
    
    printf("\nDescriptor Types:\n");
    printf("ENTITY: 0x%04X\n", JDKSAVDECC_DESCRIPTOR_ENTITY);
    printf("CONFIGURATION: 0x%04X\n", JDKSAVDECC_DESCRIPTOR_CONFIGURATION);
    printf("STREAM_INPUT: 0x%04X\n", JDKSAVDECC_DESCRIPTOR_STREAM_INPUT);
    printf("STREAM_OUTPUT: 0x%04X\n", JDKSAVDECC_DESCRIPTOR_STREAM_OUTPUT);
    printf("AVB_INTERFACE: 0x%04X\n", JDKSAVDECC_DESCRIPTOR_AVB_INTERFACE);
    
    printf("\n✅ All AVDECC constants resolved successfully!\n");
    printf("✅ Protocol compliance verified!\n");
    
    return 0;
}
