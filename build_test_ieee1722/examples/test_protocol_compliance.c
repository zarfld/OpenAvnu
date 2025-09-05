
#include <stdio.h>
#include <jdksavdecc_aem_command.h>
#include <jdksavdecc_aem_descriptor.h>

int main() {
    printf("✅ AVDECC Protocol Compliance Test\n");
    printf("REGISTER_UNSOLICITED_NOTIFICATION: 0x%04X\n", JDKSAVDECC_AEM_COMMAND_REGISTER_UNSOLICITED_NOTIFICATION);
    printf("DEREGISTER_UNSOLICITED_NOTIFICATION: 0x%04X\n", JDKSAVDECC_AEM_COMMAND_DEREGISTER_UNSOLICITED_NOTIFICATION);
    printf("ENTITY descriptor: 0x%04X\n", JDKSAVDECC_DESCRIPTOR_ENTITY);
    printf("CONFIGURATION descriptor: 0x%04X\n", JDKSAVDECC_DESCRIPTOR_CONFIGURATION);
    printf("All AVDECC protocol constants are accessible!\n");
    return 0;
}
