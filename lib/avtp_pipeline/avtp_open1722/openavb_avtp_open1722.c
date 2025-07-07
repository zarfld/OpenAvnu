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
* MODULE SUMMARY : OpenAvnu wrapper implementation for Open1722 AVTP library
* 
* This module provides the implementation of the adaptation layer between
* OpenAvnu's existing AVTP API and the COVESA Open1722 library.
*/

#include "openavb_avtp_open1722.h"

#ifdef OPENAVNU_USE_OPEN1722

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>

#include "openavb_platform.h"
#include "openavb_types_base_pub.h"
#include "openavb_trace_pub.h"

// Minimal implementation for now - will be expanded once we resolve compatibility issues
#include "avtp/Utils.h"
#include "avtp/aaf/PcmStream.h"
#include "avtp/cvf/H264.h"
#include "avtp/Crf.h"
#include "avtp/acf/Common.h"

#define	AVB_LOG_COMPONENT	"AVTP-Open1722"
#include "openavb_log.h"

// Helper macros for Open1722 integration
#define OPEN1722_STREAM_ID_SIZE 8
#define OPEN1722_MAX_PACKET_SIZE 1522

/**
 * @brief Get Open1722 context from stream
 */
static inline openavb_open1722_context_t* getOpen1722Context(avtp_stream_t *pStream)
{
    return (openavb_open1722_context_t*)pStream->pPrivateData;
}

/**
 * @brief Initialize Open1722 context
 */
static openavbRC initOpen1722Context(avtp_stream_t *pStream)
{
    AVB_TRACE_ENTRY(AVB_TRACE_AVTP);
    
    openavb_open1722_context_t *pContext = calloc(1, sizeof(openavb_open1722_context_t));
    if (!pContext) {
        AVB_RC_LOG_RET(AVB_RC(OPENAVB_AVTP_FAILURE | OPENAVB_RC_OUT_OF_MEMORY));
    }
    
    // Initialize Open1722 packet buffer
    pContext->pdu = calloc(1, OPEN1722_MAX_PACKET_SIZE);
    if (!pContext->pdu) {
        free(pContext);
        AVB_RC_LOG_RET(AVB_RC(OPENAVB_AVTP_FAILURE | OPENAVB_RC_OUT_OF_MEMORY));
    }
    
    // Initialize statistics
    memset(&pContext->stats, 0, sizeof(pContext->stats));
    
    // Initialize sequence number
    pContext->sequence_num = 0;
    
    // Set default format (will be configured later)
    pContext->format_type = OPENAVB_OPEN1722_FORMAT_AAF;
    
    pStream->pPrivateData = pContext;
    
    AVB_TRACE_EXIT(AVB_TRACE_AVTP);
    AVB_RC_RET(OPENAVB_AVTP_SUCCESS);
}

/**
 * @brief Cleanup Open1722 context
 */
static void cleanupOpen1722Context(avtp_stream_t *pStream)
{
    AVB_TRACE_ENTRY(AVB_TRACE_AVTP);
    
    openavb_open1722_context_t *pContext = getOpen1722Context(pStream);
    if (pContext) {
        if (pContext->pdu) {
            free(pContext->pdu);
        }
        free(pContext);
        pStream->pPrivateData = NULL;
    }
    
    AVB_TRACE_EXIT(AVB_TRACE_AVTP);
}

/**
 * @brief Configure Open1722 stream header
 */
