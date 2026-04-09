#pragma once

#include <stdint.h>

#include "smls_edge.h"
#include "smls_op.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define SMLS_NODE_MAX_INPUTS 8
#define SMLS_NODE_MAX_OUTPUTS 8

    typedef enum
    {
        SMLS_NODE_OK = 0,

        SMLS_NODE_ERR_NULL_PTR       = -1,
        SMLS_NODE_ERR_INPUT_MISSING  = -2,
        SMLS_NODE_ERR_OUTPUT_MISSING = -3,
        SMLS_NODE_ERR_PARAM_NULL     = -4,
        SMLS_NODE_ERR_SHAPE_MISMATCH = -5,

    } smls_node_err_t;

    struct smls_node;

    /* =========================================================
     * Node callback types
     * ========================================================= */

    /**
     * @brief Initialize node runtime state
     *
     * Called once before graph execution.
     *
     * @param node Target node
     *
     * @return
     * 0 on success
     * negative on error
     */
    typedef int (*smls_node_init_fn_t)(struct smls_node* node);

    /**
     * @brief Execute one node step
     *
     * Node implementation reads from input edges
     * and writes to output edges.
     *
     * @param node Target node
     *
     * @return
     * 0 on success
     * negative on error
     */
    typedef int (*smls_node_step_fn_t)(struct smls_node* node);

    /**
     * @brief Reset runtime mutable state
     *
     * @param node Target node
     */
    typedef void (*smls_node_reset_fn_t)(struct smls_node* node);

    /**
     * @brief Validate node configuration
     *
     * Check for:
     * - valid input/output bindings
     * - compatible data types
     * - parameter consistency
     *
     * @param node Target node
     *
     * @return
     * 0 on success
     * negative on error
     */

    typedef int (*smls_node_validate_fn_t)(struct smls_node* node);

    /**
     * @brief Infer output shape from input shapes and parameters
     *
     * Optional callback to compute output signal shapes.
     *
     * @param node Target node
     *
     * @return
     * 0 on success
     * negative on error
     */
    typedef int (*smls_node_shape_fn_t)(struct smls_node* node);

    /* =========================================================
     * Shared operator callbacks
     * ========================================================= */

    /**
     * @brief Shared operator definition
     */
    typedef struct
    {
        /**
         * @brief Operator semantic type
         */
        smls_op_type_t op_type;

        /**
         * @brief Validate callback
         */
        smls_node_validate_fn_t validate;

        /**
         * @brief Shape inference callback
         */
        smls_node_shape_fn_t infer_shape;

        /**
         * @brief Init callback
         */
        smls_node_init_fn_t init;

        /**
         * @brief Step callback
         */
        smls_node_step_fn_t step;

        /**
         * @brief Reset callback
         */
        smls_node_reset_fn_t reset;

    } smls_node_ops_t;

    /* =========================================================
     * Runtime node instance
     * ========================================================= */

    /**
     * @brief Runtime executable node
     */
    typedef struct smls_node
    {
        /**
         * @brief Unique node id
         */
        uint16_t id;

        /**
         * @brief Shared operator callbacks
         */
        const smls_node_ops_t* ops;

        /**
         * @brief Number of valid inputs
         */
        uint16_t input_used_mask;

        /**
         * @brief Number of valid outputs
         */
        uint16_t output_used_mask;

        /**
         * @brief Input edges
         */
        smls_edge_t* inputs[SMLS_NODE_MAX_INPUTS];

        /**
         * @brief Output edges
         */
        smls_edge_t* outputs[SMLS_NODE_MAX_OUTPUTS];

        /**
         * @brief Immutable parameter block
         */
        void* param;

        /**
         * @brief Mutable runtime state
         */
        void* state;

        /**
         * @brief timestamp of last step execution in microseconds
         */
        uint64_t prev_timestamp_us;

        /**
         * @brief Sample period [s]
         */
        float dt;

    } smls_node_t;

    /* =========================================================
     * Node API
     * ========================================================= */
    /**
     * @brief Get shared operator callbacks
     *
     * @param type Operator type
     *
     * @return Shared ops pointer
     */
    const smls_node_ops_t* smls_op_get_ops(smls_op_type_t type);

    /**
     * @brief Create and initialize a node
     *
     * @return
     * 0 on success
     * negative on error
     */
    int smls_node_create(smls_node_t* node, uint16_t id, smls_op_type_t type, void* param,
                         void* state, float dt);

    /**
     * @brief Bind one input edge
     *
     * @return
     * 0 on success
     * negative on error
     */
    int smls_node_bind_input(smls_node_t* node, smls_edge_t* edge, uint8_t slot);

    /**
     * @brief Bind one output edge
     *
     * @return
     * 0 on success
     * negative on error
     */
    int smls_node_bind_output(smls_node_t* node, smls_edge_t* edge, uint8_t slot);

    /**
     * @brief Validate node configuration
     *
     * Check whether node inputs / outputs / params
     * satisfy operator requirements.
     *
     * @return
     * 0 on success
     * negative on error
     */
    int smls_node_validate(smls_node_t* node);

    /**
     * @brief Infer output shape from input shapes and parameters
     *   Optional step to compute output signal shapes based on
     *  input shapes and operator parameters.
     * @return
     * 0 on success
     * negative on error
     */
    int smls_node_infer_shape(smls_node_t* node);

    /**
     * @brief Execute init callback
     *
     * @return
     * 0 on success
     * negative on error
     */
    int smls_node_init(smls_node_t* node);

    /**
     * @brief Execute one node step
     *
     * @return
     * 0 on success
     * negative on error
     */
    int smls_node_step(smls_node_t* node);

    /**
     * @brief Reset runtime state
     */
    void smls_node_reset(smls_node_t* node);

#ifdef __cplusplus
}
#endif
