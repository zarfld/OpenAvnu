#include "openavb_platform.h"
#include "openavb_grandmaster_osal.h"
#include "openavb_trace.h"

#define AVB_LOG_COMPONENT       "osalGrandmaster"
#include "openavb_pub.h"
#include "openavb_log.h"

// Windows does not currently provide a gPTP shared memory interface.
// These functions are stubs so that the library can link.

bool osalAVBGrandmasterInit(void)
{
    AVB_LOG_WARNING("Grandmaster support not implemented on Windows");
    return TRUE;
}

bool osalAVBGrandmasterClose(void)
{
    return TRUE;
}

bool osalAVBGrandmasterGetCurrent(
        uint8_t gptp_grandmaster_id[],
        uint8_t * gptp_domain_number )
{
    if (gptp_grandmaster_id) memset(gptp_grandmaster_id, 0, 8);
    if (gptp_domain_number) *gptp_domain_number = 0;
    AVB_LOG_WARNING("Grandmaster data unavailable on Windows");
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
    if (clock_identity) memset(clock_identity, 0, 8);
    if (priority1) *priority1 = 0;
    if (clock_class) *clock_class = 0;
    if (offset_scaled_log_variance) *offset_scaled_log_variance = 0;
    if (clock_accuracy) *clock_accuracy = 0;
    if (priority2) *priority2 = 0;
    if (domain_number) *domain_number = 0;
    if (log_sync_interval) *log_sync_interval = 0;
    if (log_announce_interval) *log_announce_interval = 0;
    if (log_pdelay_interval) *log_pdelay_interval = 0;
    if (port_number) *port_number = 0;
    AVB_LOG_WARNING("Grandmaster interface data unavailable on Windows");
    return TRUE;
}
