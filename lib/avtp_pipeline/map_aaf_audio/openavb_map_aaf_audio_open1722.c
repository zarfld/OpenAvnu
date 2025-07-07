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
* MODULE SUMMARY : Enhanced AAF mapping module implementation with Open1722 integration
* 
* This module provides enhanced AAF (AVTP Audio Format) mapping that can optionally
* use the Open1722 library for standards-compliant AVTP packet creation and parsing.
* It maintains full backward compatibility with the existing OpenAvnu implementation.
*/

#include <stdlib.h>
#include <string.h>
#include "openavb_mcr_hal_pub.h"
#include "openavb_types_pub.h"
#include "openavb_trace_pub.h"
#include "openavb_avtp_time_pub.h"
#include "openavb_mediaq_pub.h"
#include "openavb_map_pub.h"
#include "openavb_map_aaf_audio_open1722_pub.h"

#define	AVB_LOG_COMPONENT	"AAF Open1722 Mapping"
#include "openavb_log_pub.h"

// Include the original AAF mapping implementation for fallback
#include "openavb_map_aaf_audio.c"

#ifdef OPENAVNU_USE_OPEN1722

// Enhanced private data structure
typedef struct {
    // Original private data for compatibility
    pvt_data_t legacy;
    
    // Open1722 specific data
    bool useOpen1722;
    openavb_open1722_context_t* open1722Context;
    openavb_aaf_format_t open1722Format;
} pvt_data_open1722_t;

