#pragma once

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct
    {
        float* data;
        size_t capacity;
        size_t head;
        size_t tail;
        size_t size;
    } smls_ringbuf_t;

    /**
     * @brief Initialize ring buffer
     *
     * @param rb Ring buffer object
     * @param storage External storage memory
     * @param capacity Number of elements
     */
    void smls_ringbuf_init(smls_ringbuf_t* rb, float* storage, size_t capacity);

    /**
     * @brief Push one sample
     *
     * Overwrites oldest sample if full.
     */
    void smls_ringbuf_push(smls_ringbuf_t* rb, float value);

    /**
     * @brief Pop oldest sample
     *
     * @return true if success
     */
    bool smls_ringbuf_pop(smls_ringbuf_t* rb, float* out);

    /**
     * @brief Get newest sample
     */
    bool smls_ringbuf_latest(const smls_ringbuf_t* rb, float* out);

    /**
     * @brief Access sample by age
     *
     * age = 0 newest
     * age = 1 previous
     */
    bool smls_ringbuf_at(const smls_ringbuf_t* rb, size_t age, float* out);

    /**
     * @brief Check empty
     */
    bool smls_ringbuf_empty(const smls_ringbuf_t* rb);

    /**
     * @brief Check full
     */
    bool smls_ringbuf_full(const smls_ringbuf_t* rb);

    /**
     * @brief Current size
     */
    size_t smls_ringbuf_size(const smls_ringbuf_t* rb);

#ifdef __cplusplus
}
#endif
