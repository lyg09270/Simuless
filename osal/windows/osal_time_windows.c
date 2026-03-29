#include "osal_time.h"

#ifdef OSAL_PLATFORM_WINDOWS

#include <windows.h>

/* =========================
 * High resolution timer
 * ========================= */

static LARGE_INTEGER g_freq;
static int g_init = 0;

static void osal_timer_init(void)
{
    if (!g_init)
    {
        QueryPerformanceFrequency(&g_freq);
        g_init = 1;
    }
}

uint64_t osal_time_us(void)
{
    osal_timer_init();

    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);

    return (uint64_t)(counter.QuadPart * 1000000ULL / g_freq.QuadPart);
}

uint64_t osal_time_ms(void)
{
    return osal_time_us() / 1000ULL;
}

void osal_sleep_ms(uint32_t ms)
{
    Sleep(ms);
}

#endif