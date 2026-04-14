#include "smls_edge.h"

#include "smls_log.h"
#include <string.h>

int smls_edge_init(smls_edge_t* edge)
{
    if (edge == NULL)
    {
        return -1;
    }

    edge->signal = NULL;

    edge->type = SMLS_DTYPE_FLOAT32;

    for (uint32_t i = 0; i < SMLS_EDGE_SIGNAL_MAX_DIM; i++)
    {
        edge->shape[i] = 1;
    }

    edge->timestamp_us = 0u;
    edge->sequence     = 0u;

    return 0;
}

int smls_edge_signal_bind(smls_edge_t* edge, void* signal, smls_dtype_t type)
{
    if ((edge == NULL) || (signal == NULL))
    {
        return -1;
    }

    edge->signal = signal;
    edge->type   = type;

    return 0;
}

uint32_t smls_dtype_size(smls_dtype_t type)
{
    switch (type)
    {
    case SMLS_DTYPE_BOOL:
        return sizeof(uint8_t);

    case SMLS_DTYPE_FLOAT32:
        return sizeof(float);

    case SMLS_DTYPE_UINT32:
        return sizeof(uint32_t);

    case SMLS_DTYPE_INT32:
        return sizeof(int32_t);

    default:
        return 0u;
    }
}

uint32_t smls_dtype_element_count(smls_edge_t* edge)
{
    if (edge == NULL)
    {
        return 0u;
    }

    uint32_t count = 1u;

    for (uint32_t i = 0; i < SMLS_EDGE_SIGNAL_MAX_DIM; i++)
    {
        count *= edge->shape[i];
        // SMLS_LOG("smls_edge", " dim=%u size=%u count=%u", i, edge->shape[i], count);
    }

    return count;
}

const char* smls_dtype_get_name(smls_dtype_t type)
{
    switch (type)
    {
    case SMLS_DTYPE_BOOL:
        return "bool";

    case SMLS_DTYPE_FLOAT32:
        return "float32";

    case SMLS_DTYPE_UINT32:
        return "uint32";

    case SMLS_DTYPE_INT32:
        return "int32";

    default:
        return "unknown";
    }
}
