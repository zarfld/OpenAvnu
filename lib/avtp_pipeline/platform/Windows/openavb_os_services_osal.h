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

#ifndef _OPENAVB_OS_SERVICES_OSAL_H
#define _OPENAVB_OS_SERVICES_OSAL_H

#include "openavb_hal.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <process.h>
#include <time.h>
#include <errno.h>

//#include "openavb_tasks.h"

#define EXTERN_DLL_EXPORT  extern DLL_EXPORT

#ifndef PTHREAD_MUTEX_RECURSIVE
#define PTHREAD_MUTEX_RECURSIVE PTHREAD_MUTEX_RECURSIVE_NP
#endif

#include "openavb_osal_pub.h"

// Uncomment to use manual data alignment adjustments. Not needed for Linux
//#define DATA_ALIGNMENT_ADJUSTMENT	1

// Many socket implementations support a minimum timeout of 1ms (value 1000 here).
#define RAWSOCK_MIN_TIMEOUT_USEC        1000

#define SLEEP(sec)                  Sleep((sec)*1000)
#define SLEEP_MSEC(mSec)            Sleep(mSec)
#define SLEEP_NSEC(nSec)            Sleep((DWORD)((nSec)/1000000))
#define SLEEP_UNTIL_NSEC(nSec)  				   xSleepUntilNSec(nSec)
inline static void xSleepUntilNSec(U64 nSec)
{
        U64 nowNS;
        while (TRUE) {
                CLOCK_GETTIME64(OPENAVB_CLOCK_WALLTIME, &nowNS);
                if (nowNS >= nSec)
                        break;
                U64 diff = nSec - nowNS;
                Sleep((DWORD)(diff / NANOSECONDS_PER_MSEC));
        }
}

#define SPIN_UNTIL_NSEC(nsec)					xSpinUntilNSec(nsec)
inline static void xSpinUntilNSec(U64 nSec)
{
	do {
		U64 spinNowNS;
		CLOCK_GETTIME64(OPENAVB_CLOCK_WALLTIME, &spinNowNS);
		if (spinNowNS > nSec)
			break;
	}
	while (1);
}

#define RAND()                          rand()
#define SRAND(seed)                    srand(seed)
#define PRAGMA_ALIGN_8
#define SIGNAL_CALLBACK_SETUP(__NAM, __CB) \
        signal(__NAM, __CB)
#define SIGNAL_SIGNAL_SETUP(__SIG, __NAM, __CB, __ERR) \
        __ERR = (signal(__SIG, __CB) == SIG_ERR ? -1 : 0)
// the following macros define thread related items

#define THREAD_TYPE(thread) \
typedef struct { \
        HANDLE handle; \
        DWORD id; \
        int err; \
} thread##_type;

#define THREAD_DEFINITON(thread) \
thread##_type   thread##_ThreadData

#define THREAD_CREATE(threadName, threadhandle, thread_attr_name, thread_function, thread_function_arg) \
        { \
                threadhandle##_ThreadData.handle = CreateThread(NULL, threadName##_THREAD_STK_SIZE, \
                                (LPTHREAD_START_ROUTINE)thread_function, (void*)thread_function_arg, 0, \
                                &threadhandle##_ThreadData.id); \
                threadhandle##_ThreadData.err = (threadhandle##_ThreadData.handle == NULL); \
        }

#define THREAD_SET_RT_PRIORITY(threadhandle, priority) \
        SetThreadPriority(threadhandle##_ThreadData.handle, priority)

#define THREAD_PIN(threadhandle, affinity) \
        SetThreadAffinityMask(threadhandle##_ThreadData.handle, affinity)

#define THREAD_CHECK_ERROR(threadhandle, message, error) \
        do { \
                error=FALSE; \
                if (threadhandle##_ThreadData.err) { \
                        AVB_LOGF_ERROR("Thread error: %s", message); \
                        error=TRUE; \
                        break; \
                } \
        } while (0)

#define THREAD_STARTTHREAD(err)
#define THREAD_KILL(threadhandle, signal)          TerminateThread(threadhandle##_ThreadData.handle, signal)
#define THREAD_JOINABLE(threadhandle)
#define THREAD_JOIN(threadhandle, signal)          WaitForSingleObject(threadhandle##_ThreadData.handle, INFINITE)
#define THREAD_SLEEP(threadhandle, secs)                   Sleep((secs)*1000)


#define SEM_T(sem) HANDLE sem;
#define SEM_ERR_T(err) DWORD err;
#define SEM_INIT(sem, init, err) do { sem = CreateSemaphore(NULL, init, 0x7fffffff, NULL); err = (sem == NULL) ? GetLastError() : 0; } while (0)
#define SEM_WAIT(sem, err) do { DWORD _r = WaitForSingleObject(sem, INFINITE); err = (_r == WAIT_OBJECT_0) ? 0 : _r; } while (0)
#define SEM_TIMEDWAIT(sem, timeoutMSec, err) do { DWORD _r = WaitForSingleObject(sem, timeoutMSec); err = (_r == WAIT_OBJECT_0) ? 0 : _r; } while (0)
#define SEM_POST(sem, err) err = (ReleaseSemaphore(sem, 1, NULL) ? 0 : GetLastError())
#define SEM_DESTROY(sem, err) err = (CloseHandle(sem) ? 0 : GetLastError())
#define SEM_IS_ERR_NONE(err) (0 == (err))
#define SEM_IS_ERR_TIMEOUT(err) ((err) == WAIT_TIMEOUT)
#define SEM_LOG_ERR(err) if (0 != (err)) AVB_LOGF_ERROR("Semaphore error code: %lu", (unsigned long)(err));

typedef struct
{
	char *libName;
	char *funcName;
	void *libHandle;
} link_lib_t;

#define LINK_LIB(library)					\
link_lib_t library

#endif // _OPENAVB_OS_SERVICES_OSAL_H

