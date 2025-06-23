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

#include <winsock2.h>
#include <windows.h>

#include "openavb_platform.h"
#include "openavb_osal.h"
#include "openavb_qmgr.h"

#define	AVB_LOG_COMPONENT	"osal"
#include "openavb_pub.h"
#include "openavb_log.h"

static FILE *s_logfile = NULL;
static bool s_wsa_started = FALSE;

extern DLL_EXPORT bool osalAVBInitialize(const char* logfilename, const char* ifname)
{
        if (!s_wsa_started) {
                WSADATA data;
                if (WSAStartup(MAKEWORD(2,2), &data) == 0) {
                        s_wsa_started = TRUE;
                }
        }
        // Open the log file, if requested.
        if (s_logfile) {
                fclose(s_logfile);
                s_logfile = NULL;
        }
        if (logfilename) {
                fopen_s(&s_logfile, logfilename, "w");
                if (s_logfile == NULL) {
                        fprintf(stderr, "Error opening log file: %s\n", logfilename);
                }
        }

	avbLogInitEx(s_logfile);
	osalAVBTimeInit();
	openavbQmgrInitialize(FQTSS_MODE_HW_CLASS, 0, ifname, 0, 0, 0);
	return TRUE;
}

extern DLL_EXPORT bool osalAVBFinalize(void)
{
        openavbQmgrFinalize();
        osalAVBTimeClose();
        avbLogExit();

        if (s_wsa_started) {
                WSACleanup();
                s_wsa_started = FALSE;
        }

	// Done with the log file.
        if (s_logfile) {
                fclose(s_logfile);
                s_logfile = NULL;
        }

	return TRUE;
}

