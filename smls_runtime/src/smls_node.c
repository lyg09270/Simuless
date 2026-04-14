#include "smls_node.h"

#include <string.h>

/*
 * Register operator headers here
 */
#include "smls_op_list.h"

/**
 * @brief Get shared operator callbacks
 */
const smls_node_ops_t* smls_op_get_ops(smls_op_type_t type)
{
    switch (type)
    {
    case SMLS_OP_GAIN:
        return &g_smls_gain_ops;

    case SMLS_OP_DIFF_EQ:
        return &g_smls_diff_eq_ops;

    case SMLS_OP_INTEGRATOR:
        return &g_smls_integrator_ops;

    default:
        return NULL;
    }
}

/**
 * @brief Create and initialize a node
 */
int smls_node_create(smls_node_t* node, uint16_t id, smls_op_type_t type, void* param, void* state,
                     float dt)
{
    const smls_node_ops_t* ops = NULL;

    if (node == NULL)
    {
        return -1;
    }

    ops = smls_op_get_ops(type);
    if (ops == NULL)
    {
        return -1;
    }

    memset(node, 0, sizeof(*node));

    node->id                = id;
    node->ops               = ops;
    node->param             = param;
    node->state             = state;
    node->prev_timestamp_us = 0u;
    node->dt                = dt;

    return 0;
}

/**
 * @brief Bind one input edge
 */
int smls_node_bind_input(smls_node_t* node, smls_edge_t* edge, uint8_t slot)
{
    if ((node == NULL) || (edge == NULL) || (slot >= SMLS_NODE_MAX_INPUTS))
    {
        return -1;
    }

    node->inputs[slot] = edge;

    return 0;
}

/**
 * @brief Bind one output edge
 */
int smls_node_bind_output(smls_node_t* node, smls_edge_t* edge, uint8_t slot)
{
    if ((node == NULL) || (edge == NULL) || (slot >= SMLS_NODE_MAX_OUTPUTS))
    {
        return -1;
    }

    node->outputs[slot] = edge;

    return 0;
}

int smls_node_validate(smls_node_t* node)
{
    if ((node == NULL) || (node->ops == NULL))
    {
        return -1;
    }

    if (node->ops->validate == NULL)
    {
        return 0;
    }

    return node->ops->validate(node);
}

int smls_node_infer_shape(smls_node_t* node)
{
    if ((node == NULL) || (node->ops == NULL))
    {
        return -1;
    }

    if (node->ops->infer_shape == NULL)
    {
        return 0;
    }

    return node->ops->infer_shape(node);
}

/**
 * @brief Execute init callback
 */
int smls_node_init(smls_node_t* node)
{
    if ((node == NULL) || (node->ops == NULL))
    {
        return -1;
    }

    if (node->ops->init == NULL)
    {
        return 0;
    }

    return node->ops->init(node);
}

/**
 * @brief Execute one node step
 */
int smls_node_step(smls_node_t* node)
{
    if ((node == NULL) || (node->ops == NULL) || (node->ops->step == NULL))
    {
        return -1;
    }

    return node->ops->step(node);
}

/**
 * @brief Reset runtime state
 */
void smls_node_reset(smls_node_t* node)
{
    if ((node == NULL) || (node->ops == NULL) || (node->ops->reset == NULL))
    {
        return;
    }

    node->ops->reset(node);
}
