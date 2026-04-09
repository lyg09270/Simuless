#pragma once

#include <stdint.h>

#include "smls_node.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Difference equation parameter
     *
     * Standard discrete-time transfer function:
     *
     *                 b0 + b1 z^-1 + ... + b(M-1) z^-(M-1)
     * H(z) = -------------------------------------------------------
     *         1 + a1 z^-1 + ... + a(N-1) z^-(N-1)
     *
     * Corresponding time-domain difference equation:
     *
     * y[k] =
     *     b0*x[k]
     *   + b1*x[k-1]
     *   + ...
     *   + b(M-1)*x[k-(M-1)]
     *
     *   - a1*y[k-1]
     *   - ...
     *   - a(N-1)*y[k-(N-1)]
     *
     * IMPORTANT:
     * den[0] must always be 1.0f
     *
     * Coefficient storage convention:
     *
     * num = { b0, b1, ..., b(M-1) }
     * den = { 1,  a1, ..., a(N-1) }
     *
     * Example:
     *
     * First-order low-pass filter:
     *
     * y[k] = alpha*x[k] + (1-alpha)*y[k-1]
     *
     * should be defined as:
     *
     * num = { alpha }
     * den = { 1.0f, -(1.0f - alpha) }
     *
     * because the internal implementation uses:
     *
     * y[k] = sum(num*x_hist) - sum(den*y_hist)
     */

    /**
     * @brief Difference equation parameter
     *
     * Standard discrete-time transfer function:
     *
     * y[k] =
     * sum(b_i * x[k-i]) -
     * sum(a_j * y[k-j])
     *
     * where den[0] must be 1.
     */
    typedef struct
    {
        /**
         * @brief Feedforward coefficients
         *
         * numerator coefficients:
         * b0, b1, ..., b(M-1)
         */
        const float* num;

        /**
         * @brief Feedback coefficients
         *
         * denominator coefficients:
         * a0, a1, ..., a(N-1)
         *
         * a0 must always be 1.
         */
        const float* den;

        /**
         * @brief Numerator coefficient count
         */
        uint8_t num_order;

        /**
         * @brief Denominator coefficient count
         */
        uint8_t den_order;

    } smls_diff_eq_param_t;

    /**
     * @brief Difference equation runtime state
     *
     * Stores historical input and output samples.
     */
    typedef struct
    {
        /**
         * @brief Input history buffer
         *
         * size = num_order
         */
        float* input_hist;

        /**
         * @brief Output history buffer
         *
         * size = den_order - 1
         */
        float* output_hist;

    } smls_diff_eq_state_t;

    /**
     * @brief Shared diff_eq operator callbacks
     */
    extern const smls_node_ops_t g_smls_diff_eq_ops;

    /**
     * @brief Difference equation step callback
     *
     * Read:
     * - input edge 0
     *
     * Write:
     * - output edge 0
     *
     * @param node Target node
     *
     * @return
     * 0 on success
     * negative on error
     */
    int smls_diff_eq_step(struct smls_node* node);

    /**
     * @brief Reset difference equation history
     *
     * Clear input and output history buffers.
     *
     * @param node Target node
     */
    void smls_diff_eq_reset(struct smls_node* node);

#ifdef __cplusplus
}
#endif
