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
* HEADER SUMMARY : MSVC-Compatible Open1722 wrapper
* 
* This header provides MSVC-compatible definitions for Open1722 functionality
* by defining essential structures and constants without including problematic headers.
*/

#ifndef OPENAVB_OPEN1722_MSVC_WRAPPER_H
#define OPENAVB_OPEN1722_MSVC_WRAPPER_H 1

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Open1722 constants (copied from Defines.h to avoid inclusion issues)
#define AVTP_QUADLET_SIZE 4
#define AVTP_COMMON_HEADER_LEN (1 * AVTP_QUADLET_SIZE)
#define AVTP_AAF_HEADER_LEN (6 * AVTP_QUADLET_SIZE)

// AVTP subtypes
#define AVTP_SUBTYPE_AAF 0x02

// AAF format constants
#define AVTP_AAF_FORMAT_USER 0x00
#define AVTP_AAF_NSR_USER_SPECIFIED 0x00

// MSVC-compatible structure definitions (without __attribute__)
#pragma pack(push, 1)

typedef struct {
    uint8_t header[AVTP_COMMON_HEADER_LEN];
    uint8_t payload[0];
} Avtp_CommonHeader_t;

typedef struct {
    uint8_t header[AVTP_AAF_HEADER_LEN];
    uint8_t payload[0];
} Avtp_Aaf_t;

#pragma pack(pop)

// Simple inline functions to replace Open1722 functions
static inline void Avtp_CommonHeader_SetSubtype(Avtp_CommonHeader_t* pdu, uint8_t subtype)
{
    if (pdu) {
        pdu->header[0] = (pdu->header[0] & 0x80) | (subtype & 0x7F);
    }
}

static inline void Avtp_CommonHeader_SetStreamValid(Avtp_CommonHeader_t* pdu, uint8_t valid)
{
    if (pdu) {
        pdu->header[0] = (pdu->header[0] & 0x7F) | ((valid & 0x01) << 7);
    }
}

static inline void Avtp_CommonHeader_SetVersion(Avtp_CommonHeader_t* pdu, uint8_t version)
{
    if (pdu) {
        pdu->header[1] = (pdu->header[1] & 0x8F) | ((version & 0x07) << 4);
    }
}

static inline uint8_t Avtp_CommonHeader_GetSubtype(const Avtp_CommonHeader_t* pdu)
{
    return pdu ? (pdu->header[0] & 0x7F) : 0;
}

static inline void Avtp_Aaf_SetFormat(Avtp_Aaf_t* pdu, uint8_t format)
{
    if (pdu) {
        pdu->header[8] = format;
    }
}

static inline void Avtp_Aaf_SetNsr(Avtp_Aaf_t* pdu, uint8_t nsr)
{
    if (pdu) {
        pdu->header[9] = (pdu->header[9] & 0xF0) | (nsr & 0x0F);
    }
}

static inline void Avtp_Aaf_SetChannelsPerFrame(Avtp_Aaf_t* pdu, uint8_t channels)
{
    if (pdu) {
        pdu->header[10] = (pdu->header[10] & 0xFC) | ((channels - 1) & 0x03FF);
    }
}

static inline void Avtp_Aaf_SetBitDepth(Avtp_Aaf_t* pdu, uint8_t depth)
{
    if (pdu) {
        pdu->header[11] = depth;
    }
}

static inline void Avtp_Aaf_SetStreamDataLength(Avtp_Aaf_t* pdu, uint16_t length)
{
    if (pdu) {
        pdu->header[12] = (length >> 8) & 0xFF;
        pdu->header[13] = length & 0xFF;
    }
}

static inline void Avtp_Aaf_SetTimestamp(Avtp_Aaf_t* pdu, uint32_t timestamp)
{
    if (pdu) {
        pdu->header[16] = (timestamp >> 24) & 0xFF;
        pdu->header[17] = (timestamp >> 16) & 0xFF;
        pdu->header[18] = (timestamp >> 8) & 0xFF;
        pdu->header[19] = timestamp & 0xFF;
    }
}

static inline uint8_t Avtp_Aaf_GetChannelsPerFrame(const Avtp_Aaf_t* pdu)
{
    return pdu ? ((pdu->header[10] & 0x03FF) + 1) : 0;
}

static inline uint8_t Avtp_Aaf_GetBitDepth(const Avtp_Aaf_t* pdu)
{
    return pdu ? pdu->header[11] : 0;
}

static inline uint16_t Avtp_Aaf_GetStreamDataLength(const Avtp_Aaf_t* pdu)
{
    return pdu ? ((uint16_t)pdu->header[12] << 8) | pdu->header[13] : 0;
}

static inline uint32_t Avtp_Aaf_GetTimestamp(const Avtp_Aaf_t* pdu)
{
    if (!pdu) return 0;
    return ((uint32_t)pdu->header[16] << 24) |
           ((uint32_t)pdu->header[17] << 16) |
           ((uint32_t)pdu->header[18] << 8) |
           pdu->header[19];
}

#ifdef __cplusplus
}
#endif

#endif // OPENAVB_OPEN1722_MSVC_WRAPPER_H
