#include "osal_time.h"

#ifdef OSAL_PLATFORM_POSIX

#include <time.h>
#include <unistd.h>

/* =========================
 * POSIX monotonic clock
 * ========================= */

uint64_t osal_time_us(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    return (uint64_t)ts.tv_sec * 1000000ULL + ts.tv_nsec / 1000ULL;
}

uint64_t osal_time_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    return (uint64_t)ts.tv_sec * 1000ULL + ts.tv_nsec / 1000000ULL;
}

void osal_sleep_ms(uint32_t ms)
{
    usleep(ms * 1000);
}

#endif
