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
* HEADER SUMMARY : OpenAvnu AVTP Adapter - Compatibility layer for Open1722 integration
* 
* This header provides compatibility macros and transition helpers to enable
* seamless integration of Open1722 while maintaining backward compatibility
* with existing OpenAvnu AVTP code.
*/

#ifndef OPENAVB_AVTP_ADAPTER_H
#define OPENAVB_AVTP_ADAPTER_H 1

#include "openavb_platform.h"
#include "openavb_avtp.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief AVTP implementation selection
 * 
 * This macro determines which AVTP implementation to use based on
 * compile-time configuration.
 */
#ifdef OPENAVNU_USE_OPEN1722
    #include "openavb_avtp_open1722.h"
    #define OPENAVB_AVTP_IMPLEMENTATION "Open1722"
#else
    #define OPENAVB_AVTP_IMPLEMENTATION "Legacy"
#endif

/**
 * @brief Compatibility macros for AVTP stream operations
 * 
 * These macros provide a uniform interface that automatically selects
 * the appropriate implementation (Open1722 or legacy) based on configuration.
 */

#ifdef OPENAVNU_USE_OPEN1722

    // Open1722 implementation
    #define openavbAvtpStreamCreate(pStream) \
        openavbAvtpOpen1722StreamCreate(pStream)
    
    #define openavbAvtpStreamDestroy(pStream) \
        openavbAvtpOpen1722StreamDestroy(pStream)
    
    #define openavbAvtpTxBufferAdapted(pv, pInfo) \
        openavbAvtpOpen1722TxBuffer(pv, pInfo)
    
    #define openavbAvtpRxBufferAdapted(pv, pInfo) \
        openavbAvtpOpen1722RxBuffer(pv, pInfo)

#else

    // Legacy implementation (forward declarations)
    extern openavbRC openavbAvtpTxBuffer(void *pv, avtp_info_t *pInfo);
    extern openavbRC openavbAvtpRxBuffer(void *pv, avtp_info_t *pInfo);
    
    // Legacy implementation wrappers
    #define openavbAvtpStreamCreate(pStream) \
        OPENAVB_AVTP_SUCCESS  // Legacy doesn't have explicit create
    
    #define openavbAvtpStreamDestroy(pStream) \
        OPENAVB_AVTP_SUCCESS  // Legacy doesn't have explicit destroy
    
    #define openavbAvtpTxBufferAdapted(pv, pInfo) \
        openavbAvtpTxBuffer(pv, pInfo)
    
    #define openavbAvtpRxBufferAdapted(pv, pInfo) \
        openavbAvtpRxBuffer(pv, pInfo)

#endif

/**
 * @brief Migration helper functions
 * 
 * These functions assist in migrating from legacy AVTP to Open1722
 * and provide runtime information about the current implementation.
 */

/**
 * @brief Get current AVTP implementation name
 * 
 * @return String identifying the current AVTP implementation
 */
static inline const char* openavbAvtpGetImplementation(void)
{
    return OPENAVB_AVTP_IMPLEMENTATION;
}

/**
 * @brief Check if Open1722 is available
 * 
 * @return TRUE if Open1722 implementation is available and enabled
 */
static inline bool openavbAvtpIsOpen1722Available(void)
{
#ifdef OPENAVNU_USE_OPEN1722
    return TRUE;
#else
    return FALSE;
#endif
}

/**
 * @brief Get AVTP implementation capabilities
 * 
 * Returns a bitmask indicating the capabilities of the current
 * AVTP implementation.
 */
typedef enum {
    OPENAVB_AVTP_CAP_AAF          = 0x01,  // Audio AVTP Format
    OPENAVB_AVTP_CAP_CVF_H264     = 0x02,  // Compressed Video Format (H.264)
    OPENAVB_AVTP_CAP_CVF_MJPEG    = 0x04,  // Compressed Video Format (MJPEG)
    OPENAVB_AVTP_CAP_CRF          = 0x08,  // Clock Reference Format
    OPENAVB_AVTP_CAP_RVF          = 0x10,  // Raw Video Format
    OPENAVB_AVTP_CAP_ACF_CAN      = 0x20,  // AVTP Control Format (CAN)
    OPENAVB_AVTP_CAP_ACF_LIN      = 0x40,  // AVTP Control Format (LIN)
    OPENAVB_AVTP_CAP_ACF_FLEXRAY  = 0x80,  // AVTP Control Format (FlexRay)
    OPENAVB_AVTP_CAP_VALIDATION   = 0x100, // Packet validation
    OPENAVB_AVTP_CAP_STATISTICS   = 0x200  // Enhanced statistics
} openavb_avtp_capabilities_t;

/**
 * @brief Get AVTP implementation capabilities
 * 
 * @return Bitmask of available capabilities
 */
