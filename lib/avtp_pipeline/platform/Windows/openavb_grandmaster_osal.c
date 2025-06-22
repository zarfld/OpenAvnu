#include "openavb_platform.h"
#include "openavb_grandmaster_osal.h"
#include "openavb_trace.h"

#define AVB_LOG_COMPONENT       "osalGrandmaster"
#include "openavb_pub.h"
#include "openavb_log.h"

#include <windows.h>
#include <stdint.h>
#include <string.h>

/*
 * Windows implementation reads the gPTP information from a shared
 * memory section created by the gPTP daemon. The section layout is
 * identical to the Linux implementation: a mutex placeholder followed
 * by the gPtpTimeData structure. Synchronization is not currently
 * performed on Windows, so the mutex portion is ignored.
 */

typedef long double FrequencyRatio;

typedef struct {
    int64_t ml_phoffset;
    int64_t ls_phoffset;
    FrequencyRatio ml_freqoffset;
    FrequencyRatio ls_freqoffset;
    uint64_t local_time;

    uint8_t gptp_grandmaster_id[8];
    uint8_t gptp_domain_number;

    uint8_t  clock_identity[8];
    uint8_t  priority1;
    uint8_t  clock_class;
    int16_t  offset_scaled_log_variance;
    uint8_t  clock_accuracy;
    uint8_t  priority2;
    uint8_t  domain_number;
    int8_t   log_sync_interval;
    int8_t   log_announce_interval;
    int8_t   log_pdelay_interval;
    uint16_t port_number;
} gPtpTimeData;

#define GPTP_SECTION_NAME TEXT("Global\\ptp")

static HANDLE g_hMapFile = NULL;
static gPtpTimeData *g_pData = NULL;

static bool mapGPTPSection(void)
{
    g_hMapFile = OpenFileMapping(FILE_MAP_READ, FALSE, GPTP_SECTION_NAME);
    if (!g_hMapFile) {
        AVB_LOG_ERROR("Failed to open gPTP shared memory section");
        return FALSE;
    }

    g_pData = (gPtpTimeData*)MapViewOfFile(g_hMapFile, FILE_MAP_READ, 0, 0, sizeof(gPtpTimeData));
    if (!g_pData) {
        AVB_LOG_ERROR("Failed to map gPTP shared memory section");
        CloseHandle(g_hMapFile);
        g_hMapFile = NULL;
        return FALSE;
    }

    return TRUE;
}

bool osalAVBGrandmasterInit(void)
{
    if (g_pData)
        return TRUE;
    return mapGPTPSection();
}

bool osalAVBGrandmasterClose(void)
{
    if (g_pData) {
        UnmapViewOfFile(g_pData);
        g_pData = NULL;
    }
    if (g_hMapFile) {
        CloseHandle(g_hMapFile);
        g_hMapFile = NULL;
    }
    return TRUE;
}

static bool ensureMapped(void)
{
    if (!g_pData)
        return osalAVBGrandmasterInit();
    return TRUE;
}

bool osalAVBGrandmasterGetCurrent(
        uint8_t gptp_grandmaster_id[],
        uint8_t * gptp_domain_number )
{
    if (!ensureMapped())
        return FALSE;

    if (gptp_grandmaster_id)
        memcpy(gptp_grandmaster_id, g_pData->gptp_grandmaster_id, sizeof(g_pData->gptp_grandmaster_id));
    if (gptp_domain_number)
        *gptp_domain_number = g_pData->gptp_domain_number;

    return TRUE;
}

bool osalClockGrandmasterGetInterface(
        uint8_t clock_identity[],
        uint8_t * priority1,
        uint8_t * clock_class,
        int16_t * offset_scaled_log_variance,
        uint8_t * clock_accuracy,
        uint8_t * priority2,
        uint8_t * domain_number,
        int8_t * log_sync_interval,
        int8_t * log_announce_interval,
        int8_t * log_pdelay_interval,
        uint16_t * port_number)
{
    if (!ensureMapped())
        return FALSE;

    if (clock_identity)
        memcpy(clock_identity, g_pData->clock_identity, sizeof(g_pData->clock_identity));
    if (priority1)
        *priority1 = g_pData->priority1;
    if (clock_class)
        *clock_class = g_pData->clock_class;
    if (offset_scaled_log_variance)
        *offset_scaled_log_variance = g_pData->offset_scaled_log_variance;
    if (clock_accuracy)
        *clock_accuracy = g_pData->clock_accuracy;
    if (priority2)
        *priority2 = g_pData->priority2;
    if (domain_number)
        *domain_number = g_pData->domain_number;
    if (log_sync_interval)
        *log_sync_interval = g_pData->log_sync_interval;
    if (log_announce_interval)
        *log_announce_interval = g_pData->log_announce_interval;
    if (log_pdelay_interval)
        *log_pdelay_interval = g_pData->log_pdelay_interval;
    if (port_number)
        *port_number = g_pData->port_number;

    return TRUE;
}
