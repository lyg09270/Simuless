#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Maximum supported tensor dimensions
 *
 * Defines the maximum rank supported by one edge payload.
 *
 * Example:
 * - scalar: rank = 1
 * - vector: rank = 1
 * - matrix: rank = 2
 * - tensor: rank = 3 / 4
 */
#define SMLS_EDGE_MAX_DIMS 4

    /**
     * @brief Edge API return codes
     */
    typedef enum
    {
        /**
         * @brief Operation completed successfully
         */
        SMLS_EDGE_ERR_NONE = 0,

        /**
         * @brief Null pointer argument
         */
        SMLS_EDGE_ERR_NULL_PTR = -1,

        /**
         * @brief Invalid argument
         */
        SMLS_EDGE_ERR_INVALID_ARG = -2,

        /**
         * @brief Input size does not match edge payload size
         */
        SMLS_EDGE_ERR_SIZE_MISMATCH = -3,

        /**
         * @brief Memory allocation failed
         */
        SMLS_EDGE_ERR_INSUFFICIENT_MEMORY = -4

    } smls_edge_err_t;

    /**
     * @brief Supported payload data types
     */
    typedef enum
    {
        /**
         * @brief 32-bit floating point
         */
        SMLS_EDGE_DTYPE_FLOAT32 = 0,

        /**
         * @brief 32-bit signed integer
         */
        SMLS_EDGE_DTYPE_INT32 = 1,

        /**
         * @brief 8-bit unsigned integer
         */
        SMLS_EDGE_DTYPE_UINT8 = 2

    } smls_edge_dtype_t;

    /**
     * @brief Edge payload specification
     *
     * Describes the semantic structure of the signal
     * carried by one graph edge.
     */
    typedef struct
    {
        /**
         * @brief Element data type
         */
        smls_edge_dtype_t dtype;

        /**
         * @brief Tensor rank
         *
         * Example:
         * - scalar: 1
         * - vec3: 1
         * - mat3x3: 2
         */
        uint8_t rank;

        /**
         * @brief Tensor dimensions
         *
         * Example:
         * - scalar: {1}
         * - vec3: {3}
         * - mat3x3: {3,3}
         */
        uint32_t dims[SMLS_EDGE_MAX_DIMS];

    } smls_edge_spec_t;

    /**
     * @brief Runtime graph edge object
     *
     * Represents the current signal value
     * transported along one graph edge.
     */
    typedef struct
    {
        /**
         * @brief Payload specification
         */
        smls_edge_spec_t spec;

        /**
         * @brief Runtime payload buffer
         */
        void* data;

        /**
         * @brief Total payload size in bytes
         */
        uint32_t bytes;

        /**
         * @brief Memory ownership flag
         *
         * - 1: internally allocated
         * - 0: externally bound buffer
         */
        uint8_t owns_memory;

        /**
         * @brief Signal timestamp in microseconds
         */
        uint64_t timestamp_us;

        /**
         * @brief Monotonic sequence counter
         *
         * Incremented after each successful write.
         */
        uint64_t sequence_id;

    } smls_edge_t;

    /**
     * @brief Initialize edge metadata
     *
     * This function validates the payload specification
     * and calculates total payload bytes.
     *
     * Memory is NOT allocated at this stage.
     *
     * Use ::smls_edge_alloc() or ::smls_edge_bind()
     * after initialization.
     *
     * @param spec Input payload specification
     * @param edge Output edge object
     *
     * @return
     * - ::SMLS_EDGE_ERR_NONE
     * - ::SMLS_EDGE_ERR_NULL_PTR
     * - ::SMLS_EDGE_ERR_INVALID_ARG
     */
    int smls_edge_init(const smls_edge_spec_t* spec, smls_edge_t* edge);

    /**
     * @brief Allocate internal payload memory
     *
     * Allocates edge buffer and initializes all bytes to zero.
     *
     * @param edge Target edge
     *
     * @return
     * - ::SMLS_EDGE_ERR_NONE
     * - ::SMLS_EDGE_ERR_NULL_PTR
     * - ::SMLS_EDGE_ERR_INVALID_ARG
     * - ::SMLS_EDGE_ERR_INSUFFICIENT_MEMORY
     */
    int smls_edge_alloc(smls_edge_t* edge);

    /**
     * @brief Bind external memory buffer
     *
     * Binds an externally managed buffer to edge.
     *
     * Typical use cases:
     * - static MCU buffers
     * - DMA memory
     * - shared memory
     * - codegen static arrays
     *
     * @param edge Target edge
     * @param buffer External buffer
     * @param bytes Buffer size in bytes
     *
     * @return
     * - ::SMLS_EDGE_ERR_NONE
     * - ::SMLS_EDGE_ERR_NULL_PTR
     * - ::SMLS_EDGE_ERR_SIZE_MISMATCH
     */
    int smls_edge_bind(smls_edge_t* edge, void* buffer, uint32_t bytes);

    /**
     * @brief Release internal payload memory
     *
     * Safe to call multiple times.
     *
     * External bound memory will NOT be freed.
     *
     * @param edge Target edge
     */
    void smls_edge_free(smls_edge_t* edge);

    /**
     * @brief Clear payload buffer
     *
     * Sets all payload bytes to zero.
     *
     * @param edge Target edge
     */
    void smls_edge_clear(smls_edge_t* edge);

    /**
     * @brief Write new signal data to edge
     *
     * Copies input buffer into edge payload buffer.
     *
     * Timestamp and sequence counter are updated.
     *
     * @param edge Target edge
     * @param src Source data buffer
     * @param bytes Copy size in bytes
     * @param timestamp_us Signal timestamp
     *
     * @return
     * - ::SMLS_EDGE_ERR_NONE
     * - ::SMLS_EDGE_ERR_NULL_PTR
     * - ::SMLS_EDGE_ERR_INVALID_ARG
     * - ::SMLS_EDGE_ERR_SIZE_MISMATCH
     */
    int smls_edge_write(smls_edge_t* edge, const void* src, uint32_t bytes, uint64_t timestamp_us);

#ifdef __cplusplus
}
#endif