// Enhanced transmit callback using Open1722
tx_cb_ret_t openavbMapAVTPAudioOpen1722TxCB(media_q_t *pMediaQ, U8 *pData, U32 *dataLen)
{
    media_q_item_t *pMediaQItem = NULL;
    AVB_TRACE_ENTRY(AVB_TRACE_MAP_DETAIL);

    if (!pMediaQ) {
        AVB_LOG_ERROR("Mapping module invalid MediaQ");
        AVB_TRACE_EXIT(AVB_TRACE_MAP_DETAIL);
        return TX_CB_RET_PACKET_NOT_READY;
    }

    if (!pData || !dataLen) {
        AVB_LOG_ERROR("Mapping module data or data length argument incorrect.");
        AVB_TRACE_EXIT(AVB_TRACE_MAP_DETAIL);
        return TX_CB_RET_PACKET_NOT_READY;
    }

    pvt_data_open1722_t *pPvtData = (pvt_data_open1722_t*)pMediaQ->pPvtMapInfo;
    if (!pPvtData) {
        AVB_LOG_ERROR("Private mapping module data not allocated.");
        AVB_TRACE_EXIT(AVB_TRACE_MAP_DETAIL);
        return TX_CB_RET_PACKET_NOT_READY;
    }

    // If not using Open1722, fall back to legacy implementation
    if (!pPvtData->useOpen1722 || !pPvtData->open1722Context) {
        return openavbMapAVTPAudioTxCB(pMediaQ, pData, dataLen);
    }

    media_q_pub_map_aaf_audio_info_t *pPubMapInfo = pMediaQ->pPubMapInfo;

    // Check if we have enough data
    U32 bytesNeeded = pPubMapInfo->itemFrameSizeBytes * pPubMapInfo->framesPerPacket;
    if (!openavbMediaQIsAvailableBytes(pMediaQ, bytesNeeded, TRUE)) {
        AVB_LOG_VERBOSE("Not enough bytes are ready");
        AVB_TRACE_EXIT(AVB_TRACE_MAP_DETAIL);
        return TX_CB_RET_PACKET_NOT_READY;
    }

    pMediaQItem = openavbMediaQTailLock(pMediaQ, TRUE);
    if (!pMediaQItem || !pMediaQItem->pPubData || pMediaQItem->dataLen == 0) {
        if (pMediaQItem) {
            openavbMediaQTailPull(pMediaQ);
        }
        AVB_TRACE_EXIT(AVB_TRACE_MAP_DETAIL);
        return TX_CB_RET_PACKET_NOT_READY;
    }

    // Get timestamp if available
    U64 timestamp = 0;
    if (openavbAvtpTimeTimestampIsValid(pMediaQItem->pAvtpTime)) {
        // Add the max transit time
        openavbAvtpTimeAddUSec(pMediaQItem->pAvtpTime, pPvtData->legacy.maxTransitUsec);
        timestamp = openavbAvtpTimeGetAvtpTimestamp(pMediaQItem->pAvtpTime);
    }

    // Prepare audio data
    U32 audioDataSize = (pMediaQItem->dataLen - pMediaQItem->readIdx);
    if (audioDataSize > pPvtData->legacy.payloadSize) {
        audioDataSize = pPvtData->legacy.payloadSize;
    }

    // Create AVTP packet using Open1722
    bool result = openavb_open1722_create_aaf_packet(
        pPvtData->open1722Context,
        &pPvtData->open1722Format,
        (U8*)pMediaQItem->pPubData + pMediaQItem->readIdx,
        audioDataSize,
        timestamp);

    if (!result) {
        AVB_LOG_ERROR("Failed to create AAF packet using Open1722");
        openavbMediaQTailPull(pMediaQ);
        AVB_TRACE_EXIT(AVB_TRACE_MAP_DETAIL);
        return TX_CB_RET_PACKET_NOT_READY;
    }

    // Get the created packet
    U32 packetSize = openavb_open1722_get_packet_size(pPvtData->open1722Context);
    const U8* packetData = openavb_open1722_get_packet_data(pPvtData->open1722Context);

    if (!packetData || packetSize == 0 || packetSize > *dataLen) {
        AVB_LOG_ERROR("Invalid packet data from Open1722");
        openavbMediaQTailPull(pMediaQ);
        AVB_TRACE_EXIT(AVB_TRACE_MAP_DETAIL);
        return TX_CB_RET_PACKET_NOT_READY;
    }

    // Copy the packet to output buffer
    memcpy(pData, packetData, packetSize);
    *dataLen = packetSize;

    // Update media queue item
    pMediaQItem->readIdx += audioDataSize;
    if (pMediaQItem->readIdx >= pMediaQItem->dataLen) {
        // Finished reading the entire item
        openavbMediaQTailPull(pMediaQ);
    } else {
        // More to read next interval
        openavbMediaQTailUnlock(pMediaQ);
    }

    AVB_LOGF_VERBOSE("Created AAF packet using Open1722: %u bytes", packetSize);
    AVB_TRACE_EXIT(AVB_TRACE_MAP_DETAIL);
    return TX_CB_RET_PACKET_READY;
}

