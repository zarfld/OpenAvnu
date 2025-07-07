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
* MODULE SUMMARY : OpenAvnu wrapper implementation for Open1722 AVTP library - Phase 1
* 
* This module provides a functional implementation of the adaptation layer between
* OpenAvnu's existing AVTP API and the COVESA Open1722 library, including basic
* AAF (Audio) stream support.
*/

#include "openavb_avtp_open1722_simple.h"

#ifdef OPENAVNU_USE_OPEN1722

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Use our MSVC-compatible wrapper instead of problematic Open1722 headers
#include "openavb_open1722_msvc_wrapper.h"

static bool s_open1722_initialized = false;

bool openavb_open1722_initialize(void)
{
    if (s_open1722_initialized) {
        return true;
    }
    
    // Validate that our constants are correct
    if (AVTP_COMMON_HEADER_LEN != 4 || AVTP_AAF_HEADER_LEN != 24) {
        return false;  // Sanity check failed
    }
    
    s_open1722_initialized = true;
    return true;
}

void openavb_open1722_shutdown(void)
{
    if (!s_open1722_initialized) {
        return;
    }
    
    // Open1722 is header-only, no explicit shutdown needed
    s_open1722_initialized = false;
}

openavb_open1722_context_t* openavb_open1722_create_context(void)
{
    if (!s_open1722_initialized) {
        return NULL;
    }
    
    openavb_open1722_context_t* context = (openavb_open1722_context_t*)malloc(sizeof(openavb_open1722_context_t));
    if (!context) {
        return NULL;
    }
    
    memset(context, 0, sizeof(openavb_open1722_context_t));
    context->initialized = true;
    context->subtype = OPENAVB_AVTP_SUBTYPE_AAF;
    context->packet_size = 0;
    
    return context;
}

void openavb_open1722_destroy_context(openavb_open1722_context_t* context)
{
    if (!context) {
        return;
    }
    
    context->initialized = false;
    free(context);
}

bool openavb_open1722_create_aaf_packet(
    openavb_open1722_context_t* context,
    const openavb_aaf_format_t* format,
    const U8* audio_data,
    U32 data_length,
    U64 timestamp)
{
    if (!context || !context->initialized || !format || !audio_data) {
        return false;
    }
    
    // Check if packet will fit in buffer
    U32 total_size = AVTP_AAF_HEADER_LEN + data_length;
    if (total_size > OPENAVB_OPEN1722_MAX_PACKET_SIZE) {
        return false;
    }
    
    // Clear the packet buffer
    memset(context->packet_buffer, 0, sizeof(context->packet_buffer));
    
    // Create AAF packet using Open1722
    Avtp_Aaf_t* aaf_packet = (Avtp_Aaf_t*)context->packet_buffer;
    
    // Set common header fields
    Avtp_CommonHeader_SetSubtype((Avtp_CommonHeader_t*)aaf_packet, AVTP_SUBTYPE_AAF);
    Avtp_CommonHeader_SetStreamValid((Avtp_CommonHeader_t*)aaf_packet, 1);
    Avtp_CommonHeader_SetVersion((Avtp_CommonHeader_t*)aaf_packet, 0);
    
    // Set AAF-specific fields
    Avtp_Aaf_SetFormat(aaf_packet, AVTP_AAF_FORMAT_USER);
    Avtp_Aaf_SetNsr(aaf_packet, AVTP_AAF_NSR_USER_SPECIFIED);
    Avtp_Aaf_SetChannelsPerFrame(aaf_packet, format->channels);
    Avtp_Aaf_SetBitDepth(aaf_packet, (U8)format->bit_depth);
    Avtp_Aaf_SetStreamDataLength(aaf_packet, data_length);
    Avtp_Aaf_SetTimestamp(aaf_packet, (U32)(timestamp & 0xFFFFFFFF));
    
    // Copy audio data
    memcpy(aaf_packet->payload, audio_data, data_length);
    
    // Store format and packet size
    context->aaf_format = *format;
    context->packet_size = total_size;
    
    return true;
}

bool openavb_open1722_parse_aaf_packet(
    openavb_open1722_context_t* context,
    const U8* packet_data,
    U32 packet_length,
    openavb_aaf_format_t* format,
    U8** audio_data,
    U32* data_length,
    U64* timestamp)
{
    if (!context || !packet_data || !format || !audio_data || !data_length || !timestamp) {
        return false;
    }
    
    if (packet_length < AVTP_AAF_HEADER_LEN) {
        return false;
    }
    
    const Avtp_Aaf_t* aaf_packet = (const Avtp_Aaf_t*)packet_data;
    
    // Verify this is an AAF packet
    U8 subtype = Avtp_CommonHeader_GetSubtype((Avtp_CommonHeader_t*)aaf_packet);
    if (subtype != AVTP_SUBTYPE_AAF) {
        return false;
    }
    
    // Extract format information
    U8 channels, bit_depth;
    U16 stream_data_length;
    U32 ts;
    
    channels = Avtp_Aaf_GetChannelsPerFrame(aaf_packet);
    bit_depth = Avtp_Aaf_GetBitDepth(aaf_packet);
    stream_data_length = Avtp_Aaf_GetStreamDataLength(aaf_packet);
    ts = Avtp_Aaf_GetTimestamp(aaf_packet);
    
    // Validate data length
    if (stream_data_length > packet_length - AVTP_AAF_HEADER_LEN) {
        return false;
    }
    
    // Fill output parameters
    format->channels = channels;
    format->bit_depth = bit_depth;
    format->sample_rate = 48000; // Default, not stored in AAF packet
    
    *audio_data = (U8*)aaf_packet->payload;
    *data_length = stream_data_length;
    *timestamp = ts;
    
    return true;
}

U32 openavb_open1722_get_packet_size(const openavb_open1722_context_t* context)
{
    if (!context || !context->initialized) {
        return 0;
    }
    
    return context->packet_size;
}

const U8* openavb_open1722_get_packet_data(const openavb_open1722_context_t* context)
{
    if (!context || !context->initialized) {
        return NULL;
    }
    
    return context->packet_buffer;
}

#endif // OPENAVNU_USE_OPEN1722