static openavbRC configureStreamHeader(avtp_stream_t *pStream, struct avtp_stream_pdu *pdu)
{
    AVB_TRACE_ENTRY(AVB_TRACE_AVTP_DETAIL);
    
    openavb_open1722_context_t *pContext = getOpen1722Context(pStream);
    if (!pContext || !pdu) {
        AVB_RC_LOG_RET(AVB_RC(OPENAVB_AVTP_FAILURE | OPENAVB_RC_INVALID_ARGUMENT));
    }
    
    // Set common AVTP header fields
    int res = avtp_pdu_set((struct avtp_common_pdu*)pdu, AVTP_FIELD_SUBTYPE, pStream->subtype);
    if (res < 0) {
        AVB_RC_LOG_RET(AVB_RC(OPENAVB_AVTP_FAILURE | OPENAVB_RC_INVALID_ARGUMENT));
    }
    
    // Set stream ID
    res = avtp_pdu_set((struct avtp_common_pdu*)pdu, AVTP_FIELD_STREAM_ID, 
                       *(uint64_t*)pStream->streamID.buffer);
    if (res < 0) {
        AVB_RC_LOG_RET(AVB_RC(OPENAVB_AVTP_FAILURE | OPENAVB_RC_INVALID_ARGUMENT));
    }
    
    // Set sequence number
    res = avtp_pdu_set((struct avtp_common_pdu*)pdu, AVTP_FIELD_SEQ_NUM, 
                       pContext->sequence_num++);
    if (res < 0) {
        AVB_RC_LOG_RET(AVB_RC(OPENAVB_AVTP_FAILURE | OPENAVB_RC_INVALID_ARGUMENT));
    }
    
    AVB_TRACE_EXIT(AVB_TRACE_AVTP_DETAIL);
    AVB_RC_RET(OPENAVB_AVTP_SUCCESS);
}

/*
 * Public API Implementation
 */

openavbRC openavbAvtpOpen1722StreamCreate(avtp_stream_t *pStream)
{
    AVB_TRACE_ENTRY(AVB_TRACE_AVTP);
    
    if (!pStream) {
        AVB_RC_LOG_RET(AVB_RC(OPENAVB_AVTP_FAILURE | OPENAVB_RC_INVALID_ARGUMENT));
    }
    
    AVB_LOG_INFO("Creating Open1722 AVTP stream");
    
    // Initialize Open1722 context
    openavbRC rc = initOpen1722Context(pStream);
    if (rc != OPENAVB_AVTP_SUCCESS) {
        AVB_RC_LOG_RET(rc);
    }
    
    // Configure stream based on format
    // This will be extended as format-specific configuration is added
    
    AVB_LOG_INFO("Open1722 AVTP stream created successfully");
    
    AVB_TRACE_EXIT(AVB_TRACE_AVTP);
    AVB_RC_RET(OPENAVB_AVTP_SUCCESS);
}

openavbRC openavbAvtpOpen1722StreamDestroy(avtp_stream_t *pStream)
{
    AVB_TRACE_ENTRY(AVB_TRACE_AVTP);
    
    if (!pStream) {
        AVB_RC_LOG_RET(AVB_RC(OPENAVB_AVTP_FAILURE | OPENAVB_RC_INVALID_ARGUMENT));
    }
    
    AVB_LOG_INFO("Destroying Open1722 AVTP stream");
    
    cleanupOpen1722Context(pStream);
    
    AVB_LOG_INFO("Open1722 AVTP stream destroyed successfully");
    
    AVB_TRACE_EXIT(AVB_TRACE_AVTP);
    AVB_RC_RET(OPENAVB_AVTP_SUCCESS);
}

openavbRC openavbAvtpOpen1722TxBuffer(void *pv, avtp_info_t *pInfo)
{
    AVB_TRACE_ENTRY(AVB_TRACE_AVTP_DETAIL);
    
    if (!pv || !pInfo) {
        AVB_RC_LOG_RET(AVB_RC(OPENAVB_AVTP_FAILURE | OPENAVB_RC_INVALID_ARGUMENT));
    }
    
    avtp_stream_t *pStream = (avtp_stream_t*)pv;
    openavb_open1722_context_t *pContext = getOpen1722Context(pStream);
    
    if (!pContext) {
        AVB_RC_LOG_RET(AVB_RC(OPENAVB_AVTP_FAILURE | OPENAVB_RC_INVALID_ARGUMENT));
    }
    
    // Configure packet header
    openavbRC rc = configureStreamHeader(pStream, pContext->pdu);
    if (rc != OPENAVB_AVTP_SUCCESS) {
        AVB_RC_LOG_RET(rc);
    }
    
    // Format-specific packet creation will be implemented here
    // For now, this is a placeholder that maintains the existing API
    
    // Update statistics
    pContext->stats.packets_sent++;
    
    AVB_TRACE_EXIT(AVB_TRACE_AVTP_DETAIL);
    AVB_RC_RET(OPENAVB_AVTP_SUCCESS);
}

