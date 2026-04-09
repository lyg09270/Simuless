#pragma once

#include <stdint.h>

#define SMLS_EDGE_SIGNAL_MAX_DIM 4

typedef enum
{
    SMLS_DTYPE_BOOL = 0,
    SMLS_DTYPE_FLOAT32,
    SMLS_DTYPE_UINT32,
    SMLS_DTYPE_INT32,
} smls_dtype_t;

typedef enum
{
    SMLS_EDGE_DIM_ROW = 0,
    SMLS_EDGE_DIM_COL,
    SMLS_EDGE_DIM_CHANNEL,
    SMLS_EDGE_DIM_TIME,
} smls_edge_dim_t;

typedef struct
{
    /**
     * @brief Pointer to signal storage
     *
     * Non-owning pointer to the signal memory.
     *
     * This pointer may reference:
     * - scalar value
     * - fixed-length vector
     * - matrix
     * - higher-dimensional tensor
     *
     * The actual element type is described by @ref type.
     *
     * The signal shape is described by @ref dim.
     *
     * Examples:
     * - scalar: &value
     * - vector: array
     * - matrix: 2D array base address
     *
     * Memory ownership remains external.
     * The edge does not allocate or free this memory.
     */
    void* signal;

    /**
     * @brief Signal element data type
     *
     * Defines the data type of each element
     * pointed to by @ref signal.
     *
     * Typical types include:
     * - bool
     * - float32
     * - uint32
     * - int32
     *
     * This field describes the element type only,
     * not the signal shape.
     */
    smls_dtype_t type;

    /**
     * @brief Signal shape rank
     *
     * Number of dimensions in the signal shape.
     *
     * For example:
     * - scalar: rank = 0
     * - vector: rank = 1
     * - matrix: rank = 2
     */
    int rank;

    /**
     * @brief Signal shape description
     *
     * Describes the fixed signal layout in the unified
     * world-signal coordinate system:
     *
     * {ROW, COL, CHANNEL, TIME}
     *
     * Dimension semantics:
     * - ROW:
     *   spatial row / feature dimension / vector length
     *
     * - COL:
     *   spatial column / secondary feature dimension
     *
     * - CHANNEL:
     *   multi-channel signal dimension
     *   (e.g. RGB / IMU axis / sensor channels)
     *
     * - TIME:
     *   temporal sequence length / history window
     *
     * Scalar signals shall be represented as:
     * {1, 1, 1, 1}
     *
     * Examples:
     * - scalar:
     *   {1,1,1,1}
     *
     * - vector[3]:
     *   {3,1,1,1}
     *
     * - matrix[3][3]:
     *   {3,3,1,1}
     *
     * - RGB image:
     *   {H,W,3,1}
     *
     * - video:
     *   {H,W,3,T}
     */
    uint16_t shape[SMLS_EDGE_SIGNAL_MAX_DIM];

    /**
     * @brief Signal timestamp in microseconds
     *
     * Timestamp indicating when the signal
     * was last updated.
     *
     * Used for:
     * - temporal synchronization
     * - multi-rate scheduling
     * - data freshness check
     * - telemetry tracing
     */
    uint64_t timestamp_us;

    /**
     * @brief Monotonic update sequence number
     *
     * Incremented whenever the signal value
     * is updated.
     *
     * Used for:
     * - update detection
     * - data dependency tracking
     * - avoiding redundant recomputation
     */
    uint32_t sequence;

} smls_edge_t;

/**
 * @brief Initialize edge structure
 *
 * Reset the edge to default state:
 * - signal = NULL
 * - default dtype
 * - shape = {1,1,1,1}
 * - timestamp = 0
 * - sequence = 0
 *
 * @param edge Pointer to target edge
 *
 * @return 0 on success, negative on error
 */
int smls_edge_init(smls_edge_t* edge);

/**
 * @brief Bind external signal memory
 *
 * Bind external memory pointer and set data type.
 *
 * This function does not allocate memory.
 * Memory ownership remains external.
 *
 * @param edge Target edge
 * @param signal External signal pointer
 * @param type Signal data type
 *
 * @return 0 on success, negative on error
 */
int smls_edge_signal_bind(smls_edge_t* edge, void* signal, smls_dtype_t type);

/**
 * @brief Get size of one data element
 *
 * Return the size in bytes of one element
 * for the given data type.
 *
 * @param type Signal data type
 *
 * @return Element size in bytes
 */
uint32_t smls_dtype_size(smls_dtype_t type);

/**
 * @brief Get total element count
 *
 * Calculate total element count from shape.
 *
 * count =
 * shape[0] * shape[1] *
 * shape[2] * shape[3]
 *
 * @param edge Target edge
 *
 * @return Total number of elements
 */
uint32_t smls_dtype_element_count(smls_edge_t* edge);

/**
 * @brief Get data type name
 *
 * Return readable string name of dtype.
 *
 * Example:
 * - float32
 * - uint32
 *
 * @param type Signal data type
 *
 * @return Constant string name
 */
const char* smls_dtype_get_name(smls_dtype_t type);
