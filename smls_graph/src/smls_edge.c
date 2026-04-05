#include "smls_edge.h"

#include <stdlib.h>
#include <string.h>

/* =========================================================
 * Internal helpers
 * ========================================================= */

static uint32_t smls_edge_dtype_size(smls_edge_dtype_t dtype)
{
    switch (dtype)
    {
    case SMLS_EDGE_DTYPE_FLOAT32:
        return sizeof(float);

    case SMLS_EDGE_DTYPE_INT32:
        return sizeof(int32_t);

    case SMLS_EDGE_DTYPE_UINT8:
        return sizeof(uint8_t);

    default:
        return 0u;
    }
}

static uint32_t smls_edge_calc_bytes(const smls_edge_spec_t* spec)
{
    if (spec == NULL)
    {
        return 0u;
    }

    if (spec->rank == 0u || spec->rank > SMLS_EDGE_MAX_DIMS)
    {
        return 0u;
    }

    uint32_t elem_size = smls_edge_dtype_size(spec->dtype);

    if (elem_size == 0u)
    {
        return 0u;
    }

    uint32_t count = 1u;

    for (uint8_t i = 0u; i < spec->rank; i++)
    {
        if (spec->dims[i] == 0u)
        {
            return 0u;
        }

        count *= spec->dims[i];
    }

    return count * elem_size;
}

/* =========================================================
 * Public API
 * ========================================================= */

int smls_edge_init(const smls_edge_spec_t* spec, smls_edge_t* edge)
{
    if (spec == NULL || edge == NULL)
    {
        return SMLS_EDGE_ERR_NULL_PTR;
    }

    uint32_t bytes = smls_edge_calc_bytes(spec);

    if (bytes == 0u)
    {
        return SMLS_EDGE_ERR_INVALID_ARG;
    }

    memset(edge, 0, sizeof(*edge));

    edge->spec  = *spec;
    edge->bytes = bytes;

    return SMLS_EDGE_ERR_NONE;
}

int smls_edge_alloc(smls_edge_t* edge)
{
    if (edge == NULL)
    {
        return SMLS_EDGE_ERR_NULL_PTR;
    }

    if (edge->bytes == 0u)
    {
        return SMLS_EDGE_ERR_INVALID_ARG;
    }

    if (edge->data != NULL)
    {
        return SMLS_EDGE_ERR_INVALID_ARG;
    }

    edge->data = malloc(edge->bytes);

    if (edge->data == NULL)
    {
        return SMLS_EDGE_ERR_INSUFFICIENT_MEMORY;
    }

    memset(edge->data, 0, edge->bytes);

    edge->owns_memory = 1u;

    return SMLS_EDGE_ERR_NONE;
}

int smls_edge_bind(smls_edge_t* edge, void* buffer, uint32_t bytes)
{
    if (edge == NULL || buffer == NULL)
    {
        return SMLS_EDGE_ERR_NULL_PTR;
    }

    if (bytes != edge->bytes)
    {
        return SMLS_EDGE_ERR_SIZE_MISMATCH;
    }

    edge->data        = buffer;
    edge->owns_memory = 0u;

    return SMLS_EDGE_ERR_NONE;
}

void smls_edge_free(smls_edge_t* edge)
{
    if (edge == NULL)
    {
        return;
    }

    if (edge->owns_memory && edge->data != NULL)
    {
        free(edge->data);
    }

    edge->data        = NULL;
    edge->owns_memory = 0u;
}

void smls_edge_clear(smls_edge_t* edge)
{
    if (edge == NULL || edge->data == NULL)
    {
        return;
    }

    memset(edge->data, 0, edge->bytes);
}

int smls_edge_write(smls_edge_t* edge, const void* src, uint32_t bytes, uint64_t timestamp_us)
{
    if (edge == NULL || src == NULL)
    {
        return SMLS_EDGE_ERR_NULL_PTR;
    }

    if (edge->data == NULL)
    {
        return SMLS_EDGE_ERR_INVALID_ARG;
    }

    if (bytes != edge->bytes)
    {
        return SMLS_EDGE_ERR_SIZE_MISMATCH;
    }

    memcpy(edge->data, src, bytes);

    edge->timestamp_us = timestamp_us;
    edge->sequence_id++;

    return SMLS_EDGE_ERR_NONE;
}