openavbRC openavbAvtpOpen1722RxBuffer(void *pv, avtp_info_t *pInfo)
{
    AVB_TRACE_ENTRY(AVB_TRACE_AVTP_DETAIL);
    
    if (!pv || !pInfo) {
        AVB_RC_LOG_RET(AVB_RC(OPENAVB_AVTP_FAILURE | OPENAVB_RC_INVALID_ARGUMENT));
    }
    
    avtp_stream_t *pStream = (avtp_stream_t*)pv;
    openavb_open1722_context_t *pContext = getOpen1722Context(pStream);
    
    if (!pContext) {
        AVB_RC_LOG_RET(AVB_RC(OPENAVB_AVTP_FAILURE | OPENAVB_RC_INVALID_ARGUMENT));
    }
    
    // Format-specific packet parsing will be implemented here
    // For now, this is a placeholder that maintains the existing API
    
    // Update statistics
    pContext->stats.packets_received++;
    
    AVB_TRACE_EXIT(AVB_TRACE_AVTP_DETAIL);
    AVB_RC_RET(OPENAVB_AVTP_SUCCESS);
}

openavbRC openavbAvtpOpen1722ConfigureAAF(avtp_stream_t *pStream, 
                                          uint8_t channels,
                                          uint32_t sample_rate,
                                          uint16_t bit_depth)
{
    AVB_TRACE_ENTRY(AVB_TRACE_AVTP);
    
    if (!pStream) {
        AVB_RC_LOG_RET(AVB_RC(OPENAVB_AVTP_FAILURE | OPENAVB_RC_INVALID_ARGUMENT));
    }
    
    openavb_open1722_context_t *pContext = getOpen1722Context(pStream);
    if (!pContext) {
        AVB_RC_LOG_RET(AVB_RC(OPENAVB_AVTP_FAILURE | OPENAVB_RC_INVALID_ARGUMENT));
    }
    
    AVB_LOG_INFO("Configuring AAF stream: channels=%u, sample_rate=%u, bit_depth=%u",
                 channels, sample_rate, bit_depth);
    
    // Set format type and configuration
    pContext->format_type = OPENAVB_OPEN1722_FORMAT_AAF;
    pContext->format_data.aaf_config.channels = channels;
    pContext->format_data.aaf_config.sample_rate = sample_rate;
    pContext->format_data.aaf_config.bit_depth = bit_depth;
    
    // Configure Open1722 AAF-specific settings
    // This will be expanded with actual Open1722 AAF configuration
    
    AVB_LOG_INFO("AAF stream configured successfully");
    
    AVB_TRACE_EXIT(AVB_TRACE_AVTP);
    AVB_RC_RET(OPENAVB_AVTP_SUCCESS);
}

openavbRC openavbAvtpOpen1722ConfigureCVF_H264(avtp_stream_t *pStream,
                                               uint16_t width,
                                               uint16_t height,
                                               uint8_t fps)
{
    AVB_TRACE_ENTRY(AVB_TRACE_AVTP);
    
    if (!pStream) {
        AVB_RC_LOG_RET(AVB_RC(OPENAVB_AVTP_FAILURE | OPENAVB_RC_INVALID_ARGUMENT));
    }
    
    openavb_open1722_context_t *pContext = getOpen1722Context(pStream);
    if (!pContext) {
        AVB_RC_LOG_RET(AVB_RC(OPENAVB_AVTP_FAILURE | OPENAVB_RC_INVALID_ARGUMENT));
    }
    
    AVB_LOG_INFO("Configuring CVF H.264 stream: %ux%u@%ufps", width, height, fps);
    
    // Set format type and configuration
    pContext->format_type = OPENAVB_OPEN1722_FORMAT_CVF_H264;
    pContext->format_data.cvf_config.width = width;
    pContext->format_data.cvf_config.height = height;
    pContext->format_data.cvf_config.fps = fps;
    
    // Configure Open1722 CVF-specific settings
    // This will be expanded with actual Open1722 CVF configuration
    
    AVB_LOG_INFO("CVF H.264 stream configured successfully");
    
    AVB_TRACE_EXIT(AVB_TRACE_AVTP);
    AVB_RC_RET(OPENAVB_AVTP_SUCCESS);
}

