/**
 * OpenAvnu Profile Framework - Unit Tests
 * 
 * Comprehensive test suite for validating profile framework functionality
 */

#include "openavb_profile_framework.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf("FAIL: %s\n", message); \
            return false; \
        } else { \
            printf("PASS: %s\n", message); \
        } \
    } while(0)

/**
 * Test framework initialization
 */
bool test_framework_initialization(void)
{
    printf("\n=== Testing Framework Initialization ===\n");
    
    TEST_ASSERT(openavb_profile_framework_init(), "Framework initialization");
    
    // Test that built-in profiles are registered
    openavb_profile_cfg_t *avb = openavb_profile_get_by_name("AVB", NULL);
    TEST_ASSERT(avb != NULL, "AVB profile registered");
    
    openavb_profile_cfg_t *milan = openavb_profile_get_by_name("MILAN", NULL);
    TEST_ASSERT(milan != NULL, "MILAN profile registered");
    
    return true;
}

/**
 * Test profile compatibility
 */
bool test_profile_compatibility(void)
{
    printf("\n=== Testing Profile Compatibility ===\n");
    
    openavb_profile_cfg_t *milan_1_0 = openavb_profile_get_by_version(
        OPENAVB_SPEC_MILAN_1_0, OPENAVB_VARIANT_STRICT);
    openavb_profile_cfg_t *milan_1_1 = openavb_profile_get_by_version(
        OPENAVB_SPEC_MILAN_1_1, OPENAVB_VARIANT_STRICT);
    openavb_profile_cfg_t *avb = openavb_profile_get_by_version(
        OPENAVB_SPEC_IEEE_1722_2016, OPENAVB_VARIANT_STANDARD);
    
    TEST_ASSERT(milan_1_0 != NULL, "MILAN 1.0 profile found");
    TEST_ASSERT(milan_1_1 != NULL, "MILAN 1.1 profile found");
    TEST_ASSERT(avb != NULL, "AVB profile found");
    
    // Test backward compatibility
    TEST_ASSERT(openavb_profile_is_compatible(milan_1_1, milan_1_0), 
                "MILAN 1.1 backward compatible with 1.0");
    
    // Test security compatibility
    TEST_ASSERT(!openavb_profile_is_compatible(milan_1_1, avb), 
                "MILAN 1.1 not compatible with unsecured AVB");
    
    return true;
}

/**
 * Test capability-based decisions
 */
bool test_capability_based_decisions(void)
{
    printf("\n=== Testing Capability-Based Decisions ===\n");
    
    openavb_profile_cfg_t *milan = openavb_profile_get_by_version(
        OPENAVB_SPEC_MILAN_1_1, OPENAVB_VARIANT_STRICT);
    
    TEST_ASSERT(milan != NULL, "MILAN 1.1 profile found");
    
    // Test security capabilities
    TEST_ASSERT(milan->capabilities.security.authentication_required, 
                "MILAN requires authentication");
    TEST_ASSERT(milan->capabilities.security.supported_cipher_suites != 0, 
                "MILAN supports encryption");
    
    // Test timing capabilities
    TEST_ASSERT(milan->capabilities.timing.sync_uncertainty_tolerance_ns <= 250000, 
                "MILAN has strict timing tolerance");
    
    // Test transport capabilities
    TEST_ASSERT(milan->capabilities.transport.fast_connect_supported, 
                "MILAN supports fast connect");
    
    return true;
}

/**
 * Test profile registration
 */
bool test_profile_registration(void)
{
    printf("\n=== Testing Profile Registration ===\n");
    
    // Create a custom test profile
    static openavb_profile_cfg_t test_profile = {
        .profile_name = "TestProfile",
        .spec_version = OPENAVB_SPEC_CUSTOM_BASE,
        .spec_variant = OPENAVB_VARIANT_STANDARD,
        .profile_id = 0x8001,
        .version_string = "Test-1.0",
        .capabilities = {
            .security = {
                .authentication_required = false,
                .encryption_required = false
            },
            .timing = {
                .min_presentation_offset_ns = 1000000,
                .max_presentation_offset_ns = 10000000,
                .sync_uncertainty_tolerance_ns = 1000000
            }
        }
    };
    
    TEST_ASSERT(openavb_profile_register(&test_profile), "Custom profile registration");
    
    openavb_profile_cfg_t *retrieved = openavb_profile_get_by_name("TestProfile", NULL);
    TEST_ASSERT(retrieved != NULL, "Custom profile retrieval");
    TEST_ASSERT(retrieved->profile_id == 0x8001, "Custom profile ID correct");
    
    return true;
}

/**
 * Run all tests
 */
int main(void)
{
    printf("OpenAvnu Profile Framework Test Suite\n");
    printf("====================================\n");
    
    bool all_passed = true;
    
    all_passed &= test_framework_initialization();
    all_passed &= test_profile_compatibility();
    all_passed &= test_capability_based_decisions();
    all_passed &= test_profile_registration();
    
    printf("\n====================================\n");
    if (all_passed) {
        printf("All tests PASSED!\n");
        return 0;
    } else {
        printf("Some tests FAILED!\n");
        return 1;
    }
}
