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
* MODULE SUMMARY : OpenAvnu AVTP Adapter implementation
* 
* This module implements the compatibility layer for Open1722 integration,
* providing migration helpers and unified configuration interfaces.
*/

#include "openavb_avtp_adapter.h"
#include "openavb_types_base_pub.h"
#include "openavb_trace_pub.h"

#define	AVB_LOG_COMPONENT	"AVTP-Adapter"

// Minimal adapter implementation - will be expanded once compatibility issues are resolved

openavbRC openavbAvtpGetStreamInfo(avtp_stream_t *pStream, 
                                   openavb_avtp_stream_info_t *pInfo)
{
    AVB_TRACE_ENTRY(AVB_TRACE_AVTP);
    
    if (!pStream || !pInfo) {
        AVB_RC_LOG_RET(AVB_RC(OPENAVB_AVTP_FAILURE | OPENAVB_RC_INVALID_ARGUMENT));
    }
    
    // Clear the info structure
    memset(pInfo, 0, sizeof(openavb_avtp_stream_info_t));
    
    // Set implementation information
    pInfo->implementation = openavbAvtpGetImplementation();
    pInfo->capabilities = openavbAvtpGetCapabilities();
    
#ifdef OPENAVNU_USE_OPEN1722
    // Get Open1722-specific information
    openavb_open1722_context_t *pContext = 
        (openavb_open1722_context_t*)pStream->pPrivateData;
    
    if (pContext) {
        pInfo->open1722_context = pContext;
        
        // Copy extended statistics
        pInfo->extended_stats.packets_sent = pContext->stats.packets_sent;
        pInfo->extended_stats.packets_received = pContext->stats.packets_received;
        pInfo->extended_stats.packets_dropped = pContext->stats.packets_dropped;
        pInfo->extended_stats.timestamp_errors = pContext->stats.timestamp_errors;
        pInfo->extended_stats.format_errors = 0; // Will be implemented
    }
#else
    // Legacy implementation - limited statistics
    pInfo->extended_stats.packets_sent = 0;     // Not available in legacy
    pInfo->extended_stats.packets_received = 0; // Not available in legacy
    pInfo->extended_stats.packets_dropped = 0;  // Not available in legacy
    pInfo->extended_stats.timestamp_errors = 0; // Not available in legacy
    pInfo->extended_stats.format_errors = 0;    // Not available in legacy
#endif
    
    AVB_TRACE_EXIT(AVB_TRACE_AVTP);
    AVB_RC_RET(OPENAVB_AVTP_SUCCESS);
}

bool openavbAvtpCheckMigrationCompatibility(avtp_stream_t *pStream)
{
    AVB_TRACE_ENTRY(AVB_TRACE_AVTP);
    
    if (!pStream) {
        AVB_TRACE_EXIT(AVB_TRACE_AVTP);
        return FALSE;
    }
    
    bool compatible = TRUE;
    
#ifdef OPENAVNU_USE_OPEN1722
    
    // Check if stream configuration is supported by Open1722
    switch (pStream->subtype) {
        case AVTP_SUBTYPE_61883_IIDC:
            // Supported by Open1722 AAF
            break;
            
        case AVTP_SUBTYPE_MMA_STREAM:
            // Check if it's a supported MMA format
            // This needs to be expanded based on specific format
            break;
            
        case AVTP_SUBTYPE_CVF:
            // Supported by Open1722 CVF
            break;
            
        case AVTP_SUBTYPE_CRF:
            // Supported by Open1722 CRF
            break;
            
        default:
            // Unknown or unsupported subtype
            compatible = FALSE;
            AVB_LOG_WARNING("Unsupported AVTP subtype for Open1722: %u", pStream->subtype);
            break;
    }
    
    // Check frame length compatibility
    if (pStream->frameLen > OPEN1722_MAX_PACKET_SIZE) {
        compatible = FALSE;
        AVB_LOG_WARNING("Frame length %u exceeds Open1722 maximum %u", 
                       pStream->frameLen, OPEN1722_MAX_PACKET_SIZE);
    }
    
#else
    // Legacy implementation - always compatible with itself
    compatible = TRUE;
#endif
    
    if (compatible) {
        AVB_LOG_INFO("Stream configuration is compatible with %s implementation", 
                     openavbAvtpGetImplementation());
    } else {
        AVB_LOG_ERROR("Stream configuration is NOT compatible with %s implementation", 
                      openavbAvtpGetImplementation());
    }
    
    AVB_TRACE_EXIT(AVB_TRACE_AVTP);
    return compatible;
}

