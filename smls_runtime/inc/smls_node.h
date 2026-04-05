#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Forward declaration of node base type
     */
    struct smls_node;

    /**
     * @brief Node initialization callback
     *
     * Called once during model initialization.
     *
     * Typical use cases:
     * - initialize internal state
     * - validate slot binding
     * - preload default parameters
     *
     * @param node Target node instance
     *
     * @return
     * - 0 on success
     * - negative value on failure
     */
    typedef int (*smls_node_init_fn_t)(struct smls_node* node);

    /**
     * @brief Node reset callback
     *
     * Reset runtime internal state.
     *
     * Typical use cases:
     * - clear controller integrator
     * - reset filter memory
     * - reset estimator covariance
     *
     * @param node Target node instance
     */
    typedef void (*smls_node_reset_fn_t)(struct smls_node* node);

    /**
     * @brief Node parameter update callback
     *
     * Used to update node parameters during runtime.
     *
     * @param node Target node instance
     * @param param Parameter object pointer
     *
     * @return
     * - 0 on success
     * - negative value on failure
     */
    typedef int (*smls_node_set_param_fn_t)(struct smls_node* node, const void* param);

    /**
     * @brief Node runtime step callback
     *
     * Executes one algorithm update step.
     *
     * Step time should be read from @ref dt.
     *
     * @param node Target node instance
     */
    typedef void (*smls_node_step_fn_t)(struct smls_node* node);

    /**
     * @brief Node operation table
     *
     * Provides common lifecycle interface for
     * all derived runtime nodes.
     */
    typedef struct
    {
        /**
         * @brief Initialize node
         */
        smls_node_init_fn_t init;

        /**
         * @brief Reset node runtime state
         */
        smls_node_reset_fn_t reset;

        /**
         * @brief Update node parameters
         */
        smls_node_set_param_fn_t set_param;

        /**
         * @brief Execute one step
         */
        smls_node_step_fn_t step;

    } smls_node_ops_t;

    /**
     * @brief Base runtime node
     *
     * Common base class for all runtime nodes.
     *
     * Derived node types should embed this
     * structure as the first member.
     */
    typedef struct smls_node
    {
        /**
         * @brief Node virtual operation table
         */
        const smls_node_ops_t* ops;

        /**
         * @brief Algorithm step time in seconds
         *
         * Used directly by controller /
         * estimator internal computation.
         */
        float dt;

        /**
         * @brief Execution divider relative to
         * model base tick
         *
         * Example:
         * - 1   : every tick
         * - 10  : every 10 ticks
         * - 100 : every 100 ticks
         */
        uint16_t rate_div;

        /**
         * @brief Runtime tick accumulator
         */
        uint16_t tick_count;

    } smls_node_t;

    /**
     * @brief Initialize node
     *
     * Safe to call even if callback is NULL.
     *
     * @param node Target node
     *
     * @return
     * - 0 on success
     * - negative value on failure
     */
    static inline int smls_node_init(smls_node_t* node)
    {
        if ((node == NULL) || (node->ops == NULL) || (node->ops->init == NULL))
        {
            return 0;
        }

        return node->ops->init(node);
    }

    /**
     * @brief Reset node state
     *
     * Safe to call even if callback is NULL.
     *
     * @param node Target node
     */
    static inline void smls_node_reset(smls_node_t* node)
    {
        if ((node == NULL) || (node->ops == NULL) || (node->ops->reset == NULL))
        {
            return;
        }

        node->ops->reset(node);
    }

    /**
     * @brief Update node parameters
     *
     * Safe to call even if callback is NULL.
     *
     * @param node Target node
     * @param param Parameter pointer
     *
     * @return
     * - 0 on success
     * - negative value on failure
     */
    static inline int smls_node_set_param(smls_node_t* node, const void* param)
    {
        if ((node == NULL) || (node->ops == NULL) || (node->ops->set_param == NULL))
        {
            return 0;
        }

        return node->ops->set_param(node, param);
    }

    /**
     * @brief Execute one node step
     *
     * Safe to call even if callback is NULL.
     *
     * @param node Target node
     */
    static inline void smls_node_step(smls_node_t* node)
    {
        if ((node == NULL) || (node->ops == NULL) || (node->ops->step == NULL))
        {
            return;
        }

        node->ops->step(node);
    }

    /**
     * @brief Check whether node should run
     *
     * This function implements simple
     * integer-rate scheduling.
     *
     * @param node Target node
     *
     * @return
     * - 1 if node should execute
     * - 0 otherwise
     */
    static inline uint8_t smls_node_should_step(smls_node_t* node)
    {
        if ((node == NULL) || (node->rate_div == 0U))
        {
            return 0U;
        }

        node->tick_count++;

        if (node->tick_count >= node->rate_div)
        {
            node->tick_count = 0U;
            return 1U;
        }

        return 0U;
    }

    /**
     * @brief Configure node execution rate
     *
     * @param node Target node
     * @param dt Step time in seconds
     * @param rate_div Divider relative to
     * model base tick
     */
    static inline void smls_node_set_rate(smls_node_t* node, float dt, uint16_t rate_div)
    {
        if (node == NULL)
        {
            return;
        }

        node->dt         = dt;
        node->rate_div   = rate_div;
        node->tick_count = 0U;
    }

#ifdef __cplusplus
}
#endif
