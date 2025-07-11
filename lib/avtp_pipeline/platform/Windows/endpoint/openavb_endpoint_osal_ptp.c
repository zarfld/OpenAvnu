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

#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "openavb_platform.h"
#include "openavb_trace.h"
#include "openavb_endpoint.h"

#define	AVB_LOG_COMPONENT	"Endpoint PTP"
//#define AVB_LOG_LEVEL AVB_LOG_LEVEL_DEBUG
#include "openavb_pub.h"
#include "openavb_log.h"

static PROCESS_INFORMATION gptpProcInfo;


inline int startPTP(void)
{
        AVB_TRACE_ENTRY(AVB_TRACE_ENDPOINT);

        // Spawn gPTP daemon so that it uses the same network interface as the endpoint
        int retVal = -1;
        char cmdLine[256];
        STARTUPINFO si;

        ZeroMemory(&gptpProcInfo, sizeof(gptpProcInfo));
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);

        snprintf(cmdLine, sizeof(cmdLine), "openavb_gptp %s -i %s",
                         x_cfg.ptp_start_opts ? x_cfg.ptp_start_opts : "",
                         x_cfg.ifname);
        AVB_LOGF_INFO("PTP start command: %s", cmdLine);

        if (CreateProcess(NULL, cmdLine, NULL, NULL, FALSE,
                          CREATE_NEW_PROCESS_GROUP, NULL, NULL,
                          &si, &gptpProcInfo)) {
                WaitForInputIdle(gptpProcInfo.hProcess, 5000);
                retVal = 0;
        } else {
                AVB_LOGF_ERROR("PTP failed to start - error %lu", GetLastError());
        }

        AVB_TRACE_EXIT(AVB_TRACE_ENDPOINT);
        return retVal;
}

inline int stopPTP(void)
{
        AVB_TRACE_ENTRY(AVB_TRACE_ENDPOINT);

        int retVal = 0;

        if (gptpProcInfo.hProcess) {
                if (GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, gptpProcInfo.dwProcessId)) {
                        WaitForSingleObject(gptpProcInfo.hProcess, 5000);
                }

                if (WaitForSingleObject(gptpProcInfo.hProcess, 0) != WAIT_OBJECT_0) {
                        TerminateProcess(gptpProcInfo.hProcess, 0);
                        WaitForSingleObject(gptpProcInfo.hProcess, INFINITE);
                }

                CloseHandle(gptpProcInfo.hProcess);
                CloseHandle(gptpProcInfo.hThread);
                ZeroMemory(&gptpProcInfo, sizeof(gptpProcInfo));
        }

        AVB_TRACE_EXIT(AVB_TRACE_ENDPOINT);
        return retVal;
}
