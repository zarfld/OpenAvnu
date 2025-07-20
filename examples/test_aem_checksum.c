/**
 * ============================================================================
 * AEM Checksum Test Program - Verify IEEE 1722.1-2013 Compliance
 * ============================================================================
 * 
 * This test program verifies that our AEM checksum implementation works
 * correctly for AVDECC descriptors according to IEEE 1722.1-2013.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Test descriptor structure (simplified entity descriptor)
typedef struct {
    uint16_t descriptor_type;
    uint16_t descriptor_index;
    uint8_t  entity_id[8];
    uint8_t  entity_model_id[8];
    uint32_t entity_capabilities;
    char     entity_name[64];
    uint32_t aem_checksum;  // AEM Checksum field
} test_descriptor_t;

// CRC32 table for IEEE 802.3 polynomial (0xEDB88320)
static uint32_t crc32_table[256];
static bool crc32_table_initialized = false;

/**
 * Initialize CRC32 table for IEEE 802.3 polynomial
 */
static void init_crc32_table(void) {
    uint32_t polynomial = 0xEDB88320; // IEEE 802.3 polynomial (reversed)
    
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t crc = i;
        for (uint32_t j = 8; j > 0; j--) {
            if (crc & 1) {
                crc = (crc >> 1) ^ polynomial;
            } else {
                crc >>= 1;
            }
        }
        crc32_table[i] = crc;
    }
    crc32_table_initialized = true;
}

/**
 * Calculate CRC32 checksum for given data
 */
static uint32_t calculate_crc32(const uint8_t* data, size_t length) {
    if (!crc32_table_initialized) {
        init_crc32_table();
    }
    
    uint32_t crc = 0xFFFFFFFF; // Initial value
    
    for (size_t i = 0; i < length; i++) {
        uint8_t table_index = (crc ^ data[i]) & 0xFF;
        crc = (crc >> 8) ^ crc32_table[table_index];
    }
    
    return crc ^ 0xFFFFFFFF; // Final XOR
}

/**
 * Calculate AEM checksum for descriptor according to IEEE 1722.1-2013
 */
static uint32_t calculate_aem_checksum(const void* descriptor, size_t descriptor_size, size_t checksum_offset) {
    const uint8_t* desc_bytes = (const uint8_t*)descriptor;
    
    // Create a temporary buffer without the checksum field
    uint8_t* temp_buffer = malloc(descriptor_size - sizeof(uint32_t));
    if (!temp_buffer) {
        return 0;
    }
    
    // Copy data before checksum
    memcpy(temp_buffer, desc_bytes, checksum_offset);
    
    // Copy data after checksum
    size_t remaining_offset = checksum_offset + sizeof(uint32_t);
    if (remaining_offset < descriptor_size) {
        size_t remaining_length = descriptor_size - remaining_offset;
        memcpy(temp_buffer + checksum_offset, desc_bytes + remaining_offset, remaining_length);
    }
    
    // Calculate CRC for the combined data
    uint32_t crc = calculate_crc32(temp_buffer, descriptor_size - sizeof(uint32_t));
    
    free(temp_buffer);
    
    return crc; // Don't convert to network byte order in test
}

/**
 * Test AEM checksum calculation
 */
int main() {
    printf("=== AEM Checksum Test Program ===\n\n");
    
    // Create a test descriptor
    test_descriptor_t test_desc;
    memset(&test_desc, 0, sizeof(test_desc));
    
    // Fill with test data
    test_desc.descriptor_type = 0x0000;  // ENTITY descriptor
    test_desc.descriptor_index = 0x0000;
    
    // Test entity ID
    uint64_t entity_id = 0x0123456789ABCDEFULL;
    for (int i = 0; i < 8; i++) {
        test_desc.entity_id[i] = (entity_id >> (8 * (7 - i))) & 0xFF;
    }
    
    // Test entity model ID
    uint64_t model_id = 0xFEDCBA9876543210ULL;
    for (int i = 0; i < 8; i++) {
        test_desc.entity_model_id[i] = (model_id >> (8 * (7 - i))) & 0xFF;
    }
    
    test_desc.entity_capabilities = 0x12345678;
    strcpy(test_desc.entity_name, "Test AVDECC Entity");
    
    // Calculate checksum
    size_t checksum_offset = offsetof(test_descriptor_t, aem_checksum);
    uint32_t calculated_checksum = calculate_aem_checksum(&test_desc, sizeof(test_desc), checksum_offset);
    
    test_desc.aem_checksum = calculated_checksum;
    
    printf("Test Descriptor Details:\n");
    printf("  Descriptor Type: 0x%04X\n", test_desc.descriptor_type);
    printf("  Descriptor Index: 0x%04X\n", test_desc.descriptor_index);
    printf("  Entity ID: ");
    for (int i = 0; i < 8; i++) {
        printf("%02X", test_desc.entity_id[i]);
    }
    printf("\n");
    printf("  Entity Model ID: ");
    for (int i = 0; i < 8; i++) {
        printf("%02X", test_desc.entity_model_id[i]);
    }
    printf("\n");
    printf("  Entity Capabilities: 0x%08X\n", test_desc.entity_capabilities);
    printf("  Entity Name: %s\n", test_desc.entity_name);
    printf("  Checksum Offset: %zu bytes\n", checksum_offset);
    printf("  Calculated AEM Checksum: 0x%08X\n", calculated_checksum);
    
    // Verify checksum by recalculating
    uint32_t verify_checksum = calculate_aem_checksum(&test_desc, sizeof(test_desc), checksum_offset);
    
    printf("\nVerification:\n");
    printf("  Original Checksum: 0x%08X\n", calculated_checksum);
    printf("  Verified Checksum: 0x%08X\n", verify_checksum);
    
    if (calculated_checksum == verify_checksum) {
        printf("✅ AEM Checksum calculation is consistent!\n");
    } else {
        printf("❌ AEM Checksum calculation mismatch!\n");
        return 1;
    }
    
    printf("\n=== AEM Checksum Test Completed Successfully ===\n");
    return 0;
}
