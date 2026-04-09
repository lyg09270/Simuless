#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Operator type for graph node IR
     *
     * This is the semantic operator definition layer.
     *
     * It is similar to ONNX op_type, but optimized
     * for control systems and embedded runtime.
     */
    typedef enum
    {
        /* =========================================
         * Arithmetic
         * ========================================= */
        SMLS_OP_NOP = 0,

        SMLS_OP_ADD,
        SMLS_OP_SUB,
        SMLS_OP_MUL,
        SMLS_OP_DIV,
        SMLS_OP_NEG,
        SMLS_OP_ABS,

        /* =========================================
         * Comparison / logic
         * ========================================= */
        SMLS_OP_GT,
        SMLS_OP_LT,
        SMLS_OP_EQ,
        SMLS_OP_MIN,
        SMLS_OP_MAX,

        /* =========================================
         * Control semantic
         * ========================================= */
        SMLS_OP_GAIN,
        SMLS_OP_ERR,
        SMLS_OP_CLAMP,
        SMLS_OP_SAT,
        SMLS_OP_SIGN,

        /* =========================================
         * Stateful dynamic system
         * ========================================= */
        SMLS_OP_INTEGRATOR,
        SMLS_OP_DIFF_EQ,
        SMLS_OP_PID,
        SMLS_OP_LPF,
        SMLS_OP_BIQUAD,

        /* =========================================
         * State-space
         * ========================================= */
        SMLS_OP_STATE_SPACE,

        /* =========================================
         * Signal routing
         * ========================================= */
        SMLS_OP_MUX,
        SMLS_OP_DEMUX,
        SMLS_OP_DELAY,
        SMLS_OP_ZOH,

    } smls_op_type_t;

#ifdef __cplusplus
}
#endif
