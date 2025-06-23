#include "CppUTest/TestHarness.h"

#define false false
#define true true
extern "C" {
#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <pthread.h>
#include "avb_gptp.h"
#endif
#include "openavb_grandmaster_osal_pub.h"
}
#ifdef _WIN32
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
#endif
#include <cstring>

gPtpTimeData gPtpTD;

extern "C" void avbLogFn(unsigned level, const char *tag, const char *company,
                          const char *component, const char *path, int line,
                          const char *fmt, ...)
{
    (void)level; (void)tag; (void)company; (void)component;
    (void)path; (void)line; (void)fmt;
}

TEST_GROUP(GrandmasterOSAL)
{
#ifdef _WIN32
    HANDLE hMap;
    gPtpTimeData *map;
    void setup()
    {
        hMap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0,
                                 sizeof(gPtpTimeData), GPTP_SECTION_NAME);
        CHECK(hMap != NULL);
        map = (gPtpTimeData*)MapViewOfFile(hMap, FILE_MAP_WRITE, 0, 0,
                                           sizeof(gPtpTimeData));
        CHECK(map != NULL);

        memset(map, 0, sizeof(*map));
        for (int i = 0; i < 8; ++i) map->gptp_grandmaster_id[i] = (uint8_t)(i + 1);
        map->gptp_domain_number = 42;
    }

    void teardown()
    {
        osalAVBGrandmasterClose();
        if (map) {
            UnmapViewOfFile(map);
            map = NULL;
        }
        if (hMap) {
            CloseHandle(hMap);
            hMap = NULL;
        }
    }
#else
    int fd;
    char *map;
    void setup()
    {
        fd = shm_open(SHM_NAME, O_RDWR | O_CREAT, 0600);
        CHECK(fd != -1);
        ftruncate(fd, SHM_SIZE);
        map = (char*)mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        CHECK(map != MAP_FAILED);

        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
        pthread_mutex_init((pthread_mutex_t*)map, &attr);
        pthread_mutexattr_destroy(&attr);

        gPtpTimeData *data = (gPtpTimeData*)(map + sizeof(pthread_mutex_t));
        memset(data, 0, sizeof(*data));
        for (int i = 0; i < 8; ++i) data->gptp_grandmaster_id[i] = (uint8_t)(i + 1);
        data->gptp_domain_number = 42;
    }

    void teardown()
    {
        osalAVBGrandmasterClose();
        munmap(map, SHM_SIZE);
        close(fd);
        shm_unlink(SHM_NAME);
    }
#endif
};

TEST(GrandmasterOSAL, ReadGrandmaster)
{
    CHECK(osalAVBGrandmasterInit());
    uint8_t gm_id[8];
    uint8_t domain = 0;
    bool result = osalAVBGrandmasterGetCurrent(gm_id, &domain);
    CHECK(result);
    const uint8_t expected[8] = {1,2,3,4,5,6,7,8};
    MEMCMP_EQUAL(expected, gm_id, 8);
    LONGS_EQUAL(42, domain);
}
