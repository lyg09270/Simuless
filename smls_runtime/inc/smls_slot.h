#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Maximum supported tensor dimensions
 *
 * Typical usage:
 * - scalar : rank = 1, dims = {1}
 * - vector : rank = 1, dims = {N}
 * - matrix : rank = 2, dims = {M, N}
 */
#define SMLS_SLOT_MAX_DIMS 4

    /**
     * @brief Slot error codes
     */
    typedef enum
    {
        SMLS_SLOT_ERR_NONE        = 0,
        SMLS_SLOT_ERR_NULL_PTR    = -1,
        SMLS_SLOT_ERR_INVALID_ARG = -2
    } smls_slot_err_t;

    /**
     * @brief Lightweight runtime signal slot
     *
     * Embedded-oriented non-owning memory view.
     *
     * The slot does NOT allocate memory.
     * It only binds to model-owned memory.
     */
    typedef struct
    {
        /**
         * @brief Pointer to external model-owned memory
         */
        float* data;

        /**
         * @brief Tensor rank
         *
         * Examples:
         * - scalar: 1
         * - vector: 1
         * - matrix: 2
         */
        uint8_t rank;

        /**
         * @brief Dimensions for each axis
         *
         * Valid entries are [0, rank).
         */
        uint16_t dims[SMLS_SLOT_MAX_DIMS];

    } smls_slot_t;

    /**
     * @brief Bind slot to external memory
     *
     * @param slot Target slot
     * @param data External memory pointer
     * @param rank Tensor rank
     * @param dims Dimension array
     *
     * @return Error code
     */
    static inline int smls_slot_bind(smls_slot_t* slot, float* data, uint8_t rank,
                                     const uint16_t* dims)
    {
        if ((slot == NULL) || (data == NULL) || (dims == NULL))
        {
            return SMLS_SLOT_ERR_NULL_PTR;
        }

        if ((rank == 0U) || (rank > SMLS_SLOT_MAX_DIMS))
        {
            return SMLS_SLOT_ERR_INVALID_ARG;
        }

        slot->data = data;
        slot->rank = rank;

        for (uint8_t i = 0; i < rank; i++)
        {
            slot->dims[i] = dims[i];
        }

        for (uint8_t i = rank; i < SMLS_SLOT_MAX_DIMS; i++)
        {
            slot->dims[i] = 0U;
        }

        return SMLS_SLOT_ERR_NONE;
    }

    /**
     * @brief Get total number of elements
     *
     * @param slot Input slot
     *
     * @return Total element count
     */
    static inline uint32_t smls_slot_size(const smls_slot_t* slot)
    {
        if (slot == NULL)
        {
            return 0U;
        }

        uint32_t size = 1U;

        for (uint8_t i = 0; i < slot->rank; i++)
        {
            size *= slot->dims[i];
        }

        return size;
    }

    /**
     * @brief Get total memory size in bytes
     *
     * @param slot Input slot
     *
     * @return Total byte size
     */
    static inline uint32_t smls_slot_bytes(const smls_slot_t* slot)
    {
        return smls_slot_size(slot) * sizeof(float);
    }

    /**
     * @brief Clear slot memory to zero
     *
     * @param slot Target slot
     */
    static inline void smls_slot_clear(smls_slot_t* slot)
    {
        if ((slot == NULL) || (slot->data == NULL))
        {
            return;
        }

        memset(slot->data, 0, smls_slot_bytes(slot));
    }

#ifdef __cplusplus
}
#endif
