#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /* =========================
     * Monotonic time
     * ========================= */

    /**
     * @brief Get monotonic time in milliseconds
     */
    uint64_t osal_time_ms(void);

    /**
     * @brief Get monotonic time in microseconds
     */
    uint64_t osal_time_us(void);

    /* =========================
     * Sleep
     * ========================= */

    /**
     * @brief Sleep for given milliseconds
     */
    void osal_sleep_ms(uint32_t ms);

#ifdef __cplusplus
}
#endif