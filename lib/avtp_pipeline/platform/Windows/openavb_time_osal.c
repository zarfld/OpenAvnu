#include <windows.h>
#include <stdio.h>
#include "openavb_platform.h"
#include "openavb_time_osal.h"
#include "openavb_trace.h"

#define AVB_LOG_COMPONENT       "osalTime"
#include "openavb_pub.h"
#include "openavb_log.h"

static LARGE_INTEGER gFreq;
static BOOL gInit = FALSE;

bool osalAVBTimeInit(void)
{
    if (!QueryPerformanceFrequency(&gFreq))
        return FALSE;
    gInit = TRUE;
    return TRUE;
}

bool osalAVBTimeClose(void)
{
    return TRUE;
}

static void perfToTimespec(LARGE_INTEGER counter, struct timespec *ts)
{
    double secs = (double)counter.QuadPart / (double)gFreq.QuadPart;
    ts->tv_sec = (time_t)secs;
    ts->tv_nsec = (long)((secs - ts->tv_sec) * 1e9);
}

bool osalClockGettime(openavb_clockId_t clockId, struct timespec *tp)
{
    if (!gInit && !osalAVBTimeInit())
        return FALSE;

    if (clockId == OPENAVB_CLOCK_MONOTONIC || clockId == OPENAVB_TIMER_CLOCK) {
        LARGE_INTEGER c; QueryPerformanceCounter(&c); perfToTimespec(c, tp); return TRUE;
    }
    else {
        FILETIME ft; GetSystemTimeAsFileTime(&ft);
        ULONGLONG t = ((ULONGLONG)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
        t -= 116444736000000000ULL; /* epoch */
        tp->tv_sec = (long)(t / 10000000ULL);
        tp->tv_nsec = (long)((t % 10000000ULL) * 100);
        return TRUE;
    }
}

bool osalClockGettime64(openavb_clockId_t clockId, U64 *ns)
{
    struct timespec ts;
    if (!osalClockGettime(clockId, &ts))
        return FALSE;
    *ns = ((U64)ts.tv_sec * 1000000000ULL) + ts.tv_nsec;
    return TRUE;
}
