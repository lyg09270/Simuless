#pragma once

#include "smls_node.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Gain parameter
     *
     * y = k * x
     */
    typedef struct
    {
        float k;

    } smls_gain_param_t;

    /**
     * @brief Shared gain operator callbacks
     */
    extern const smls_node_ops_t g_smls_gain_ops;

    /**
     * @brief Gain step implementation
     *
     * @param node Target node
     *
     * @return
     * 0 on success
     * negative on error
     */
    int smls_gain_step(struct smls_node* node);

#ifdef __cplusplus
}
#endif
