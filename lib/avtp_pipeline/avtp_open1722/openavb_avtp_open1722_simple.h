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
* HEADER SUMMARY : OpenAvnu wrapper for Open1722 AVTP library - Phase 1 Implementation
* 
* This header provides an adaptation layer between OpenAvnu's existing AVTP API
* and the COVESA Open1722 library. This Phase 1 implementation includes basic
* AVTP functionality with AAF (Audio) stream support.
*/

#ifndef OPENAVB_AVTP_OPEN1722_SIMPLE_H
#define OPENAVB_AVTP_OPEN1722_SIMPLE_H 1

#include "openavb_platform.h"
#include "openavb_types_base_pub.h"

#ifdef OPENAVNU_USE_OPEN1722

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations to avoid including Open1722 headers in public interface
struct avtp_stream_pdu;
struct Avtp_CommonHeader;
struct Avtp_Aaf;

// Maximum packet size for AVTP
#define OPENAVB_OPEN1722_MAX_PACKET_SIZE 1500

// AVTP subtypes supported
typedef enum {
    OPENAVB_AVTP_SUBTYPE_AAF = 0x02,  // Audio and Video Stream
    OPENAVB_AVTP_SUBTYPE_CVF = 0x03,  // Compressed Video Format
    OPENAVB_AVTP_SUBTYPE_CRF = 0x04   // Clock Reference Format
} openavb_avtp_subtype_t;

// AAF audio format
typedef struct {
    U8 channels;
    U32 sample_rate;
    U16 bit_depth;
} openavb_aaf_format_t;

// Simple context structure for Open1722 integration
typedef struct {
    bool initialized;
    void* open1722_context;
    openavb_avtp_subtype_t subtype;
    openavb_aaf_format_t aaf_format;
    U8 packet_buffer[OPENAVB_OPEN1722_MAX_PACKET_SIZE];
    U32 packet_size;
} openavb_open1722_context_t;

// Core initialization functions
bool openavb_open1722_initialize(void);
void openavb_open1722_shutdown(void);

// Context management
openavb_open1722_context_t* openavb_open1722_create_context(void);
void openavb_open1722_destroy_context(openavb_open1722_context_t* context);

// Basic AVTP packet operations
bool openavb_open1722_create_aaf_packet(
    openavb_open1722_context_t* context,
    const openavb_aaf_format_t* format,
    const U8* audio_data,
    U32 data_length,
    U64 timestamp);

bool openavb_open1722_parse_aaf_packet(
    openavb_open1722_context_t* context,
    const U8* packet_data,
    U32 packet_length,
    openavb_aaf_format_t* format,
    U8** audio_data,
    U32* data_length,
    U64* timestamp);

U32 openavb_open1722_get_packet_size(const openavb_open1722_context_t* context);
const U8* openavb_open1722_get_packet_data(const openavb_open1722_context_t* context);

#ifdef __cplusplus
}
#endif

#endif // OPENAVNU_USE_OPEN1722

#endif // OPENAVB_AVTP_OPEN1722_SIMPLE_H
