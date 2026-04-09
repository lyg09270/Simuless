#include "smls_diff_eq.h"

#include <string.h>

/**
 * @brief Validate difference equation node configuration
 *
 * Current implementation is scalar only.
 */
static int smls_diff_eq_validate(struct smls_node* node)
{
    if ((node == NULL) || (node->param == NULL) || (node->state == NULL))
    {
        return -1;
    }

    const smls_diff_eq_param_t* param = (const smls_diff_eq_param_t*)node->param;

    smls_diff_eq_state_t* state = (smls_diff_eq_state_t*)node->state;

    if ((node->inputs[0] == NULL) || (node->outputs[0] == NULL))
    {
        return -1;
    }

    if ((param->num == NULL) || (param->den == NULL))
    {
        return -1;
    }

    if ((param->num_order == 0u) || (param->den_order == 0u))
    {
        return -1;
    }

    if (param->den[0] == 0.0f)
    {
        return -1;
    }

    if (state->input_hist == NULL)
    {
        return -1;
    }

    if ((param->den_order > 1u) && (state->output_hist == NULL))
    {
        return -1;
    }

    if (node->inputs[0]->signal == NULL || node->outputs[0]->signal == NULL)
    {
        return -1;
    }

    if ((node->inputs[0]->type != SMLS_DTYPE_FLOAT32) ||
        (node->outputs[0]->type != SMLS_DTYPE_FLOAT32))
    {
        return -1;
    }

    return 0;
}

/**
 * @brief Infer output shape from input shape
 *
 * Difference equation keeps the same signal shape as input.
 * Current arithmetic implementation is scalar only, but shape
 * propagation still mirrors input to output.
 */
static int smls_diff_eq_infer_shape(struct smls_node* node)
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
 * @brief Difference equation step
 *
 * y[k] =
 * (sum(b_i * x[k-i]) - sum(a_j * y[k-j])) / a0
 *
 * where:
 * - b_i = num[i]
 * - a0  = den[0]
 * - a_j = den[j], j >= 1
 *
 * Current implementation is scalar only.
 */
int smls_diff_eq_step(struct smls_node* node)
{
    if (smls_diff_eq_validate(node) < 0)
    {
        return -1;
    }

    const smls_diff_eq_param_t* param = (const smls_diff_eq_param_t*)node->param;

    smls_diff_eq_state_t* state = (smls_diff_eq_state_t*)node->state;

    smls_edge_t* in_edge = node->inputs[0];

    smls_edge_t* out_edge = node->outputs[0];

    float* in = (float*)in_edge->signal;

    float* out = (float*)out_edge->signal;

    /* shift input history */
    for (int i = (int)param->num_order - 1; i > 0; i--)
    {
        state->input_hist[i] = state->input_hist[i - 1];
    }

    state->input_hist[0] = *in;

    float y = 0.0f;

    /* feedforward term */
    for (uint8_t i = 0; i < param->num_order; i++)
    {
        y += param->num[i] * state->input_hist[i];
    }

    /* feedback term */
    for (uint8_t i = 1; i < param->den_order; i++)
    {
        y -= param->den[i] * state->output_hist[i - 1];
    }

    /* normalize by den[0] */
    y /= param->den[0];

    /* shift output history */
    if (param->den_order > 1u)
    {
        for (int i = (int)param->den_order - 2; i > 0; i--)
        {
            state->output_hist[i] = state->output_hist[i - 1];
        }

        state->output_hist[0] = y;
    }

    *out = y;

    return 0;
}

/**
 * @brief Reset difference equation state
 */
void smls_diff_eq_reset(struct smls_node* node)
{
    if ((node == NULL) || (node->param == NULL) || (node->state == NULL))
    {
        return;
    }

    const smls_diff_eq_param_t* param = (const smls_diff_eq_param_t*)node->param;

    smls_diff_eq_state_t* state = (smls_diff_eq_state_t*)node->state;

    if (state->input_hist != NULL)
    {
        memset(state->input_hist, 0, sizeof(float) * param->num_order);
    }

    if ((param->den_order > 1u) && (state->output_hist != NULL))
    {
        memset(state->output_hist, 0, sizeof(float) * (param->den_order - 1u));
    }
}

/**
 * @brief Shared diff_eq ops
 */
const smls_node_ops_t g_smls_diff_eq_ops = {.op_type     = SMLS_OP_DIFF_EQ,
                                            .validate    = smls_diff_eq_validate,
                                            .infer_shape = smls_diff_eq_infer_shape,
                                            .init        = NULL,
                                            .step        = smls_diff_eq_step,
                                            .reset       = smls_diff_eq_reset};