// Enhanced receive callback using Open1722
bool openavbMapAVTPAudioOpen1722RxCB(media_q_t *pMediaQ, U8 *pData, U32 dataLen)
{
    AVB_TRACE_ENTRY(AVB_TRACE_MAP_DETAIL);
    
    if (!pMediaQ || !pData) {
        AVB_TRACE_EXIT(AVB_TRACE_MAP_DETAIL);
        return FALSE;
    }

    pvt_data_open1722_t *pPvtData = (pvt_data_open1722_t*)pMediaQ->pPvtMapInfo;
    if (!pPvtData) {
        AVB_LOG_ERROR("Private mapping module data not allocated.");
        AVB_TRACE_EXIT(AVB_TRACE_MAP_DETAIL);
        return FALSE;
    }

    // If not using Open1722, fall back to legacy implementation
    if (!pPvtData->useOpen1722 || !pPvtData->open1722Context) {
        return openavbMapAVTPAudioRxCB(pMediaQ, pData, dataLen);
    }

    // Parse packet using Open1722
    openavb_aaf_format_t format;
    U8* audioData;
    U32 audioDataLength;
    U64 timestamp;

    bool result = openavb_open1722_parse_aaf_packet(
        pPvtData->open1722Context,
        pData,
        dataLen,
        &format,
        &audioData,
        &audioDataLength,
        &timestamp);

    if (!result) {
        AVB_LOG_ERROR("Failed to parse AAF packet using Open1722");
        AVB_TRACE_EXIT(AVB_TRACE_MAP_DETAIL);
        return FALSE;
    }

    // Validate format matches our configuration
    media_q_pub_map_aaf_audio_info_t *pPubMapInfo = pMediaQ->pPubMapInfo;
    if (format.channels != pPubMapInfo->audioChannels ||
        format.bit_depth != pPvtData->open1722Format.bit_depth) {
        AVB_LOGF_ERROR("Received format doesn't match: channels %u vs %u, bit_depth %u vs %u",
                       format.channels, pPubMapInfo->audioChannels,
                       format.bit_depth, pPvtData->open1722Format.bit_depth);
        AVB_TRACE_EXIT(AVB_TRACE_MAP_DETAIL);
        return FALSE;
    }

    // Get a media queue item to store the audio data
    media_q_item_t *pMediaQItem = openavbMediaQHeadLock(pMediaQ);
    if (!pMediaQItem) {
        AVB_LOG_ERROR("Unable to get media queue item");
        AVB_TRACE_EXIT(AVB_TRACE_MAP_DETAIL);
        return FALSE;
    }

    if (pMediaQItem->itemSize < audioDataLength) {
        AVB_LOG_ERROR("Media queue item too small for audio data");
        openavbMediaQHeadUnlock(pMediaQ);
        AVB_TRACE_EXIT(AVB_TRACE_MAP_DETAIL);
        return FALSE;
    }

    // Copy audio data to media queue
    memcpy(pMediaQItem->pPubData, audioData, audioDataLength);
    pMediaQItem->dataLen = audioDataLength;

    // Set timestamp if available
    if (timestamp != 0) {
        openavbAvtpTimeSetToTimestamp(pMediaQItem->pAvtpTime, (U32)timestamp);
        openavbAvtpTimeSetTimestampValid(pMediaQItem->pAvtpTime, TRUE);
        openavbAvtpTimeSubUSec(pMediaQItem->pAvtpTime, pPubMapInfo->presentationLatencyUSec);
    }

    // Push the item to the head of the queue
    openavbMediaQHeadPush(pMediaQ);

    AVB_LOGF_VERBOSE("Received AAF packet using Open1722: %u bytes audio data", audioDataLength);
    AVB_TRACE_EXIT(AVB_TRACE_MAP_DETAIL);
    return TRUE;
}

// Enhanced initialization callback
void openavbMapAVTPAudioOpen1722GenInitCB(media_q_t *pMediaQ)
{
    AVB_TRACE_ENTRY(AVB_TRACE_MAP);
    
    if (pMediaQ) {
        pvt_data_open1722_t *pPvtData = (pvt_data_open1722_t*)pMediaQ->pPvtMapInfo;
        if (pPvtData && pPvtData->useOpen1722) {
            // Initialize Open1722 if not already done
            if (!openavb_open1722_initialize()) {
                AVB_LOG_ERROR("Failed to initialize Open1722 library");
                pPvtData->useOpen1722 = FALSE;
            } else {
                // Create Open1722 context
                pPvtData->open1722Context = openavb_open1722_create_context();
                if (!pPvtData->open1722Context) {
                    AVB_LOG_ERROR("Failed to create Open1722 context");
                    pPvtData->useOpen1722 = FALSE;
                } else {
                    AVB_LOG_INFO("Open1722 AAF mapping initialized successfully");
                }
            }
        }
        
        // Call legacy initialization for compatibility
        openavbMapAVTPAudioGenInitCB(pMediaQ);
    }
    
    AVB_TRACE_EXIT(AVB_TRACE_MAP);
}