static inline uint32_t openavbAvtpGetCapabilities(void)
{
#ifdef OPENAVNU_USE_OPEN1722
    return OPENAVB_AVTP_CAP_AAF |
           OPENAVB_AVTP_CAP_CVF_H264 |
           OPENAVB_AVTP_CAP_CVF_MJPEG |
           OPENAVB_AVTP_CAP_CRF |
           OPENAVB_AVTP_CAP_RVF |
           OPENAVB_AVTP_CAP_ACF_CAN |
           OPENAVB_AVTP_CAP_ACF_LIN |
           OPENAVB_AVTP_CAP_ACF_FLEXRAY |
           OPENAVB_AVTP_CAP_VALIDATION |
           OPENAVB_AVTP_CAP_STATISTICS;
#else
    return OPENAVB_AVTP_CAP_AAF |
           OPENAVB_AVTP_CAP_CVF_H264;  // Legacy limited support
#endif
}

/**
 * @brief Enhanced AVTP stream information
 * 
 * Extended stream information structure that includes
 * Open1722-specific details when available.
 */
typedef struct {
    const char *implementation;
    uint32_t capabilities;
    
#ifdef OPENAVNU_USE_OPEN1722
    openavb_open1722_context_t *open1722_context;
#endif
    
    struct {
        uint64_t packets_sent;
        uint64_t packets_received;
        uint64_t packets_dropped;
        uint64_t timestamp_errors;
        uint64_t format_errors;
    } extended_stats;
    
} openavb_avtp_stream_info_t;

/**
 * @brief Get extended stream information
 * 
 * Retrieves detailed information about an AVTP stream, including
 * implementation-specific details and enhanced statistics.
 * 
 * @param pStream Pointer to stream structure
 * @param pInfo Pointer to info structure to fill
 * @return OPENAVB_AVTP_SUCCESS on success, error code on failure
 */
openavbRC openavbAvtpGetStreamInfo(avtp_stream_t *pStream, 
                                   openavb_avtp_stream_info_t *pInfo);

/**
 * @brief Migration status and compatibility checking
 */
typedef enum {
    OPENAVB_AVTP_MIGRATION_NOT_STARTED,
    OPENAVB_AVTP_MIGRATION_IN_PROGRESS,
    OPENAVB_AVTP_MIGRATION_COMPLETE,
    OPENAVB_AVTP_MIGRATION_FAILED
} openavb_avtp_migration_status_t;

/**
 * @brief Check migration compatibility
 * 
 * Verifies that existing AVTP configuration is compatible
 * with Open1722 implementation.
 * 
 * @param pStream Pointer to stream structure
 * @return TRUE if compatible, FALSE if migration issues exist
 */
bool openavbAvtpCheckMigrationCompatibility(avtp_stream_t *pStream);

/**
 * @brief Format-specific configuration helpers
 * 
 * These functions provide a unified interface for configuring
 * different AVTP formats regardless of the underlying implementation.
 */

/**
 * @brief Configure audio stream
 * 
 * @param pStream Stream structure
 * @param channels Number of audio channels
 * @param sample_rate Sample rate in Hz
 * @param bit_depth Bit depth (16, 24, 32)
 * @return OPENAVB_AVTP_SUCCESS on success, error code on failure
 */
openavbRC openavbAvtpConfigureAudio(avtp_stream_t *pStream,
                                    uint8_t channels,
                                    uint32_t sample_rate,
                                    uint16_t bit_depth);

/**
 * @brief Configure video stream
 * 
 * @param pStream Stream structure
 * @param width Video width in pixels
 * @param height Video height in pixels
 * @param fps Frames per second
 * @param format Video format (H264, MJPEG, etc.)
 * @return OPENAVB_AVTP_SUCCESS on success, error code on failure
 */
openavbRC openavbAvtpConfigureVideo(avtp_stream_t *pStream,
                                    uint16_t width,
                                    uint16_t height,
                                    uint8_t fps,
                                    const char *format);

/**
 * @brief Configure automotive control stream
 * 
 * @param pStream Stream structure
 * @param protocol Protocol type (CAN, LIN, FlexRay)
 * @param bus_id Bus identifier
 * @param config Protocol-specific configuration
 * @return OPENAVB_AVTP_SUCCESS on success, error code on failure
 */
openavbRC openavbAvtpConfigureAutomotive(avtp_stream_t *pStream,
                                         const char *protocol,
                                         uint32_t bus_id,
                                         void *config);

/**
 * @brief Debug and diagnostic helpers
 */

/**
 * @brief Print AVTP implementation information
 */
void openavbAvtpPrintImplementationInfo(void);

/**
 * @brief Validate AVTP configuration
 * 
 * Performs comprehensive validation of AVTP stream configuration
 * using the current implementation's validation capabilities.
 * 
 * @param pStream Stream structure to validate
 * @return TRUE if configuration is valid, FALSE otherwise
 */
bool openavbAvtpValidateConfiguration(avtp_stream_t *pStream);

#ifdef __cplusplus
}
#endif

#endif // OPENAVB_AVTP_ADAPTER_H
