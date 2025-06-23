/*************************************************************************************************************
Copyright (c) 2012-2015, Symphony Teleca Corporation, a Harman International Industries, Incorporated company
Copyright (c) 2016-2017, Harman International Industries, Incorporated
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
 
Attributions: The inih library portion of the source code is licensed from 
Brush Technology and Ben Hoyt - Copyright (c) 2009, Brush Technology and Copyright (c) 2009, Ben Hoyt. 
Complete license and copyright information can be found at 
https://github.com/benhoyt/inih/commit/74d2ca064fb293bc60a77b0bd068075b293cf175.
*************************************************************************************************************/

#ifndef _OPENAVB_OS_SERVICES_OSAL_PUB_H
#define _OPENAVB_OS_SERVICES_OSAL_PUB_H

#define WINDOWS 1

#include "openavb_time_osal_pub.h"
#include "openavb_grandmaster_osal_pub.h"

#define INLINE_VARIABLE_NUM_OF_ARGUMENTS inline // must be okay of gcc

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <process.h>
#include <math.h>
#include <stdlib.h>


#define STD_LOG		stderr

#define NEWLINE		"\n"

#define SIN(rad) sin(rad)

#define THREAD_SELF()                          GetCurrentThreadId()
#define GET_PID()                        GetCurrentProcessId()


// Funky struct to hold a configurable ethernet address (MAC).
// The "mac" pointer is null if no config value was supplied,
// and points to the configured value if one was
typedef struct {
        unsigned char buffer[6];
        unsigned char *mac;
} cfg_mac_t;

#define MUTEX_ATTR_TYPE_DEFAULT                                    0
#define MUTEX_ATTR_TYPE_RECURSIVE                                  0
#define MUTEX_HANDLE(mutex_handle)                 HANDLE mutex_handle

#define MUTEX_ATTR_HANDLE(mutex_attr_name)
#define MUTEX_ATTR_CREATE_ERR()                    static int mutex_err
#define MUTEX_ATTR_INIT(mutex_attr_name)
#define MUTEX_ATTR_SET_TYPE(mutex_attr_name,type)
#define MUTEX_ATTR_SET_NAME(mutex_attr_name, name)
#define MUTEX_CREATE_ERR()                         int mutex_err
#define MUTEX_CREATE(mutex_handle,mutex_attr_name) do { mutex_handle = CreateMutex(NULL, FALSE, NULL); mutex_err = (mutex_handle == NULL); } while(0)
#define MUTEX_LOCK(mutex_handle)                   mutex_err = (WaitForSingleObject(mutex_handle, INFINITE) != WAIT_OBJECT_0)
#define MUTEX_UNLOCK(mutex_handle)                 mutex_err = (ReleaseMutex(mutex_handle) == 0)
#define MUTEX_DESTROY(mutex_handle)                mutex_err = (CloseHandle(mutex_handle) == 0)
#define MUTEX_LOG_ERR(message)                     if (mutex_err) AVB_LOG_ERROR(message);
#define MUTEX_IS_ERR  (mutex_err != 0)

#define MUTEX_HANDLE_ALT(mutex_handle) HANDLE mutex_handle
#define MUTEX_CREATE_ALT(mutex_handle) mutex_handle = CreateMutex(NULL, FALSE, NULL)
#define MUTEX_LOCK_ALT(mutex_handle) WaitForSingleObject(mutex_handle, INFINITE)
#define MUTEX_UNLOCK_ALT(mutex_handle) ReleaseMutex(mutex_handle)
#define MUTEX_DESTROY_ALT(mutex_handle) CloseHandle(mutex_handle)

#define ntohll(x)    _byteswap_uint64(x)
#define htonll(x)    _byteswap_uint64(x)

#endif // _OPENAVB_OS_SERVICES_OSAL_PUB_H

