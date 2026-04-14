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
        return SMLS_NODE_ERR_NULL_PTR;
    }

    if (node->param == NULL)
    {
        return SMLS_NODE_ERR_PARAM_NULL;
    }

    if (node->inputs[0] == NULL)
    {
        return SMLS_NODE_ERR_INPUT_MISSING;
    }

    if (node->outputs[0] == NULL)
    {
        return SMLS_NODE_ERR_OUTPUT_MISSING;
    }

    return SMLS_NODE_OK;
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
        return SMLS_NODE_ERR_NULL_PTR;
    }

    smls_edge_t* in_edge  = node->inputs[0];
    smls_edge_t* out_edge = node->outputs[0];

    if (in_edge == NULL)
    {
        return SMLS_NODE_ERR_INPUT_MISSING;
    }

    if (out_edge == NULL)
    {
        return SMLS_NODE_ERR_OUTPUT_MISSING;
    }

    out_edge->type = in_edge->type;
    out_edge->rank = in_edge->rank;

    memcpy(out_edge->shape, in_edge->shape, sizeof(uint16_t) * in_edge->rank);

    return SMLS_NODE_OK;
}

/**
 * @brief Gain step
 *
 * y = k * x
 */
int smls_gain_step(struct smls_node* node)
{
    int ret = smls_gain_validate(node);
    if (ret < 0)
    {
        return ret;
    }

    const smls_gain_param_t* param = (const smls_gain_param_t*)node->param;

    smls_edge_t* in_edge  = node->inputs[0];
    smls_edge_t* out_edge = node->outputs[0];

    if ((in_edge->signal == NULL) || (out_edge->signal == NULL))
    {
        return SMLS_NODE_ERR_NULL_PTR;
    }

    if ((in_edge->type != SMLS_DTYPE_FLOAT32) || (out_edge->type != SMLS_DTYPE_FLOAT32))
    {
        return SMLS_NODE_ERR_SHAPE_MISMATCH;
    }

    uint32_t count = smls_dtype_element_count(in_edge);

    // SMLS_LOG("Gain step", " count=%u, k=%.2f", count, param->k);

    float* in  = (float*)in_edge->signal;
    float* out = (float*)out_edge->signal;

    for (uint32_t i = 0; i < count; i++)
    {
        out[i] = param->k * in[i];
    }

    return SMLS_NODE_OK;
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
