#include "smls_integrator.h"

#include "smls_log.h"
#include <string.h>

/**
 * @brief Validate integrator node
 *
 * Supported:
 *   scalar / vector
 *
 * Unsupported:
 *   matrix / tensor
 */
static int smls_integrator_validate(struct smls_node* node)
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

    /* only support scalar / vector */
    if (in_edge->rank > 1)
    {
        return SMLS_NODE_ERR_UNSURPPOTED_SHAPE;
    }

    if (out_edge->rank > 1)
    {
        return SMLS_NODE_ERR_UNSURPPOTED_SHAPE;
    }

    if (in_edge->type != SMLS_DTYPE_FLOAT32)
    {
        return SMLS_NODE_ERR_SHAPE_MISMATCH;
    }

    if (out_edge->type != SMLS_DTYPE_FLOAT32)
    {
        return SMLS_NODE_ERR_SHAPE_MISMATCH;
    }

    return SMLS_NODE_OK;
}

/**
 * @brief Infer output shape for integrator
 *
 * Shape rule:
 *   shape(y) = shape(u)
 *
 * Supported:
 *   scalar / vector
 *
 * Unsupported:
 *   matrix / tensor
 */
static int smls_integrator_infer_shape(struct smls_node* node)
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

    /* only support scalar / vector */
    if (in_edge->rank > 1)
    {
        return SMLS_NODE_ERR_UNSURPPOTED_SHAPE;
    }

    out_edge->type = in_edge->type;
    out_edge->rank = in_edge->rank;

    memcpy(out_edge->shape, in_edge->shape, sizeof(uint16_t) * in_edge->rank);

    return SMLS_NODE_OK;
}

/**
 * @brief integrator step
 *
 * x[k+1] = x[k] + u[k] * dt
 * y[k]   = x[k]
 */
int smls_integrator_step(struct smls_node* node)
{
    int ret = smls_integrator_validate(node);
    if (ret < 0)
    {
        return ret;
    }

    const smls_integrator_param_t* param = (const smls_integrator_param_t*)node->param;

    smls_integrator_state_t* state = (smls_integrator_state_t*)node->state;

    smls_edge_t* in_edge  = node->inputs[0];
    smls_edge_t* out_edge = node->outputs[0];

    if (param == NULL)
    {
        return SMLS_NODE_ERR_PARAM_NULL;
    }

    if ((in_edge->signal == NULL) || (out_edge->signal == NULL) || (state == NULL) ||
        (state->state == NULL))
    {
        return SMLS_NODE_ERR_NULL_PTR;
    }

    if ((in_edge->type != SMLS_DTYPE_FLOAT32) || (out_edge->type != SMLS_DTYPE_FLOAT32))
    {
        return SMLS_NODE_ERR_SHAPE_MISMATCH;
    }

    uint32_t count = smls_dtype_element_count(in_edge);

    float* in  = (float*)in_edge->signal;
    float* out = (float*)out_edge->signal;

    for (uint32_t i = 0; i < count; i++)
    {
        state->state[i] += in[i] * param->dt;

        out[i] = state->state[i];
    }

    return SMLS_NODE_OK;
}

/**
 * @brief Global shared integrator ops
 */
const smls_node_ops_t g_smls_integrator_ops = {.op_type     = SMLS_OP_INTEGRATOR,
                                               .validate    = smls_integrator_validate,
                                               .infer_shape = smls_integrator_infer_shape,
                                               .init        = NULL,
                                               .step        = smls_integrator_step,
                                               .reset       = NULL};
