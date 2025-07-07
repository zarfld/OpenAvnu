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
* MODULE SUMMARY : Simple test for OpenAvnu Open1722 AVTP wrapper
* 
* This module tests the basic functionality of the Phase 1 implementation.
*/

#include "openavb_avtp_open1722_simple.h"

#ifdef OPENAVNU_USE_OPEN1722

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

int test_open1722_basic_functionality(void)
{
    printf("Testing OpenAvnu Open1722 wrapper - Phase 1\n");
    
    // Test 1: Initialization
    printf("1. Testing initialization...\n");
    bool init_result = openavb_open1722_initialize();
    if (!init_result) {
        printf("FAIL: Initialization failed\n");
        return -1;
    }
    printf("PASS: Initialization successful\n");
    
    // Test 2: Context creation
    printf("2. Testing context creation...\n");
    openavb_open1722_context_t* context = openavb_open1722_create_context();
    if (!context) {
        printf("FAIL: Context creation failed\n");
        return -1;
    }
    printf("PASS: Context created successfully\n");
    
    // Test 3: AAF packet creation
    printf("3. Testing AAF packet creation...\n");
    openavb_aaf_format_t format = { 2, 48000, 16 };  // Stereo, 48kHz, 16-bit
    U8 test_audio_data[64];
    memset(test_audio_data, 0xAA, sizeof(test_audio_data));
    
    bool create_result = openavb_open1722_create_aaf_packet(
        context, &format, test_audio_data, sizeof(test_audio_data), 12345);
    
    if (!create_result) {
        printf("FAIL: AAF packet creation failed\n");
        openavb_open1722_destroy_context(context);
        return -1;
    }
    
    U32 packet_size = openavb_open1722_get_packet_size(context);
    const U8* packet_data = openavb_open1722_get_packet_data(context);
    
    if (!packet_data || packet_size == 0) {
        printf("FAIL: Invalid packet data\n");
        openavb_open1722_destroy_context(context);
        return -1;
    }
    
    printf("PASS: AAF packet created, size: %u bytes\n", packet_size);
    
    // Test 4: AAF packet parsing
    printf("4. Testing AAF packet parsing...\n");
    openavb_aaf_format_t parsed_format;
    U8* parsed_audio_data;
    U32 parsed_data_length;
    U64 parsed_timestamp;
    
    bool parse_result = openavb_open1722_parse_aaf_packet(
        context, packet_data, packet_size, &parsed_format,
        &parsed_audio_data, &parsed_data_length, &parsed_timestamp);
    
    if (!parse_result) {
        printf("FAIL: AAF packet parsing failed\n");
        openavb_open1722_destroy_context(context);
        return -1;
    }
    
    // Verify parsed data
    if (parsed_format.channels != format.channels ||
        parsed_format.bit_depth != format.bit_depth ||
        parsed_data_length != sizeof(test_audio_data) ||
        parsed_timestamp != 12345) {
        printf("FAIL: Parsed data doesn't match original\n");
        printf("  Channels: %u vs %u\n", parsed_format.channels, format.channels);
        printf("  Bit depth: %u vs %u\n", parsed_format.bit_depth, format.bit_depth);
        printf("  Data length: %u vs %u\n", parsed_data_length, (U32)sizeof(test_audio_data));
        printf("  Timestamp: %llu vs 12345\n", (unsigned long long)parsed_timestamp);
        openavb_open1722_destroy_context(context);
        return -1;
    }
    
    printf("PASS: AAF packet parsed correctly\n");
    printf("  Format: %u channels, %u-bit, %u Hz\n", 
           parsed_format.channels, parsed_format.bit_depth, parsed_format.sample_rate);
    printf("  Data length: %u bytes\n", parsed_data_length);
    printf("  Timestamp: %llu\n", (unsigned long long)parsed_timestamp);
    
    // Test 5: Context cleanup
    printf("5. Testing context cleanup...\n");
    openavb_open1722_destroy_context(context);
    printf("PASS: Context destroyed successfully\n");
    
    // Test 6: Shutdown
    printf("6. Testing shutdown...\n");
    openavb_open1722_shutdown();
    printf("PASS: Shutdown successful\n");
    
    printf("\nAll tests passed! Phase 1 implementation is working.\n");
    return 0;
}

#ifdef _WIN32
int main(void)
{
    return test_open1722_basic_functionality();
}
#endif

#endif // OPENAVNU_USE_OPEN1722
