#pragma once

#include "smls_node.h"
#include "smls_slot.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Integrator parameter
     */
    typedef struct
    {
        /**
         * @brief Integration gain
         */
        float gain;

        /**
         * @brief Initial value
         */
        float init_value;

    } smls_integrator_param_t;

    /**
     * @brief Integrator runtime node
     */
    typedef struct
    {
        /**
         * @brief Base node
         */
        smls_node_t base;

        /**
         * @brief Input slot
         */
        smls_slot_t* input;

        /**
         * @brief Output slot
         */
        smls_slot_t* output;

        /**
         * @brief Gain
         */
        float gain;

        /**
         * @brief Internal state
         */
        float state;

    } smls_integrator_t;

    int smls_integrator_init(struct smls_node* node);

    void smls_integrator_reset(struct smls_node* node);

    int smls_integrator_set_param(struct smls_node* node, const void* param);

    void smls_integrator_step(struct smls_node* node);

    extern const smls_node_ops_t smls_integrator_ops;

#ifdef __cplusplus
}
#endif
