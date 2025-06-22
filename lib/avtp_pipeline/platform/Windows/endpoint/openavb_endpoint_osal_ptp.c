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

#include "openavb_platform.h"
#include "openavb_trace.h"
#include "openavb_endpoint.h"

#define	AVB_LOG_COMPONENT	"Endpoint PTP"
//#define AVB_LOG_LEVEL AVB_LOG_LEVEL_DEBUG
#include "openavb_pub.h"
#include "openavb_log.h"


inline int startPTP(void)
{
	AVB_TRACE_ENTRY(AVB_TRACE_ENDPOINT);

	// make sure ptp, a separate process, starts and is using the same interface as endpoint
	int retVal = 0;
//	char ptpCmd[80];
//	memset(ptpCmd, 0, 80);
//	snprintf(ptpCmd, 80, "./openavb_gptp %s -i %s &", x_cfg.ptp_start_opts, x_cfg.ifname);
//	AVB_LOGF_INFO("PTP start command: %s", ptpCmd);
//	if (system(ptpCmd) != 0) {
//		AVB_LOG_ERROR("PTP failed to start - Exiting");
//		retVal = -1;
//	}

	AVB_TRACE_EXIT(AVB_TRACE_ENDPOINT);
	return retVal;
}

inline int stopPTP(void)
{
	AVB_TRACE_ENTRY(AVB_TRACE_ENDPOINT);

	int retVal = 0;
//	if (system("killall -s SIGINT openavb_gptp") != 0) {
//		retVal = -1;
//	}

	AVB_TRACE_EXIT(AVB_TRACE_ENDPOINT);
	return retVal;
}
