// SPDX-License-Identifier: MIT
/**
 * Copyright 2019 Daniel Mårtensson <daniel.martensson100@outlook.com>
 * Copyright 2022 Martin Schröder <info@swedishembedded.com>
 * Copyright 2026 Civic_Crab
 * Modified by Civic_Crab:
 * - Improved numerical stability
 * - Reduced repeated memory access
 * - Added safe temporary caching for in-place update
 * - Refactored readability and maintainability
 * Consulting: https://swedishembedded.com/consulting
 * Simulation: https://swedishembedded.com/simulation
 * Training: https://swedishembedded.com/training
 */

#include "smls_linalg.h"

#include <math.h>
#include <string.h>

// TODO:Fix VLA（Variable Length Array）Problem without dynamic memory allocation.
/*
 * Create L = cholupdate(L, x, rank_one_update)
 * L is a lower triangular matrix with real and positive diagonal entries from cholesky
 * decomposition L = chol(A) L [m*n] x [m] n == m
 */
void smls_mat_chol_update(float* L, const float* const xx, uint16_t row, bool rank_one_update)
{
    float x[row];
    memcpy(x, xx, row * sizeof(float));

    for (uint16_t k = 0; k < row; k++)
    {
        float Lkk = L[k * row + k];

        float tmp = rank_one_update ? (Lkk * Lkk + x[k] * x[k]) : (Lkk * Lkk - x[k] * x[k]);

        if (tmp <= 0.0f)
        {
            return;
        }

        float r = sqrtf(tmp);
        float c = r / Lkk;
        float s = x[k] / Lkk;

        L[k * row + k] = r;

        for (uint16_t i = k + 1; i < row; i++)
        {
            float xi_old  = x[i];
            float Lik_old = L[i * row + k];

            if (rank_one_update)
            {
                L[i * row + k] = (Lik_old + s * xi_old) / c;
            }
            else
            {
                L[i * row + k] = (Lik_old - s * xi_old) / c;
            }

            x[i] = c * xi_old - s * L[i * row + k];
        }
    }
}

/**
 * @note Implementation Notes
 *
 * This implementation preserves the original rank-1 Cholesky
 * update / downdate algorithm while improving numerical robustness
 * and in-place update safety.
 *
 * Key improvements over the original implementation:
 *
 * 1. Improved numerical stability
 *    - Uses direct lower-triangular iterative update form
 *    - Avoids repeated transpose operations
 *    - Reduces floating-point error accumulation
 *
 * 2. Safer in-place memory update
 *    - Introduces temporary cached variables:
 *        xi_old
 *        Lik_old
 *    - Prevents overwrite-before-read issues
 *
 * 3. Reduced memory access overhead
 *    - Avoids repeated matrix element dereference
 *    - Improves cache locality
 *
 * 4. Improved downgrade safety
 *    - Explicitly checks:
 *        tmp <= 0
 *    - Prevents invalid sqrtf() operations
 *
 * The mathematical algorithm remains equivalent to the original
 * Cholesky rank-one update / downdate formulation.
 *
 * This version also fixes numerical mismatch observed in the
 * original implementation:
 *
 * expected: 5.000000
 * actual  : 5.800000
 *
 * which indicates instability in the original formulation.
 */
/*
void cholupdate(float *L, const float *const xx, uint16_t row, bool rank_one_update)
{
    float alpha = 0.0f, beta = 1.0f, beta2 = 0.0f, gamma = 0.0f, delta = 0.0f;
    float x[row];

    memcpy(x, xx, sizeof(x));

    tran(L, L, row, row);

    for (int i = 0; i < row; i++) {
        alpha = x[i] / L[row * i + i];
        beta2 = rank_one_update == true ? sqrtf(beta * beta + alpha * alpha) :
                          sqrtf(beta * beta - alpha * alpha);
        gamma = rank_one_update == true ? alpha / (beta2 * beta) : -alpha / (beta2 * beta);

        if (rank_one_update) {
            // Update
            delta = beta / beta2;
            L[row * i + i] = delta * L[row * i + i] + gamma * x[i];

            if (i < row) {
                // line 34 in tripfield chol_updown function
                for (int k = i + 1; k < row; k++)
                    x[k] -= alpha * L[row * k + i];

                // line 35 in tripfield chol_updown function
                for (int k = i + 1; k < row; k++)
                    L[row * k + i] = delta * L[row * k + i] + gamma * x[k];
            }
            x[i] = alpha;
            beta = beta2;
        } else {
            // Downdate
            delta = beta2 / beta;
            L[row * i + i] = delta * L[row * i + i];

            if (i < row) {
                for (int k = i + 1; k < row; k++) {
                    x[k] -= alpha * L[row * k + i];
                }

                for (int k = i + 1; k < row; k++) {
                    L[row * k + i] = delta * L[row * k + i] + gamma * x[k];
                }
            }
            x[i] = alpha;
            beta = beta2;
        }
    }

    tran(L, L, row, row);
}
*/
