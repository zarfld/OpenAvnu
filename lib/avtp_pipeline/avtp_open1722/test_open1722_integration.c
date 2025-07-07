/*************************************************************************************************************
Copyright (c) 2024, OpenAvnu Project
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS LISTED "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS LISTED BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*************************************************************************************************************/

/*
* MODULE SUMMARY : Test program for Open1722 AVTP integration
* 
* This test program validates the Open1722 integration and adapter layer
* functionality in OpenAvnu.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "openavb_platform.h"
#include "openavb_types.h"
#include "openavb_avtp.h"
#include "openavb_avtp_adapter.h"

#define	AVB_LOG_COMPONENT	"AVTP-Test"
#include "openavb_log.h"

/**
 * @brief Test Open1722 integration
 */
static bool testOpen1722Integration(void)
{
    printf("\n=== Testing Open1722 Integration ===\n");
    
    // Test implementation detection
    const char *impl = openavbAvtpGetImplementation();
    printf("AVTP Implementation: %s\n", impl);
    
    bool open1722Available = openavbAvtpIsOpen1722Available();
    printf("Open1722 Available: %s\n", open1722Available ? "Yes" : "No");
    
    // Test capabilities
    uint32_t caps = openavbAvtpGetCapabilities();
    printf("Capabilities: 0x%08X\n", caps);
    
    // Print detailed implementation info
    openavbAvtpPrintImplementationInfo();
    
    return TRUE;
}

/**
 * @brief Test stream configuration
 */
static bool testStreamConfiguration(void)
{
    printf("\n=== Testing Stream Configuration ===\n");
    
    // Create a test stream structure
    avtp_stream_t testStream;
    memset(&testStream, 0, sizeof(testStream));
    
    // Set basic parameters
    testStream.frameLen = 1500;
    testStream.ifname = "eth0";
    testStream.subtype = AVTP_SUBTYPE_61883_IIDC;
    testStream.tx = TRUE;
    
    // Test configuration validation
    bool valid = openavbAvtpValidateConfiguration(&testStream);
    printf("Basic configuration valid: %s\n", valid ? "Yes" : "No");
    
    // Test migration compatibility
    bool compatible = openavbAvtpCheckMigrationCompatibility(&testStream);
    printf("Migration compatible: %s\n", compatible ? "Yes" : "No");
    
    // Test audio configuration
    openavbRC rc = openavbAvtpConfigureAudio(&testStream, 2, 48000, 24);
    printf("Audio configuration: %s\n", (rc == OPENAVB_AVTP_SUCCESS) ? "SUCCESS" : "FAILED");
    
    // Test video configuration
    rc = openavbAvtpConfigureVideo(&testStream, 1920, 1080, 30, "H264");
    printf("Video configuration: %s\n", (rc == OPENAVB_AVTP_SUCCESS) ? "SUCCESS" : "FAILED");
    
#ifdef OPENAVNU_USE_OPEN1722
    // Test automotive configuration (Open1722 only)
    rc = openavbAvtpConfigureAutomotive(&testStream, "CAN", 1, NULL);
    printf("Automotive configuration: %s\n", (rc == OPENAVB_AVTP_SUCCESS) ? "SUCCESS" : "FAILED");
#else
    printf("Automotive configuration: SKIPPED (Legacy implementation)\n");
#endif
    
    return TRUE;
}

/**
 * @brief Test stream lifecycle
 */
static bool testStreamLifecycle(void)
{
    printf("\n=== Testing Stream Lifecycle ===\n");
    
    // Create a test stream
    avtp_stream_t testStream;
    memset(&testStream, 0, sizeof(testStream));
    
    testStream.frameLen = 1500;
    testStream.ifname = "eth0";
    testStream.subtype = AVTP_SUBTYPE_61883_IIDC;
    testStream.tx = TRUE;
    
    // Test stream creation
    openavbRC rc = openavbAvtpStreamCreate(&testStream);
    printf("Stream creation: %s\n", (rc == OPENAVB_AVTP_SUCCESS) ? "SUCCESS" : "FAILED");
    
    if (rc == OPENAVB_AVTP_SUCCESS) {
        // Test stream info retrieval
        openavb_avtp_stream_info_t streamInfo;
        rc = openavbAvtpGetStreamInfo(&testStream, &streamInfo);
        printf("Stream info retrieval: %s\n", (rc == OPENAVB_AVTP_SUCCESS) ? "SUCCESS" : "FAILED");
        
        if (rc == OPENAVB_AVTP_SUCCESS) {
            printf("  Implementation: %s\n", streamInfo.implementation);
            printf("  Capabilities: 0x%08X\n", streamInfo.capabilities);
            printf("  Packets sent: %llu\n", streamInfo.extended_stats.packets_sent);
            printf("  Packets received: %llu\n", streamInfo.extended_stats.packets_received);
        }
        
        // Test stream destruction
        rc = openavbAvtpStreamDestroy(&testStream);
        printf("Stream destruction: %s\n", (rc == OPENAVB_AVTP_SUCCESS) ? "SUCCESS" : "FAILED");
    }
    
    return (rc == OPENAVB_AVTP_SUCCESS);
}

/**
 * @brief Main test function
 */
int main(int argc, char *argv[])
{
    printf("OpenAvnu Open1722 Integration Test\n");
    printf("==================================\n");
    
    bool allTestsPassed = TRUE;
    
    // Run tests
    allTestsPassed &= testOpen1722Integration();
    allTestsPassed &= testStreamConfiguration();
    allTestsPassed &= testStreamLifecycle();
    
    // Print results
    printf("\n=== Test Results ===\n");
    printf("Overall result: %s\n", allTestsPassed ? "PASS" : "FAIL");
    
    return allTestsPassed ? 0 : 1;
}
