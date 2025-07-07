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
* HEADER SUMMARY : Enhanced AAF mapping module with Open1722 integration
* 
* This header provides an enhanced AAF audio mapping that can optionally use
* the Open1722 library for AVTP packet creation and parsing while maintaining
* full backward compatibility with the existing implementation.
*/

#ifndef OPENAVB_MAP_AAF_AUDIO_OPEN1722_PUB_H
#define OPENAVB_MAP_AAF_AUDIO_OPEN1722_PUB_H 1

#include "openavb_map_aaf_audio_pub.h"

#ifdef OPENAVNU_USE_OPEN1722
#include "openavb_avtp_open1722_simple.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Enhanced AAF mapping info structure with Open1722 support
typedef struct {
    // Base AAF mapping info (for compatibility)
    media_q_pub_map_aaf_audio_info_t base;
    
#ifdef OPENAVNU_USE_OPEN1722
    // Open1722 integration
    bool useOpen1722;
    openavb_open1722_context_t* open1722Context;
    openavb_aaf_format_t open1722Format;
#endif
} media_q_pub_map_aaf_audio_open1722_info_t;

// Enhanced initialization function with Open1722 support
extern DLL_EXPORT bool openavbMapAVTPAudioOpen1722Initialize(
    media_q_t *pMediaQ, 
    openavb_map_cb_t *pMapCB, 
    U32 inMaxTransitUsec,
    bool useOpen1722);

#ifdef __cplusplus
}
#endif

#endif // OPENAVB_MAP_AAF_AUDIO_OPEN1722_PUB_H
