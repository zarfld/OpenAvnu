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
* MODULE SUMMARY : Test for enhanced AAF mapping with Open1722 integration
* 
* This module tests the integration between OpenAvnu's AAF mapping module
* and the Open1722 AVTP library.
*/

#include "openavb_map_aaf_audio_open1722_pub.h"

#ifdef OPENAVNU_USE_OPEN1722

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Mock media queue structure for testing
typedef struct {
    media_q_pub_map_aaf_audio_info_t pubInfo;
    void* pvtInfo;
    U8 mockDataBuffer[1024];
    media_q_item_t mockItem;
} mock_media_q_t;

int test_aaf_open1722_integration(void)
{
    printf("Testing Enhanced AAF Mapping with Open1722 Integration - Phase 2\n");
    
    // Test 1: Enhanced initialization with Open1722
    printf("1. Testing enhanced AAF mapping initialization...\n");
    
    mock_media_q_t mockMediaQ;
    memset(&mockMediaQ, 0, sizeof(mockMediaQ));
    
    // Set up basic media queue info
    mockMediaQ.pubInfo.audioChannels = 2;        // Stereo
    mockMediaQ.pubInfo.audioRate = 48000;        // 48kHz
    mockMediaQ.pubInfo.audioBitDepth = 16;       // 16-bit
    mockMediaQ.pubInfo.framesPerPacket = 6;      // 6 frames per packet
    mockMediaQ.pubInfo.itemFrameSizeBytes = 4;   // 2 channels * 2 bytes
    
    openavb_map_cb_t mapCallbacks;
    memset(&mapCallbacks, 0, sizeof(mapCallbacks));
    
    // Test enhanced initialization with Open1722 enabled
    bool initResult = openavbMapAVTPAudioOpen1722Initialize(
        (media_q_t*)&mockMediaQ, 
        &mapCallbacks, 
        1000,  // maxTransitUsec
        true   // useOpen1722
    );
    
    if (!initResult) {
        printf("FAIL: Enhanced AAF mapping initialization failed\n");
        return -1;
    }
    printf("PASS: Enhanced AAF mapping initialized with Open1722\n");
    
    // Test 2: Verify Open1722 wrapper is accessible
    printf("2. Testing Open1722 wrapper accessibility...\n");
    
    if (!openavb_open1722_initialize()) {
        printf("FAIL: Open1722 wrapper not accessible\n");
        return -1;
    }
    
    openavb_open1722_context_t* testContext = openavb_open1722_create_context();
    if (!testContext) {
        printf("FAIL: Could not create Open1722 context\n");
        return -1;
    }
    
    printf("PASS: Open1722 wrapper accessible and working\n");
    
    // Test 3: Test packet creation compatibility
    printf("3. Testing packet creation compatibility...\n");
    
    openavb_aaf_format_t testFormat = { 2, 48000, 16 };
    U8 testAudioData[64];
    memset(testAudioData, 0x5A, sizeof(testAudioData));
    
    bool packetResult = openavb_open1722_create_aaf_packet(
        testContext, &testFormat, testAudioData, sizeof(testAudioData), 54321);
    
    if (!packetResult) {
        printf("FAIL: Could not create test AAF packet\n");
        openavb_open1722_destroy_context(testContext);
        return -1;
    }
    
    U32 packetSize = openavb_open1722_get_packet_size(testContext);
    const U8* packetData = openavb_open1722_get_packet_data(testContext);
    
    if (!packetData || packetSize == 0) {
        printf("FAIL: Invalid packet data\n");
        openavb_open1722_destroy_context(testContext);
        return -1;
    }
    
    printf("PASS: AAF packet creation working, size: %u bytes\n", packetSize);
    
    // Test 4: Test packet parsing compatibility  
    printf("4. Testing packet parsing compatibility...\n");
    
    openavb_aaf_format_t parsedFormat;
    U8* parsedAudioData;
    U32 parsedDataLength;
    U64 parsedTimestamp;
    
    bool parseResult = openavb_open1722_parse_aaf_packet(
        testContext, packetData, packetSize, &parsedFormat,
        &parsedAudioData, &parsedDataLength, &parsedTimestamp);
    
    if (!parseResult) {
        printf("FAIL: Could not parse test AAF packet\n");
        openavb_open1722_destroy_context(testContext);
        return -1;
    }
    
    // Verify parsed data matches original
    if (parsedFormat.channels != testFormat.channels ||
        parsedFormat.bit_depth != testFormat.bit_depth ||
        parsedDataLength != sizeof(testAudioData) ||
        parsedTimestamp != 54321) {
        printf("FAIL: Parsed packet data doesn't match original\n");
        openavb_open1722_destroy_context(testContext);
        return -1;
    }
    
    printf("PASS: AAF packet parsing working correctly\n");
    
    // Test 5: Integration completeness check
    printf("5. Testing integration completeness...\n");
    
    // Verify callback functions are set
    if (!mapCallbacks.map_gen_init_cb ||
        !mapCallbacks.map_tx_cb ||
        !mapCallbacks.map_rx_cb ||
        !mapCallbacks.map_gen_end_cb) {
        printf("FAIL: Not all callback functions are set\n");
        openavb_open1722_destroy_context(testContext);
        return -1;
    }
    
    printf("PASS: All required callbacks are configured\n");
    
    // Cleanup
    openavb_open1722_destroy_context(testContext);
    openavb_open1722_shutdown();
    
    // Test 6: Legacy fallback test
    printf("6. Testing legacy fallback functionality...\n");
    
    // Test initialization with Open1722 disabled
    mock_media_q_t legacyMediaQ;
    memset(&legacyMediaQ, 0, sizeof(legacyMediaQ));
    legacyMediaQ.pubInfo = mockMediaQ.pubInfo;
    
    openavb_map_cb_t legacyCallbacks;
    memset(&legacyCallbacks, 0, sizeof(legacyCallbacks));
    
    bool legacyResult = openavbMapAVTPAudioOpen1722Initialize(
        (media_q_t*)&legacyMediaQ,
        &legacyCallbacks,
        1000,
        false  // useOpen1722 = false
    );
    
    if (!legacyResult) {
        printf("FAIL: Legacy fallback initialization failed\n");
        return -1;
    }
    
    printf("PASS: Legacy fallback working correctly\n");
    
    printf("\nAll Phase 2 integration tests passed!\n");
    printf("Enhanced AAF mapping with Open1722 integration is working correctly.\n");
    
    return 0;
}

#ifdef _WIN32
int main(void)
{
    return test_aaf_open1722_integration();
}
#endif

#endif // OPENAVNU_USE_OPEN1722