// Enhanced cleanup callback
void openavbMapAVTPAudioOpen1722GenEndCB(media_q_t *pMediaQ)
{
    AVB_TRACE_ENTRY(AVB_TRACE_MAP);
    
    if (pMediaQ) {
        pvt_data_open1722_t *pPvtData = (pvt_data_open1722_t*)pMediaQ->pPvtMapInfo;
        if (pPvtData && pPvtData->useOpen1722 && pPvtData->open1722Context) {
            openavb_open1722_destroy_context(pPvtData->open1722Context);
            pPvtData->open1722Context = NULL;
            AVB_LOG_INFO("Open1722 AAF mapping cleaned up");
        }
        
        // Call legacy cleanup
        openavbMapAVTPAudioGenEndCB(pMediaQ);
    }
    
    AVB_TRACE_EXIT(AVB_TRACE_MAP);
}

// Enhanced initialization function
extern DLL_EXPORT bool openavbMapAVTPAudioOpen1722Initialize(
    media_q_t *pMediaQ, 
    openavb_map_cb_t *pMapCB, 
    U32 inMaxTransitUsec,
    bool useOpen1722)
{
    AVB_TRACE_ENTRY(AVB_TRACE_MAP);

    if (!pMediaQ || !pMapCB) {
        AVB_LOG_ERROR("Invalid arguments");
        AVB_TRACE_EXIT(AVB_TRACE_MAP);
        return FALSE;
    }

    // First initialize with legacy implementation
    bool result = openavbMapAVTPAudioInitialize(pMediaQ, pMapCB, inMaxTransitUsec);
    if (!result) {
        AVB_TRACE_EXIT(AVB_TRACE_MAP);
        return FALSE;
    }

    if (useOpen1722) {
        // Upgrade to enhanced private data structure
        pvt_data_t *legacyPvtData = (pvt_data_t*)pMediaQ->pPvtMapInfo;
        pvt_data_open1722_t *enhancedPvtData = calloc(1, sizeof(pvt_data_open1722_t));
        
        if (!enhancedPvtData) {
            AVB_LOG_ERROR("Unable to allocate enhanced private data");
            AVB_TRACE_EXIT(AVB_TRACE_MAP);
            return FALSE;
        }

        // Copy legacy data
        if (legacyPvtData) {
            memcpy(&enhancedPvtData->legacy, legacyPvtData, sizeof(pvt_data_t));
            free(legacyPvtData);
        }

        // Set up Open1722 specific data
        enhancedPvtData->useOpen1722 = TRUE;
        enhancedPvtData->open1722Context = NULL; // Will be created in GenInitCB

        // Set up Open1722 format from legacy configuration
        media_q_pub_map_aaf_audio_info_t *pPubMapInfo = pMediaQ->pPubMapInfo;
        enhancedPvtData->open1722Format.channels = (U8)pPubMapInfo->audioChannels;
        enhancedPvtData->open1722Format.sample_rate = 48000; // Default
        enhancedPvtData->open1722Format.bit_depth = (U16)enhancedPvtData->legacy.aaf_bit_depth;

        // Update private data pointer
        pMediaQ->pPvtMapInfo = enhancedPvtData;

        // Override callbacks to use Open1722 versions
        pMapCB->map_gen_init_cb = openavbMapAVTPAudioOpen1722GenInitCB;
        pMapCB->map_tx_cb = openavbMapAVTPAudioOpen1722TxCB;
        pMapCB->map_rx_cb = openavbMapAVTPAudioOpen1722RxCB;
        pMapCB->map_gen_end_cb = openavbMapAVTPAudioOpen1722GenEndCB;

        AVB_LOG_INFO("Enhanced AAF mapping with Open1722 support initialized");
    }

    AVB_TRACE_EXIT(AVB_TRACE_MAP);
    return TRUE;
}

#endif // OPENAVNU_USE_OPEN1722