openavbRC openavbAvtpOpen1722ConfigureCRF(avtp_stream_t *pStream,
                                          uint64_t base_frequency,
                                          uint32_t pull)
{
    AVB_TRACE_ENTRY(AVB_TRACE_AVTP);
    
    if (!pStream) {
        AVB_RC_LOG_RET(AVB_RC(OPENAVB_AVTP_FAILURE | OPENAVB_RC_INVALID_ARGUMENT));
    }
    
    openavb_open1722_context_t *pContext = getOpen1722Context(pStream);
    if (!pContext) {
        AVB_RC_LOG_RET(AVB_RC(OPENAVB_AVTP_FAILURE | OPENAVB_RC_INVALID_ARGUMENT));
    }
    
    AVB_LOG_INFO("Configuring CRF stream: base_freq=%llu, pull=%u", base_frequency, pull);
    
    // Set format type and configuration
    pContext->format_type = OPENAVB_OPEN1722_FORMAT_CRF;
    pContext->format_data.crf_config.base_frequency = base_frequency;
    pContext->format_data.crf_config.pull = pull;
    
    // Configure Open1722 CRF-specific settings
    // This will be expanded with actual Open1722 CRF configuration
    
    AVB_LOG_INFO("CRF stream configured successfully");
    
    AVB_TRACE_EXIT(AVB_TRACE_AVTP);
    AVB_RC_RET(OPENAVB_AVTP_SUCCESS);
}

openavbRC openavbAvtpOpen1722ConfigureACF_CAN(avtp_stream_t *pStream,
                                              uint32_t can_bus_id,
                                              uint8_t message_type)
{
    AVB_TRACE_ENTRY(AVB_TRACE_AVTP);
    
    if (!pStream) {
        AVB_RC_LOG_RET(AVB_RC(OPENAVB_AVTP_FAILURE | OPENAVB_RC_INVALID_ARGUMENT));
    }
    
    openavb_open1722_context_t *pContext = getOpen1722Context(pStream);
    if (!pContext) {
        AVB_RC_LOG_RET(AVB_RC(OPENAVB_AVTP_FAILURE | OPENAVB_RC_INVALID_ARGUMENT));
    }
    
    AVB_LOG_INFO("Configuring ACF-CAN stream: bus_id=%u, message_type=%u", 
                 can_bus_id, message_type);
    
    // Set format type and configuration
    pContext->format_type = OPENAVB_OPEN1722_FORMAT_ACF_CAN;
    pContext->format_data.acf_can_config.can_bus_id = can_bus_id;
    pContext->format_data.acf_can_config.can_message_type = message_type;
    
    // Configure Open1722 ACF-CAN-specific settings
    // This will be expanded with actual Open1722 ACF configuration
    
    AVB_LOG_INFO("ACF-CAN stream configured successfully");
    
    AVB_TRACE_EXIT(AVB_TRACE_AVTP);
    AVB_RC_RET(OPENAVB_AVTP_SUCCESS);
}

openavbRC openavbAvtpOpen1722GetStats(avtp_stream_t *pStream, 
                                      openavb_open1722_context_t *pStats)
{
    AVB_TRACE_ENTRY(AVB_TRACE_AVTP);
    
    if (!pStream || !pStats) {
        AVB_RC_LOG_RET(AVB_RC(OPENAVB_AVTP_FAILURE | OPENAVB_RC_INVALID_ARGUMENT));
    }
    
    openavb_open1722_context_t *pContext = getOpen1722Context(pStream);
    if (!pContext) {
        AVB_RC_LOG_RET(AVB_RC(OPENAVB_AVTP_FAILURE | OPENAVB_RC_INVALID_ARGUMENT));
    }
    
    // Copy statistics
    memcpy(&pStats->stats, &pContext->stats, sizeof(pContext->stats));
    
    AVB_TRACE_EXIT(AVB_TRACE_AVTP);
    AVB_RC_RET(OPENAVB_AVTP_SUCCESS);
}

bool openavbAvtpOpen1722ValidatePacket(const uint8_t *packet, size_t packet_len)
{
    AVB_TRACE_ENTRY(AVB_TRACE_AVTP_DETAIL);
    
    if (!packet || packet_len == 0) {
        AVB_TRACE_EXIT(AVB_TRACE_AVTP_DETAIL);
        return FALSE;
    }
    
    // Use Open1722 validation functions
    // This is a placeholder - actual validation will use Open1722 utilities
    
    AVB_TRACE_EXIT(AVB_TRACE_AVTP_DETAIL);
    return TRUE;
}

#endif // OPENAVNU_USE_OPEN1722