openavbRC openavbAvtpConfigureAudio(avtp_stream_t *pStream,
                                    uint8_t channels,
                                    uint32_t sample_rate,
                                    uint16_t bit_depth)
{
    AVB_TRACE_ENTRY(AVB_TRACE_AVTP);
    
    if (!pStream) {
        AVB_RC_LOG_RET(AVB_RC(OPENAVB_AVTP_FAILURE | OPENAVB_RC_INVALID_ARGUMENT));
    }
    
    AVB_LOG_INFO("Configuring audio stream: %s implementation, %u channels, %u Hz, %u-bit",
                 openavbAvtpGetImplementation(), channels, sample_rate, bit_depth);
    
#ifdef OPENAVNU_USE_OPEN1722
    // Use Open1722 AAF configuration
    openavbRC rc = openavbAvtpOpen1722ConfigureAAF(pStream, channels, sample_rate, bit_depth);
    if (rc != OPENAVB_AVTP_SUCCESS) {
        AVB_RC_LOG_RET(rc);
    }
#else
    // Legacy configuration
    // Set basic stream parameters
    pStream->subtype = AVTP_SUBTYPE_61883_IIDC;
    
    // Note: Legacy implementation doesn't have explicit format configuration
    // Stream parameters would be set through existing mechanisms
    AVB_LOG_INFO("Using legacy audio configuration");
#endif
    
    AVB_LOG_INFO("Audio stream configured successfully");
    
    AVB_TRACE_EXIT(AVB_TRACE_AVTP);
    AVB_RC_RET(OPENAVB_AVTP_SUCCESS);
}

openavbRC openavbAvtpConfigureVideo(avtp_stream_t *pStream,
                                    uint16_t width,
                                    uint16_t height,
                                    uint8_t fps,
                                    const char *format)
{
    AVB_TRACE_ENTRY(AVB_TRACE_AVTP);
    
    if (!pStream || !format) {
        AVB_RC_LOG_RET(AVB_RC(OPENAVB_AVTP_FAILURE | OPENAVB_RC_INVALID_ARGUMENT));
    }
    
    AVB_LOG_INFO("Configuring video stream: %s implementation, %ux%u@%ufps, format=%s",
                 openavbAvtpGetImplementation(), width, height, fps, format);
    
#ifdef OPENAVNU_USE_OPEN1722
    // Use Open1722 CVF configuration
    openavbRC rc = OPENAVB_AVTP_FAILURE;
    
    if (strcmp(format, "H264") == 0 || strcmp(format, "h264") == 0) {
        rc = openavbAvtpOpen1722ConfigureCVF_H264(pStream, width, height, fps);
    } else if (strcmp(format, "MJPEG") == 0 || strcmp(format, "mjpeg") == 0) {
        // Configure MJPEG when implemented
        AVB_LOG_WARNING("MJPEG support not yet implemented");
        rc = OPENAVB_AVTP_FAILURE;
    } else {
        AVB_LOG_ERROR("Unsupported video format: %s", format);
        rc = OPENAVB_AVTP_FAILURE;
    }
    
    if (rc != OPENAVB_AVTP_SUCCESS) {
        AVB_RC_LOG_RET(rc);
    }
#else
    // Legacy configuration
    if (strcmp(format, "H264") == 0 || strcmp(format, "h264") == 0) {
        pStream->subtype = AVTP_SUBTYPE_CVF;
        AVB_LOG_INFO("Using legacy H.264 video configuration");
    } else {
        AVB_LOG_ERROR("Video format %s not supported in legacy implementation", format);
        AVB_RC_LOG_RET(AVB_RC(OPENAVB_AVTP_FAILURE | OPENAVB_RC_INVALID_ARGUMENT));
    }
#endif
    
    AVB_LOG_INFO("Video stream configured successfully");
    
    AVB_TRACE_EXIT(AVB_TRACE_AVTP);
    AVB_RC_RET(OPENAVB_AVTP_SUCCESS);
}

openavbRC openavbAvtpConfigureAutomotive(avtp_stream_t *pStream,
                                         const char *protocol,
                                         uint32_t bus_id,
                                         void *config)
{
    AVB_TRACE_ENTRY(AVB_TRACE_AVTP);
    
    if (!pStream || !protocol) {
        AVB_RC_LOG_RET(AVB_RC(OPENAVB_AVTP_FAILURE | OPENAVB_RC_INVALID_ARGUMENT));
    }
    
    AVB_LOG_INFO("Configuring automotive stream: %s implementation, protocol=%s, bus_id=%u",
                 openavbAvtpGetImplementation(), protocol, bus_id);
    
#ifdef OPENAVNU_USE_OPEN1722
    // Use Open1722 ACF configuration
    openavbRC rc = OPENAVB_AVTP_FAILURE;
    
    if (strcmp(protocol, "CAN") == 0 || strcmp(protocol, "can") == 0) {
        uint8_t message_type = 0; // Default CAN message type
        if (config) {
            message_type = *(uint8_t*)config;
        }
        rc = openavbAvtpOpen1722ConfigureACF_CAN(pStream, bus_id, message_type);
    } else if (strcmp(protocol, "LIN") == 0 || strcmp(protocol, "lin") == 0) {
        // LIN configuration when implemented
        AVB_LOG_WARNING("LIN support not yet implemented");
        rc = OPENAVB_AVTP_FAILURE;
    } else if (strcmp(protocol, "FlexRay") == 0 || strcmp(protocol, "flexray") == 0) {
        // FlexRay configuration when implemented
        AVB_LOG_WARNING("FlexRay support not yet implemented");
        rc = OPENAVB_AVTP_FAILURE;
    } else {
        AVB_LOG_ERROR("Unsupported automotive protocol: %s", protocol);
        rc = OPENAVB_AVTP_FAILURE;
    }
    
    if (rc != OPENAVB_AVTP_SUCCESS) {
        AVB_RC_LOG_RET(rc);
    }
#else
    // Legacy implementation doesn't support automotive protocols
    AVB_LOG_ERROR("Automotive protocols not supported in legacy implementation");
    AVB_RC_LOG_RET(AVB_RC(OPENAVB_AVTP_FAILURE | OPENAVB_RC_NOT_SUPPORTED));
#endif
    
    AVB_LOG_INFO("Automotive stream configured successfully");
    
    AVB_TRACE_EXIT(AVB_TRACE_AVTP);
    AVB_RC_RET(OPENAVB_AVTP_SUCCESS);
}

