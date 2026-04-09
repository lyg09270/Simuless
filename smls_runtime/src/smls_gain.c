#include "smls_gain.h"

#include "smls_log.h"
#include <string.h>

/**
 * @brief Validate gain node configuration
 *
 * Require:
 * - input[0]
 * - output[0]
 * - param
 */
static int smls_gain_validate(struct smls_node* node)
{
    if (node == NULL)
    {
        return -1;
    }

    if (node->param == NULL)
    {
        return -1;
    }

    if (node->inputs[0] == NULL)
    {
        return -1;
    }

    if (node->outputs[0] == NULL)
    {
        return -1;
    }

    return 0;
}

/**
 * @brief Infer output shape
 *
 * gain keeps the same shape as input
 *
 * y = k * x
 */
static int smls_gain_infer_shape(struct smls_node* node)
{
    if (node == NULL)
    {
        return -1;
    }

    smls_edge_t* in_edge = node->inputs[0];

    smls_edge_t* out_edge = node->outputs[0];

    if ((in_edge == NULL) || (out_edge == NULL))
    {
        return -1;
    }

    out_edge->type = in_edge->type;
    out_edge->rank = in_edge->rank;

    memcpy(out_edge->shape, in_edge->shape, sizeof(uint16_t) * in_edge->rank);

    return 0;
}

/**
 * @brief Gain step
 *
 * y = k * x
 */
int smls_gain_step(struct smls_node* node)
{
    if (smls_gain_validate(node) < 0)
    {
        return -1;
    }

    const smls_gain_param_t* param = (const smls_gain_param_t*)node->param;

    smls_edge_t* in_edge = node->inputs[0];

    smls_edge_t* out_edge = node->outputs[0];

    if ((in_edge->signal == NULL) || (out_edge->signal == NULL))
    {
        return -1;
    }

    if ((in_edge->type != SMLS_DTYPE_FLOAT32) || (out_edge->type != SMLS_DTYPE_FLOAT32))
    {
        return -1;
    }

    uint32_t count = smls_dtype_element_count(in_edge);

    SMLS_LOG("Gain step", " count=%u, k=%.2f", count, param->k);

    float* in = (float*)in_edge->signal;

    float* out = (float*)out_edge->signal;

    for (uint32_t i = 0; i < count; i++)
    {
        out[i] = param->k * in[i];
    }

    return 0;
}

/**
 * @brief Global shared gain ops
 */
const smls_node_ops_t g_smls_gain_ops = {.op_type     = SMLS_OP_GAIN,
                                         .validate    = smls_gain_validate,
                                         .infer_shape = smls_gain_infer_shape,
                                         .init        = NULL,
                                         .step        = smls_gain_step,
                                         .reset       = NULL};
