#pragma once

#include "smls_node.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Discrete-time integrator parameters
     *
     * Difference equation:
     *   x[k+1] = x[k] + u[k] * dt
     *
     * Initial condition:
     *   x[0] = x0
     *
     * Supported signal types:
     *   scalar / vector
     */
    typedef struct
    {
        float x0; /**< Initial state */
    } smls_integrator_param_t;

    /**
     * @brief Runtime state for integrator
     *
     * state[i] stores the accumulated integral
     * value of the i-th signal element.
     *
     * Shape contract:
     *   dim = number of signal elements
     */
    typedef struct
    {
        float* state; /**< External state buffer */
        uint32_t dim; /**< Number of elements */
    } smls_integrator_state_t;

    /**
     * @brief Shared integrator operator callbacks
     */
    extern const smls_node_ops_t g_smls_integrator_ops;

    /**
     * @brief Execute one integration step
     *
     * x[k+1] = x[k] + u[k] * dt
     */
    int smls_integrator_step(struct smls_node* node);

#ifdef __cplusplus
}
#endif
