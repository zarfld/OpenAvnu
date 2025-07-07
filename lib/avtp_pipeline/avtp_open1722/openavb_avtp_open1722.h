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
* HEADER SUMMARY : OpenAvnu wrapper for Open1722 AVTP library
* 
* This header provides an adaptation layer between OpenAvnu's existing AVTP API
* and the COVESA Open1722 library, enabling modernized AVTP support while maintaining
* backward compatibility with existing OpenAvnu code.
*/

#ifndef OPENAVB_AVTP_OPEN1722_H
#define OPENAVB_AVTP_OPEN1722_H 1

#include "openavb_platform.h"
#include "openavb_types_base_pub.h"

#ifdef OPENAVNU_USE_OPEN1722

// Forward declarations to avoid including problematic Open1722 headers for now
// We'll add these back incrementally as we fix compatibility issues
struct avtp_stream_pdu;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Open1722 context structure for AVTP streams
 * 
 * This structure extends the existing avtp_stream_t to include
 * Open1722-specific context and packet handling state.
 */
typedef struct {
    // Open1722 packet context
    struct avtp_stream_pdu *pdu;
    
    // Format-specific handlers
    enum {
        OPENAVB_OPEN1722_FORMAT_AAF,
        OPENAVB_OPEN1722_FORMAT_CVF_H264,
        OPENAVB_OPEN1722_FORMAT_CVF_MJPEG,
        OPENAVB_OPEN1722_FORMAT_CRF,
        OPENAVB_OPEN1722_FORMAT_RVF,
        OPENAVB_OPEN1722_FORMAT_ACF_CAN,
        OPENAVB_OPEN1722_FORMAT_ACF_LIN,
        OPENAVB_OPEN1722_FORMAT_ACF_FLEXRAY
    } format_type;
    
    // Format-specific configuration
    union {
        struct {
            uint8_t channels;
            uint32_t sample_rate;
            uint16_t bit_depth;
        } aaf_config;
        
        struct {
            uint16_t width;
            uint16_t height;
            uint8_t fps;
        } cvf_config;
        
        struct {
            uint64_t base_frequency;
            uint32_t pull;
        } crf_config;
        
        struct {
            uint32_t can_bus_id;
            uint8_t can_message_type;
        } acf_can_config;
    } format_data;
    
    // Packet assembly state
    uint8_t sequence_num;
    uint32_t timestamp_interval;
    
    // Statistics and monitoring
    struct {
        uint64_t packets_sent;
        uint64_t packets_received;
        uint64_t packets_dropped;
        uint64_t timestamp_errors;
    } stats;
    
} openavb_open1722_context_t;

/**
 * @brief Initialize Open1722 AVTP stream
 * 
 * Creates and configures an AVTP stream using Open1722 library
 * while maintaining compatibility with existing OpenAvnu stream API.
 * 
 * @param pStream Pointer to existing OpenAvnu stream structure
 * @return OPENAVB_AVTP_SUCCESS on success, error code on failure
 */
openavbRC openavbAvtpOpen1722StreamCreate(avtp_stream_t *pStream);

/**
 * @brief Destroy Open1722 AVTP stream
 * 
 * Cleans up Open1722 resources associated with the stream.
 * 
 * @param pStream Pointer to stream structure
 * @return OPENAVB_AVTP_SUCCESS on success, error code on failure
 */
openavbRC openavbAvtpOpen1722StreamDestroy(avtp_stream_t *pStream);

/**
 * @brief Transmit buffer using Open1722
 * 
 * Sends media data using Open1722 packet formatting and transmission.
 * 
 * @param pv Stream handle
 * @param pInfo AVTP information structure with media data
 * @return OPENAVB_AVTP_SUCCESS on success, error code on failure
 */
openavbRC openavbAvtpOpen1722TxBuffer(void *pv, avtp_info_t *pInfo);

/**
 * @brief Receive buffer using Open1722
 * 
 * Receives and parses AVTP packets using Open1722 library.
 * 
 * @param pv Stream handle
 * @param pInfo AVTP information structure to fill with received data
 * @return OPENAVB_AVTP_SUCCESS on success, error code on failure
 */
openavbRC openavbAvtpOpen1722RxBuffer(void *pv, avtp_info_t *pInfo);

/**
 * @brief Configure AAF (Audio AVTP Format) stream
 * 
 * Sets up an AAF stream with specified audio parameters.
 * 
 * @param pStream Stream structure
 * @param channels Number of audio channels
 * @param sample_rate Sample rate in Hz
 * @param bit_depth Bit depth (16, 24, 32)
 * @return OPENAVB_AVTP_SUCCESS on success, error code on failure
 */
openavbRC openavbAvtpOpen1722ConfigureAAF(avtp_stream_t *pStream, 
                                          uint8_t channels,
                                          uint32_t sample_rate,
                                          uint16_t bit_depth);

/**
 * @brief Configure CVF (Compressed Video Format) stream for H.264
 * 
 * Sets up a CVF stream for H.264 video transmission.
 * 
 * @param pStream Stream structure
 * @param width Video width in pixels
 * @param height Video height in pixels
 * @param fps Frames per second
 * @return OPENAVB_AVTP_SUCCESS on success, error code on failure
 */
openavbRC openavbAvtpOpen1722ConfigureCVF_H264(avtp_stream_t *pStream,
                                               uint16_t width,
                                               uint16_t height,
                                               uint8_t fps);

/**
 * @brief Configure CRF (Clock Reference Format) stream
 * 
 * Sets up a CRF stream for media clock synchronization.
 * 
 * @param pStream Stream structure
 * @param base_frequency Base frequency in Hz
 * @param pull Pull value for clock adjustment
 * @return OPENAVB_AVTP_SUCCESS on success, error code on failure
 */
openavbRC openavbAvtpOpen1722ConfigureCRF(avtp_stream_t *pStream,
                                          uint64_t base_frequency,
                                          uint32_t pull);

/**
 * @brief Configure ACF-CAN (AVTP Control Format for CAN) stream
 * 
 * Sets up an ACF stream for CAN bus data transport.
 * 
 * @param pStream Stream structure
 * @param can_bus_id CAN bus identifier
 * @param message_type CAN message type
 * @return OPENAVB_AVTP_SUCCESS on success, error code on failure
 */
openavbRC openavbAvtpOpen1722ConfigureACF_CAN(avtp_stream_t *pStream,
                                              uint32_t can_bus_id,
                                              uint8_t message_type);

/**
 * @brief Get Open1722 stream statistics
 * 
 * Retrieves detailed statistics about Open1722 stream performance.
 * 
 * @param pStream Stream structure
 * @param pStats Pointer to statistics structure to fill
 * @return OPENAVB_AVTP_SUCCESS on success, error code on failure
 */
openavbRC openavbAvtpOpen1722GetStats(avtp_stream_t *pStream, 
                                      openavb_open1722_context_t *pStats);

/**
 * @brief Validate Open1722 packet compliance
 * 
 * Checks if received packets conform to IEEE 1722-2016 standards.
 * 
 * @param packet Raw packet data
 * @param packet_len Packet length
 * @return TRUE if packet is compliant, FALSE otherwise
 */
bool openavbAvtpOpen1722ValidatePacket(const uint8_t *packet, size_t packet_len);

#ifdef __cplusplus
}
#endif

#endif // OPENAVNU_USE_OPEN1722

#endif // OPENAVB_AVTP_OPEN1722_H