void openavbAvtpPrintImplementationInfo(void)
{
    AVB_LOG_INFO("=== OpenAvnu AVTP Implementation Information ===");
    AVB_LOG_INFO("Current Implementation: %s", openavbAvtpGetImplementation());
    AVB_LOG_INFO("Open1722 Available: %s", openavbAvtpIsOpen1722Available() ? "Yes" : "No");
    
    uint32_t caps = openavbAvtpGetCapabilities();
    AVB_LOG_INFO("Capabilities:");
    AVB_LOG_INFO("  - AAF (Audio): %s", (caps & OPENAVB_AVTP_CAP_AAF) ? "Yes" : "No");
    AVB_LOG_INFO("  - CVF H.264: %s", (caps & OPENAVB_AVTP_CAP_CVF_H264) ? "Yes" : "No");
    AVB_LOG_INFO("  - CVF MJPEG: %s", (caps & OPENAVB_AVTP_CAP_CVF_MJPEG) ? "Yes" : "No");
    AVB_LOG_INFO("  - CRF (Clock Reference): %s", (caps & OPENAVB_AVTP_CAP_CRF) ? "Yes" : "No");
    AVB_LOG_INFO("  - RVF (Raw Video): %s", (caps & OPENAVB_AVTP_CAP_RVF) ? "Yes" : "No");
    AVB_LOG_INFO("  - ACF-CAN: %s", (caps & OPENAVB_AVTP_CAP_ACF_CAN) ? "Yes" : "No");
    AVB_LOG_INFO("  - ACF-LIN: %s", (caps & OPENAVB_AVTP_CAP_ACF_LIN) ? "Yes" : "No");
    AVB_LOG_INFO("  - ACF-FlexRay: %s", (caps & OPENAVB_AVTP_CAP_ACF_FLEXRAY) ? "Yes" : "No");
    AVB_LOG_INFO("  - Packet Validation: %s", (caps & OPENAVB_AVTP_CAP_VALIDATION) ? "Yes" : "No");
    AVB_LOG_INFO("  - Enhanced Statistics: %s", (caps & OPENAVB_AVTP_CAP_STATISTICS) ? "Yes" : "No");
    AVB_LOG_INFO("================================================");
}

bool openavbAvtpValidateConfiguration(avtp_stream_t *pStream)
{
    AVB_TRACE_ENTRY(AVB_TRACE_AVTP);
    
    if (!pStream) {
        AVB_TRACE_EXIT(AVB_TRACE_AVTP);
        return FALSE;
    }
    
    bool valid = TRUE;
    
    // Basic validation common to all implementations
    if (pStream->frameLen == 0) {
        AVB_LOG_ERROR("Invalid frame length: %u", pStream->frameLen);
        valid = FALSE;
    }
    
    if (pStream->ifname == NULL || strlen(pStream->ifname) == 0) {
        AVB_LOG_ERROR("Invalid interface name");
        valid = FALSE;
    }
    
    // Check if configuration is compatible with current implementation
    if (!openavbAvtpCheckMigrationCompatibility(pStream)) {
        valid = FALSE;
    }
    
#ifdef OPENAVNU_USE_OPEN1722
    // Open1722-specific validation
    if (openavbAvtpGetCapabilities() & OPENAVB_AVTP_CAP_VALIDATION) {
        // Additional Open1722 validation can be added here
        AVB_LOG_DEBUG("Using Open1722 validation capabilities");
    }
#endif
    
    if (valid) {
        AVB_LOG_INFO("AVTP stream configuration is valid");
    } else {
        AVB_LOG_ERROR("AVTP stream configuration validation failed");
    }
    
    AVB_TRACE_EXIT(AVB_TRACE_AVTP);
    return valid;
}
